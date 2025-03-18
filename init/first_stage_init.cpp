#define LOG_TAG "init"

#include <dirent.h>
#include <fcntl.h>
#include <linux/mount.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/sysmacros.h>
#include <sys/utsname.h>
#include <unistd.h>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <memory>
#include <regex>
#include <string>
#include <vector>

#include "modprobe.h"

#include <bits/std_thread.h>
#include "first_stage_mount.h"
#include "fs_mgr.h"
#include "libbase.h"
#include "log_new.h"
#include "property_manager.h"
#include "reboot_utils.h"
#include "util.h"
#include "verify.h"

using namespace std::literals;
namespace fs = std::filesystem;
const char* kSecondStageRes = "/second_stage_res";

namespace minimal_systems {
namespace init {

namespace {

enum class BootMode {
    NORMAL_MODE,
    RECOVERY_MODE,
    CHARGER_MODE,
};

void FreeRamdisk() {
    if (!IsRunningInRamdisk()) {
        LOGI("Not running in a ramdisk, skipping cleanup.");
        return;
    }

    const std::string ramdisks[] = {"/dev/ram0", "/dev/initrd"};

    for (const auto& ramdisk : ramdisks) {
        if (umount(ramdisk.c_str()) == 0) {
            LOGI("Unmounted %s", ramdisk.c_str());
        } else {
            LOGE("Failed to unmount %s: %s", ramdisk.c_str(), strerror(errno));
        }

        if (unlink(ramdisk.c_str()) == 0) {
            LOGI("Removed %s", ramdisk.c_str());
        } else {
            LOGE("Failed to remove %s: %s", ramdisk.c_str(), strerror(errno));
        }
    }
}

bool ForceNormalBoot(const std::string& cmdline, const std::string& bootconfig) {
    return bootconfig.find("sysboot.force_normal_boot = \"1\"") != std::string::npos ||
           cmdline.find("sysboot.force_normal_boot=1") != std::string::npos;
}

static void Copy(const char* src, const char* dst) {
    if (link(src, dst) == 0) {
        LOGI("Hard linked %s to %s", src, dst);
        return;
    }

    LOGE("Hard link failed, copying file instead");

    int src_fd = open(src, O_RDONLY);
    int dst_fd = open(dst, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (src_fd < 0 || dst_fd < 0) {
        LOGE("File error: %s", strerror(errno));
        if (src_fd >= 0) close(src_fd);
        if (dst_fd >= 0) close(dst_fd);
        return;
    }

    char buffer[4096];
    ssize_t bytes;
    while ((bytes = read(src_fd, buffer, sizeof(buffer))) > 0) {
        if (write(dst_fd, buffer, bytes) != bytes) {
            LOGE("Write error: %s", strerror(errno));
            break;
        }
    }

    if (bytes < 0) LOGE("Read error: %s", strerror(errno));

    close(src_fd);
    close(dst_fd);
    LOGI("Copied %s to %s", src, dst);
}

void PrepareSwitchRoot() {
    static constexpr const char* src_bin = "/init";       // Main init binary
    static constexpr const char* dst = "/new_root/init";  // Target location

    if (access(dst, X_OK) == 0) {
        LOGI("%s already exists and is executable", dst);
        return;
    }

    std::string dst_dir = fs::path(dst).parent_path();
    std::error_code ec;
    if (access(dst_dir.c_str(), F_OK) != 0) {
        if (!fs::create_directories(dst_dir, ec)) {
            LOGE("Cannot create %s: %s", dst_dir.c_str(), ec.message().c_str());
            return;
        }
    }

    Copy(src_bin, dst);
}

bool IsChargerMode() {
    return GetProperty("ro.bootmode") == "charger";
}

bool IsRecoveryMode() {
    return GetProperty("ro.bootmode") == "recovery";
}

bool IsNormalBootForced() {
    return GetProperty("ro.bootmode") == "normal";
}

std::string GetPageSizeSuffix() {
    static const size_t page_size = sysconf(_SC_PAGE_SIZE);
    if (page_size <= 4096) {
        return "";
    }
    LOGD("_%zuk", page_size / 1024);
    return minimal_systems::base::StringPrintf("_%zuk", page_size / 1024);
}

constexpr bool EndsWith(const std::string_view str, const std::string_view suffix) {
    return str.size() >= suffix.size() &&
           0 == str.compare(str.size() - suffix.size(), suffix.size(), suffix);
}

constexpr std::string_view GetPageSizeSuffix(std::string_view dirname) {
    if (EndsWith(dirname, "_16k")) {
        return "_16k";
    }
    if (EndsWith(dirname, "_64k")) {
        return "_64k";
    }
    return "";
}

}  // namespace

std::string GetModuleLoadList(BootMode boot_mode, const std::string& dir_path) {
    std::string module_load_file;

    switch (boot_mode) {
        case BootMode::NORMAL_MODE:
            module_load_file = "modules.load";
            break;
        case BootMode::RECOVERY_MODE:
            module_load_file = "modules.load.recovery";
            break;
        case BootMode::CHARGER_MODE:
            module_load_file = "modules.load.charger";
            break;
    }

    if (module_load_file != "modules.load") {
        struct stat fileStat{};
        std::string load_path = dir_path + "/" + module_load_file;
        if (stat(load_path.c_str(), &fileStat)) {
            module_load_file = "modules.load";
        }
    }

    return module_load_file;
}

#define MODULE_BASE_DIR "/lib/modules"

bool LoadKernelModules(BootMode boot_mode, bool want_console, bool want_parallel,
                       int& modules_loaded) {
    struct utsname uts{};
    if (uname(&uts)) {
        LOGE("Failed to get kernel version.");
    }
    int major = 0, minor = 0;
    if (sscanf(uts.release, "%d.%d", &major, &minor) != 2) {
        LOGE("Failed to parse kernel version ", uts.release);
    }

    std::unique_ptr<DIR, int (*)(DIR*)> base_dir(opendir(MODULE_BASE_DIR),
                                                 [](DIR* dir) { return closedir(dir); });
    if (!base_dir) {
        LOGI("Unable to open /lib/modules, skipping module loading.");
        return true;
    }
    dirent* entry = nullptr;
    std::vector<std::string> module_dirs;
    const auto page_size_suffix = GetPageSizeSuffix();
    const std::string release_specific_module_dir = uts.release + page_size_suffix;
    while ((entry = readdir(base_dir.get()))) {
        if (entry->d_type != DT_DIR) {
            continue;
        }
        if (entry->d_name == release_specific_module_dir) {
            LOGI("Release specific kernel module dir %s found, loading modules from here with no "
                 "fallbacks.",
                 release_specific_module_dir.c_str());
            module_dirs.clear();
            module_dirs.emplace_back(entry->d_name);
            break;
        }
        // Is a directory does not have page size suffix, it does not mean this directory is for 4K
        // kernels. Certain 16K kernel builds put all modules in /lib/modules/`uname -r` without any
        // suffix. Therefore, only ignore a directory if it has _16k/_64k suffix and the suffix does
        // not match system page size.
        const auto dir_page_size_suffix = GetPageSizeSuffix(entry->d_name);
        if (!dir_page_size_suffix.empty() && dir_page_size_suffix != page_size_suffix) {
            continue;
        }
        int dir_major = 0, dir_minor = 0;
        if (sscanf(entry->d_name, "%d.%d", &dir_major, &dir_minor) != 2 || dir_major != major ||
            dir_minor != minor) {
            continue;
        }
        module_dirs.emplace_back(entry->d_name);
    }

    // Sort the directories so they are iterated over during module loading
    // in a consistent order. Alphabetical sorting is fine here because the
    // kernel version at the beginning of the directory name must match the
    // current kernel version, so the sort only applies to a label that
    // follows the kernel version, for example /lib/modules/5.4 vs.
    // /lib/modules/5.4-gki.
    std::sort(module_dirs.begin(), module_dirs.end());

    for (const auto& module_dir : module_dirs) {
        std::string dir_path = MODULE_BASE_DIR "/";
        dir_path.append(module_dir);
        Modprobe m({dir_path}, GetModuleLoadList(boot_mode, dir_path));
        bool retval = m.LoadListedModules(!want_console);
        modules_loaded = m.GetModuleCount();
        if (modules_loaded > 0) {
            LOGI("Loaded %d modules from %s", modules_loaded, dir_path);
            return retval;
        }
    }

    Modprobe m({MODULE_BASE_DIR}, GetModuleLoadList(boot_mode, MODULE_BASE_DIR));
    bool retval = (want_parallel) ? m.LoadModulesParallel(std::thread::hardware_concurrency())
                                  : m.LoadListedModules(!want_console);
    modules_loaded = m.GetModuleCount();
    if (modules_loaded > 0) {
        LOGI("Loaded %d modules from %s", modules_loaded, MODULE_BASE_DIR);
        return retval;
    }
    return true;
}

static bool IsChargerMode(const std::string& cmdline, const std::string& bootconfig) {
    return bootconfig.find("sysboot.mode = \"charger\"") != std::string::npos ||
           cmdline.find("sysboot.mode=charger") != std::string::npos;
}

static std::string BootModeToString(BootMode mode) {
    switch (mode) {
        case BootMode::CHARGER_MODE:
            return "CHARGER_MODE";
        case BootMode::RECOVERY_MODE:
            return "RECOVERY_MODE";
        case BootMode::NORMAL_MODE:
            return "NORMAL_MODE";
        default:
            return "UNKNOWN_MODE";
    }
}

static BootMode GetBootMode(const std::string& cmdline, const std::string& bootconfig) {
    BootMode mode;

    if (IsChargerMode(cmdline, bootconfig)) {
        mode = BootMode::CHARGER_MODE;
    } else if (IsRecoveryMode() && !ForceNormalBoot(cmdline, bootconfig)) {
        mode = BootMode::RECOVERY_MODE;
    } else {
        mode = BootMode::NORMAL_MODE;
    }

    LOGI("Detected boot mode: %s", BootModeToString(mode).c_str());
    return mode;
}

void MaybeResumeFromHibernation(const std::string& bootconfig) {
    std::string hibernationResumeDevice;
    minimal_systems::fs_mgr::GetBootconfigFromString(
            bootconfig, "sysboot.hibernation_resume_device", &hibernationResumeDevice);

    if (!hibernationResumeDevice.empty()) {
        int fd = open("/sys/power/resume", O_RDWR | O_CLOEXEC);
        if (fd >= 0) {
            if (!WriteStringToFd(hibernationResumeDevice, fd)) {
                LOGE("Failed to write to /sys/power/resume: ");
            }
            close(fd);
        } else {
            LOGE("Failed to open /sys/power/resume:");
        }
    }
}

/*
static std::unique_ptr<FirstStageMount> CreateFirstStageMount(const std::string& cmdline) {
    auto ret = FirstStageMount::Create(cmdline);
    if (ret.ok()) {
        return std::move(*ret);
    } else {
        LOGE("Failed to create first stage mount: %s", ret.error().c_str());
        return nullptr;
    }
}
*/

int FirstStageMain(int argc, char** argv) {
    LOGD("Compiled on %s at %s\n", __DATE__, __TIME__);

    if (REBOOT_BOOTLOADER_ON_PANIC) {
        InstallRebootSignalHandlers();
    }

    std::vector<std::pair<std::string, int>> errors;
#define CHECKCALL(x)                                                                  \
    do {                                                                              \
        int _err = (x);                                                               \
        if (_err != 0) {                                                              \
            LOGE("%s failed with error: %s (errno: %d)", #x, strerror(errno), errno); \
            errors.emplace_back(#x " failed", errno);                                 \
        }                                                                             \
    } while (0)

    umask(0);
    CHECKCALL(clearenv());
    CHECKCALL(setenv("PATH", _PATH_DEFPATH, 1));

    CHECKCALL(mount("tmpfs", "/dev", "tmpfs", MS_NOSUID, "mode=0755"));
    CHECKCALL(mkdir("/dev/pts", 0755));
    CHECKCALL(mkdir("/dev/socket", 0755));
    CHECKCALL(mkdir("/dev/dm-user", 0755));
    CHECKCALL(mount("devpts", "/dev/pts", "devpts", 0, NULL));

#define MAKE_STR(x) __STRING(x)
    CHECKCALL(mount("proc", "/proc", "proc", 0, "hidepid=2,gid=" MAKE_STR(AID_READPROC)));
#undef MAKE_STR

    CHECKCALL(chmod("/proc/cmdline", 0440));
    std::string cmdline;
    minimal_systems::base::ReadFileToString("/proc/cmdline", &cmdline);

    CHECKCALL(chmod("/proc/bootconfig", 0440));
    std::string bootconfig;
    minimal_systems::base::ReadFileToString("/proc/bootconfig", &bootconfig);

    // gid_t groups[] = {AID_READPROC};
    // CHECKCALL(setgroups(sizeof(groups) / sizeof(groups[0]), groups));

    CHECKCALL(mount("sysfs", "/sys", "sysfs", 0, NULL));
    CHECKCALL(mount("selinuxfs", "/sys/fs/selinux", "selinuxfs", 0, NULL));
    CHECKCALL(mknod("/dev/kmsg", S_IFCHR | 0600, makedev(1, 11)));

    if constexpr (WORLD_WRITABLE_KMSG) {
        CHECKCALL(mknod("/dev/kmsg_debug", S_IFCHR | 0622, makedev(1, 11)));
    }

    CHECKCALL(mknod("/dev/random", S_IFCHR | 0666, makedev(1, 8)));
    CHECKCALL(mknod("/dev/urandom", S_IFCHR | 0666, makedev(1, 9)));
    CHECKCALL(mknod("/dev/ptmx", S_IFCHR | 0666, makedev(5, 2)));
    CHECKCALL(mknod("/dev/null", S_IFCHR | 0666, makedev(1, 3)));

    CHECKCALL(mount("tmpfs", "/mnt", "tmpfs", MS_NOEXEC | MS_NOSUID | MS_NODEV,
                    "mode=0755,uid=0,gid=1000"));
    CHECKCALL(mount("tmpfs", "/debug_ramdisk", "tmpfs", MS_NOEXEC | MS_NOSUID | MS_NODEV,
                    "mode=0755,uid=0,gid=0"));
    CHECKCALL(mount("tmpfs", kSecondStageRes, "tmpfs", MS_NOSUID | MS_NODEV,
                    "mode=0755,uid=0,gid=0"));

#undef CHECKCALL

    SetStdioToDevNull(argv);

    InitKernelLogging(argv);

    if (!errors.empty()) {
        for (const auto& [error_string, error_errno] : errors) {
            LOGE("%s %s", error_string.c_str(), strerror(error_errno));
        }
        LOGE("Init encountered errors starting first stage, aborting");
    }

    LOGI("init first stage started!");

    auto old_root_dir = std::unique_ptr<DIR, decltype(&closedir)>{opendir("/"), closedir};
    if (!old_root_dir) {
        LOGE("Could not opendir(\"/\"), not freeing ramdisk");
    }

    struct stat old_root_info {};
    if (stat("/", &old_root_info) != 0) {
        LOGE("Could not stat(\"/\"), not freeing ramdisk");
        old_root_dir.reset();
    }

    GetBootMode(cmdline, GetProperty("ro.bootmode"));

    GetPageSizeSuffix();

    ExtractRootUUID(cmdline);

    DetectAndSetGPUType();
    FreeRamdisk();
    return 0;
}

}  // namespace init
}  // namespace minimal_systems

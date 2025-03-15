#define LOG_TAG "init"

#include <fcntl.h>
#include <linux/mount.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <algorithm>
#include <fstream>
#include <regex>
#include <string>
#include <vector>
#include <filesystem>

#include "log_new.h"
#include "property_manager.h"
#include "util.h"
#include "verify.h"
#include "libbase.h"

using namespace std::literals;
namespace fs = std::filesystem;

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
    static constexpr const char* src_bin = "/init";  // Main init binary
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
    LOGD("_%zuk", page_size/1024);
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

BootMode GetBootMode() {
    if (IsChargerMode()) return BootMode::CHARGER_MODE;
    if (IsRecoveryMode() && !IsNormalBootForced()) return BootMode::RECOVERY_MODE;
    return BootMode::NORMAL_MODE;
}

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
        struct stat fileStat {};
        std::string load_path = dir_path + "/" + module_load_file;
        if (stat(load_path.c_str(), &fileStat)) {
            module_load_file = "modules.load";
        }
    }

    return module_load_file;
}

int FirstStageMain(int argc, char** argv) {
    std::string cmdline = ReadFirstLine(NormalizePath("/proc/cmdline"));
    if (cmdline.empty()) {
        cmdline = ReadFirstLine("./proc/cmdline");
    }

    GetPageSizeSuffix();

    ExtractRootUUID(cmdline);

    DetectAndSetGPUType();
    FreeRamdisk();
    return 0;
}

}  // namespace init
}  // namespace minimal_systems

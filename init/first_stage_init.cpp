#define LOG_TAG "init"

#include <fcntl.h>
#include <linux/mount.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <fstream>
#include <regex>
#include <string>

#include "log_new.h"
#include "property_manager.h"

namespace minimal_systems {
namespace init {

enum class BootMode {
  NORMAL_MODE,
  RECOVERY_MODE,
  CHARGER_MODE,
};

namespace {

std::string GetProperty(const std::string& key) { return getprop(key); }

bool FileExists(const std::string& path) {
  struct stat buffer;
  return stat(path.c_str(), &buffer) == 0;
}

std::string ReadFirstLine(const std::string& path) {
  std::ifstream file(path);
  if (!file.is_open()) {
    return "";
  }
  std::string line;
  std::getline(file, line);
  return line;
}

void DetectAndSetGPUType() {
  if (FileExists("/proc/driver/nvidia/version")) {
    LOGI("GPU: NVIDIA detected");
    setprop("ro.boot.gpu", "nvidia");
  } else if (FileExists("/sys/class/drm/card0/device/vendor")) {
    std::string vendor_id = ReadFirstLine("/sys/class/drm/card0/device/vendor");
    if (vendor_id == "0x1002") {
      LOGI("GPU: AMD detected");
      setprop("ro.boot.gpu", "amd");
    } else {
      LOGW("GPU: Unknown vendor");
      setprop("ro.boot.gpu", "unknown");
    }
  } else {
    LOGW("GPU: None detected");
    setprop("ro.boot.gpu", "none");
  }
}

void FreeRamdisk() {
  if (umount("/dev/ram0") == 0 && unlink("/dev/ram0") == 0) {
    LOGI("Ramdisk successfully freed");
  } else {
    LOGE("Failed to free ramdisk");
  }
}

bool IsChargerMode() { return GetProperty("ro.bootmode") == "charger"; }

bool IsRecoveryMode() { return GetProperty("ro.bootmode") == "recovery"; }

bool IsNormalBootForced() { return GetProperty("ro.bootmode") == "normal"; }

int LoadKernelModule(const std::string& module_path) {
  int fd = open(module_path.c_str(), O_RDONLY | O_CLOEXEC);
  if (fd < 0) {
    LOGE("Failed to open module: %s, error: %s", module_path.c_str(),
         strerror(errno));
    return -1;
  }

  if (syscall(__NR_finit_module, fd, "", 0) != 0) {
    LOGE("Failed to load module %s: %s", module_path.c_str(), strerror(errno));
    close(fd);
    return -1;
  }

  LOGI("Loaded module: %s", module_path.c_str());
  close(fd);
  return 0;
}

constexpr bool kTestingMode =
    true;  // Set to true to skip module loading errors during testing

int LoadKernelModulesFromFile(const std::string& list_path) {
  std::ifstream file(list_path);
  if (!file.is_open()) {
    LOGE("Cannot open module list: %s", list_path.c_str());
    return -1;
  }

  static const std::vector<std::string> kModuleBlocklist = {
      // Testing Modules
      "kunit.ko", "kunit-test.ko", "selftests.ko", "regmap-kunit.ko",
      "soc-utils-test.ko", "platform-test.ko", "input_test.ko",
      "dev_addr_lists_test.ko", "soc-topology-test.ko", "iio-test-format.ko",
      "fat_test.ko", "ext4-inode-test.ko", "time_test.ko", "lib_test.ko",

      // Debugging & Headers
      "kheaders.ko",

      // Redundant or Non-Critical Modules
      "9pnet.ko", "9pnet_fd.ko", "virtio_console.ko", "virtio_balloon.ko",
      "tipc_diag.ko", "tls.ko", "ppp_deflate.ko", "ppp_mppe.ko", "ax88796b.ko",
      "ftdi_sio.ko", "btbcm.ko", "btqca.ko", "btsdio.ko",
      "ieee802154_socket.ko", "r8153_ecm.ko", "acpi_mdio.ko", "pwrseq-core.ko",
      "regmap-ram.ko", "regmap-raw-ram.ko", "ptp_vmclock.ko", "ptp_kvm.ko",
      "macsec.ko", "nhc_hop.ko"};

  std::string module_name;
  int failed_count = 0;

  while (std::getline(file, module_name)) {
    if (module_name.empty()) continue;

    // Skip blocklisted modules
    if (std::find(kModuleBlocklist.begin(), kModuleBlocklist.end(),
                  module_name) != kModuleBlocklist.end()) {
      // LOGW("Skipping blocked module: %s", module_name.c_str());
      continue;
    }

    std::string module_path = "./lib/modules/" + module_name;

    if (kTestingMode) {
      LOGI("Loaded (test mode): %s", module_name.c_str());
      continue;
    }

    if (LoadKernelModule(module_path) != 0) {
      setprop("ro.boot.module_load_error", module_name);
      failed_count++;
      LOGW("Continuing despite failed module: %s", module_name.c_str());
    } else {
      LOGI("Successfully loaded module: %s", module_name.c_str());
    }
  }

  if (failed_count > 0) {
    LOGW("Kernel modules loaded with %d errors.", failed_count);
  } else {
    LOGI("Kernel modules loaded successfully from: %s", list_path.c_str());
  }

  return (kTestingMode || failed_count == 0) ? 0 : -1;
}

}  // namespace

BootMode GetBootMode() {
  if (IsChargerMode()) return BootMode::CHARGER_MODE;
  if (IsRecoveryMode() && !IsNormalBootForced()) return BootMode::RECOVERY_MODE;
  return BootMode::NORMAL_MODE;
}

/**
 * Loads kernel modules from:
 *   1. Custom modules specified via "ro.boot.modules"
 *   2. Main kernel modules from primary or fallback paths
 *
 * Custom modules are loaded first, followed by the main modules.
 *
 * @return 0 on success; -1 on critical failure.
 */
int LoadKernelModules() {
  const std::string primary = "./lib/modules/modules-load.list";
  const std::string fallback = "./firmware/lib/modules/module_load.list";
  const std::string custom_modules_path = GetProperty("ro.boot.modules");
  bool has_errors = false;

  // Load custom modules if specified
  if (!custom_modules_path.empty()) {
    std::string custom_module_list = custom_modules_path + "/modules-load.list";
    if (FileExists(custom_module_list)) {
      LOGI("Loading custom kernel modules from: %s",
           custom_module_list.c_str());
      setprop("ro.boot.module_load_custom", "1");
      if (LoadKernelModulesFromFile(custom_module_list) != 0) {
        LOGW("Errors loading custom kernel modules");
        has_errors = true;
      }
    } else {
      LOGW("Custom module list not found at: %s", custom_module_list.c_str());
    }
  } else {
    LOGI("No custom modules path specified via ro.boot.modules");
  }

  // Load main kernel modules
  if (FileExists(primary)) {
    LOGI("Loading main kernel modules from primary: %s", primary.c_str());
    setprop("ro.boot.module_load_primary", "1");
    if (LoadKernelModulesFromFile(primary) != 0) {
      LOGW("Errors loading main kernel modules from primary path");
      has_errors = true;
    }
  } else if (FileExists(fallback)) {
    LOGI("Loading main kernel modules from fallback: %s", fallback.c_str());
    setprop("ro.boot.module_load_fallback", "1");
    if (LoadKernelModulesFromFile(fallback) != 0) {
      LOGW("Errors loading main kernel modules from fallback path");
      has_errors = true;
    }
  } else {
    LOGW("No main kernel module lists found; skipped loading main modules");
  }

  return has_errors ? -1 : 0;
}

std::string ExtractRootUUID(const std::string& cmdline) {
  std::smatch match;
  if (std::regex_search(cmdline, match,
                        std::regex(R"(root=UUID=([a-fA-F0-9-]+))"))) {
    return match[1];
  }
  return "";
}

int FirstStageMain(int argc, char** argv) {
  std::string cmdline = ReadFirstLine("/proc/cmdline");
  if (cmdline.empty()) {
    cmdline = ReadFirstLine("./proc/cmdline");
  }

  std::string root_uuid = ExtractRootUUID(cmdline);
  if (!root_uuid.empty()) {
    setprop("ro.boot.root_uuid", root_uuid);
  } else {
    LOGW("Root UUID not found");
  }

  if (LoadKernelModules() != 0) {
    LOGE("Kernel module loading failed");
    return -1;
  }

  DetectAndSetGPUType();
  FreeRamdisk();

  return 0;
}

}  // namespace init
}  // namespace minimal_systems

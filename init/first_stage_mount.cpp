// frameworks/base/init/first_stage_mount.cpp

/*
 * Copyright (C) 2025 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * You may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "init"

#include "first_stage_mount.h"

#include <sys/stat.h>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "fs_mgr.h"
#include "log_new.h"
#include "property_manager.h"
#include "verify.h"

namespace minimal_systems {
namespace init {

// Normalize paths by removing duplicate slashes
std::string NormalizePath(const std::string& path) {
  std::string result;
  std::unique_copy(path.begin(), path.end(), std::back_inserter(result),
                   [](char a, char b) { return a == '/' && b == '/'; });
  return result;
}

// Parse and process fstab file
void ParseFstabFile(const std::string& filepath) {
  std::ifstream file(filepath);
  if (!file.is_open()) {
    LOGE(
        "Error: Unable to open fstab file at '%s'. Please verify file path and "
        "permissions.",
        filepath.c_str());
    return;
  }

  LOGI("Parsing fstab file: '%s'", filepath.c_str());
  std::string line;
  int line_number = 0;

  while (std::getline(file, line)) {
    ++line_number;
    line.erase(0, line.find_first_not_of(" \t"));  // Trim leading whitespace

    if (line.empty() || line[0] == '#')
      continue;  // Skip empty lines and comments

    std::istringstream iss(line);
    std::string device, mount_point, filesystem, options;

    if (!(iss >> device >> mount_point >> filesystem >> options)) {
      LOGE(
          "Error: Malformed fstab entry on line %d in '%s'. Skipping this "
          "entry.",
          line_number, filepath.c_str());
      continue;
    }

    // OverlayFS Handling (no verification needed)
    if (filesystem == "overlay") {
      LOGI("OverlayFS detected at mount point '%s'. Attempting to mount.",
           mount_point.c_str());

      if (!minimal_systems::fs_mgr::MountOverlayFs(mount_point)) {
        LOGE("Critical: Overlay mount failed at '%s' (line %d). Aborting.",
             mount_point.c_str(), line_number);
        std::exit(-1);
      }

      LOGI("Successfully mounted overlay at '%s'.", mount_point.c_str());
      continue;
    }

    // Ensure that only partitions with the "verify" flag are mounted
    if (options.find("verify") == std::string::npos) {
      continue;
    }

    // **Step 1: Verify the partition before mounting**
    if (!VerifyPartition(device, mount_point, filesystem)) {
      LOGE(
          "Critical: Partition verification failed for '%s'. Boot process "
          "halted.",
          device.c_str());
      std::exit(-1);
    }

    LOGI("Partition verification successful for '%s'. Proceeding with mount.",
         device.c_str());

    // **Step 2: Attempt to mount the verified partition**
    LOGI(
        "Attempting to mount device '%s' at '%s' with filesystem type '%s' and "
        "options: '%s'.",
        device.c_str(), mount_point.c_str(), filesystem.c_str(),
        options.c_str());

    if (!minimal_systems::fs_mgr::MountPartition(device, mount_point,
                                                 filesystem, options)) {
      LOGE(
          "Critical: Mount operation failed for '%s' (line %d) at '%s'. System "
          "will halt.",
          device.c_str(), line_number, mount_point.c_str());
      std::exit(-1);
    }

    LOGI("Mount operation completed successfully for '%s' at '%s'.",
         device.c_str(), mount_point.c_str());
  }

  LOGI("Completed parsing of fstab file: '%s'.", filepath.c_str());
}

// Load fstab file from multiple paths, applying normalization if needed
bool LoadFstab(const std::vector<std::string>& fstab_paths) {
  auto& props = PropertyManager::instance();
  std::string hardware = getprop("ro.boot.hardware");

  for (const auto& path : fstab_paths) {
    std::ifstream file(path);
    if (file) {
      LOGI("Fstab file found at '%s' for hardware '%s' without normalization.",
           path.c_str(), hardware.c_str());
      ParseFstabFile(path);
      return true;
    }

    std::string normalized_path = NormalizePath(path);
    file.open(normalized_path);
    if (file) {
      LOGI("Fstab file found at '%s' for hardware '%s' after normalization.",
           normalized_path.c_str(), hardware.c_str());
      ParseFstabFile(normalized_path);
      return true;
    }

    LOGW("Warning: No fstab file found at '%s'. Searching in other locations.",
         path.c_str());
  }

  std::ifstream fallback_file("/etc/fstab");
  if (fallback_file) {
    LOGI("Using fallback fstab file '/etc/fstab' for hardware '%s'.",
         hardware.c_str());
    ParseFstabFile("/etc/fstab");
    return true;
  }

  LOGE(
      "Fatal Error: No valid fstab found. Device will reboot into bootloader.");
  std::exit(-1);
}

// First-stage mount process
bool FirstStageMount() {
  auto& props = PropertyManager::instance();
  std::string boot_mode = getprop("ro.bootmode");

  const std::vector<std::string> fstab_paths = {
      "etc/fstab", "usr/share/etc/fstab", "/etc/fstab"};

  if (boot_mode == "recovery") {
    LOGI("Boot mode: 'recovery' detected. Loading recovery fstab.");
    return LoadFstab(fstab_paths);
  } else {
    LOGI("Boot mode: 'normal' detected. Loading normal fstab.");
    return LoadFstab(fstab_paths);
  }
}

}  // namespace init
}  // namespace minimal_systems

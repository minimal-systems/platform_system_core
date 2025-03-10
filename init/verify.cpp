// frameworks/base/init/verify.cpp

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

#define LOG_TAG "verify"

#include "verify.h"

#include <openssl/evp.h>  // OpenSSL EVP API

#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "log_new.h"

namespace minimal_systems {
namespace init {

// Compute SHA256 hash of a given file (typically a partition device)
std::string ComputeSHA256(const std::string& filepath) {
  std::ifstream file(filepath, std::ios::binary);
  if (!file.is_open()) {
    LOGE("Error: Unable to open file '%s' for hash computation.",
         filepath.c_str());
    return "";
  }

  EVP_MD_CTX* ctx = EVP_MD_CTX_new();
  if (!ctx) {
    LOGE("Error: Failed to initialize EVP_MD_CTX.");
    return "";
  }

  const EVP_MD* sha256 = EVP_sha256();
  if (!EVP_DigestInit_ex(ctx, sha256, nullptr)) {
    LOGE("Error: EVP_DigestInit_ex failed for SHA256.");
    EVP_MD_CTX_free(ctx);
    return "";
  }

  char buffer[4096];
  while (file.read(buffer, sizeof(buffer)) || file.gcount() > 0) {
    if (!EVP_DigestUpdate(ctx, buffer, file.gcount())) {
      LOGE("Error: EVP_DigestUpdate failed.");
      EVP_MD_CTX_free(ctx);
      return "";
    }
  }

  unsigned char hash[EVP_MAX_MD_SIZE];
  unsigned int hash_len = 0;
  if (!EVP_DigestFinal_ex(ctx, hash, &hash_len)) {
    LOGE("Error: EVP_DigestFinal_ex failed.");
    EVP_MD_CTX_free(ctx);
    return "";
  }

  EVP_MD_CTX_free(ctx);

  // Convert hash to hex string
  std::ostringstream oss;
  oss << std::hex << std::setw(2) << std::setfill('0');
  for (unsigned int i = 0; i < hash_len; ++i) {
    oss << std::setw(2) << static_cast<int>(hash[i]);
  }

  std::string hash_str = oss.str();
  LOGI("SHA256 hash for '%s': %s", filepath.c_str(), hash_str.c_str());

  return hash_str;
}

// Simulated signature verification
bool VerifySignature(const std::string& hash,
                     const std::string& public_key_path) {
  if (hash.empty()) {
    LOGE("Error: Empty hash provided; signature verification aborted.");
    return false;
  }

  LOGI("Verifying signature for hash: '%s' using public key: '%s'.",
       hash.c_str(), public_key_path.c_str());

  // TODO: Replace with actual signature verification (e.g., RSA or ECDSA)
  bool simulated_result =
      true;  // Simulating a successful signature verification

  if (!simulated_result) {
    LOGE("Signature verification failed for hash: '%s'.", hash.c_str());
    return false;
  }

  LOGI("Signature verification passed for hash: '%s'.", hash.c_str());
  return true;
}

// Perform partition verification (similar to AVB checks)
bool VerifyPartition(const std::string& device, const std::string& mount_point,
                     const std::string& filesystem) {
  LOGI(
      "Starting partition verification: Device='%s', Mount Point='%s', "
      "Filesystem='%s'",
      device.c_str(), mount_point.c_str(), filesystem.c_str());

  // Step 1: Ensure the partition device exists
  std::ifstream device_file(device);
  if (!device_file.is_open()) {
    LOGW(
        "Warning: Device '%s' is not accessible or does not exist. Skipping "
        "verification.",
        device.c_str());
    return true;  // Allow boot to continue even if device is missing
  }
  LOGI("Device '%s' is accessible.", device.c_str());

  // Step 2: Compute SHA256 hash of the partition
  LOGI("Computing SHA256 hash for partition '%s'.", device.c_str());
  std::string computed_hash = ComputeSHA256(device);
  if (computed_hash.empty()) {
    LOGE(
        "Critical: Hash computation failed for partition '%s'. Verification "
        "aborted.",
        device.c_str());
    return false;  // Stop boot process if hash fails
  }

  // Step 3: Verify hash against stored signature
  std::string public_key_path = "/etc/keys/public_key.pem";
  if (!VerifySignature(computed_hash, public_key_path)) {
    LOGE(
        "Critical: Signature verification failed for partition '%s'. Boot "
        "process halted.",
        device.c_str());
    return false;
  }

  LOGI("Partition verification completed successfully for device '%s'.",
       device.c_str());
  return true;
}

}  // namespace init
}  // namespace minimal_systems

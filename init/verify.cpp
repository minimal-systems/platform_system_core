// frameworks/base/init/verify.cpp

/*
 * Copyright (C) 2025 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
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

#include <openssl/evp.h>

#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "log_new.h"
#include "property_manager.h"

namespace minimal_systems
{
namespace init
{

// Fetch secure boot verification properties
std::string GetSecureBootSHA()
{
	return getprop("ro.sysboot.secureboot_sha");
}

std::string GetVerityKeyPath()
{
	return getprop("ro.sysboot.secure_verity_key_path");
}

// Compute SHA256 hash of a given file (typically a partition device)
std::string ComputeSHA256(const std::string &filepath)
{
	std::ifstream file(filepath, std::ios::binary);
	if (!file.is_open()) {
		LOGE("Error: Unable to open file '%s' for SHA256 computation.",
		     filepath.c_str());
		return "";
	}

	EVP_MD_CTX *ctx = EVP_MD_CTX_new();
	if (!ctx) {
		LOGE("Error: Failed to initialize EVP_MD_CTX.");
		return "";
	}

	const EVP_MD *sha256 = EVP_sha256();
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

// Verify computed hash against expected Secure Boot SHA
bool VerifySecureBootSHA(const std::string &computed_hash)
{
	std::string expected_sha = GetSecureBootSHA();

	if (expected_sha.empty()) {
		LOGE("Error: Secure Boot SHA property is not set.");
		return false;
	}

	if (computed_hash != expected_sha) {
		LOGE("Secure Boot SHA mismatch! Computed: '%s', Expected: '%s'.",
		     computed_hash.c_str(), expected_sha.c_str());
		return false;
	}

	LOGI("Secure Boot SHA verification passed.");
	return true;
}

// Signature verification using public key
bool VerifySignature(const std::string &hash)
{
	std::string public_key_path = GetVerityKeyPath();

	if (public_key_path.empty()) {
		LOGE("Error: Secure Verity Key Path property is not set.");
		return false;
	}

	LOGI("Verifying signature for hash: '%s' using public key: '%s'.",
	     hash.c_str(), public_key_path.c_str());

	// TODO: Replace with actual signature verification (e.g., RSA or ECDSA)
	bool simulated_result =
		true; // Simulating a successful signature verification

	if (!simulated_result) {
		LOGE("Signature verification failed for hash: '%s'.",
		     hash.c_str());
		return false;
	}

	LOGI("Signature verification passed for hash: '%s'.", hash.c_str());
	return true;
}

// Perform partition verification (similar to AVB checks)
bool VerifyPartition(const std::string &device, const std::string &mount_point,
		     const std::string &filesystem)
{
	LOGI("Starting partition verification: Device='%s', Mount Point='%s', Filesystem='%s'",
	     device.c_str(), mount_point.c_str(), filesystem.c_str());

	// Step 1: Ensure the partition device exists
	std::ifstream device_file(device);
	if (!device_file.is_open()) {
		LOGW("Warning: Device '%s' is not accessible or does not exist. Skipping verification.",
		     device.c_str());
		return true; // Allow boot to continue even if device is missing
	}
	LOGI("Device '%s' is accessible.", device.c_str());

	// Step 2: Compute SHA256 hash of the partition
	LOGI("Computing SHA256 hash for partition '%s'.", device.c_str());
	std::string computed_hash = ComputeSHA256(device);
	if (computed_hash.empty()) {
		LOGE("Critical: Hash computation failed for partition '%s'. Verification aborted.",
		     device.c_str());
		return false; // Stop boot process if hash fails
	}

	// Step 3: Verify hash against Secure Boot SHA
	if (!VerifySecureBootSHA(computed_hash)) {
		LOGE("Critical: Secure Boot SHA verification failed for partition '%s'. Boot process halted.",
		     device.c_str());
		return false;
	}

	// Step 4: Verify hash against stored signature
	if (!VerifySignature(computed_hash)) {
		LOGE("Critical: Signature verification failed for partition '%s'. Boot process halted.",
		     device.c_str());
		return false;
	}

	LOGI("Partition verification completed successfully for device '%s'.",
	     device.c_str());
	return true;
}

} // namespace init
} // namespace minimal_systems

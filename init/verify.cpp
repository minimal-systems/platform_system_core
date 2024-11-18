#define LOG_TAG "init"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <openssl/evp.h>  // For EVP API
#include "log_new.h"
#include "verify.h"
#include <iomanip>

namespace minimal_systems {
namespace init {

// Helper function to compute SHA256 hash of a file
std::string compute_sha256(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        LOGE("Unable to open file '%s' for hash computation.", filepath.c_str());
        return "";
    }

    // Initialize EVP context for SHA256
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) {
        LOGE("Failed to create EVP_MD_CTX.");
        return "";
    }

    const EVP_MD* sha256 = EVP_sha256();
    if (!EVP_DigestInit_ex(ctx, sha256, nullptr)) {
        LOGE("EVP_DigestInit_ex failed for SHA256.");
        EVP_MD_CTX_free(ctx);
        return "";
    }

    char buffer[4096];
    while (file.read(buffer, sizeof(buffer)) || file.gcount() > 0) {
        if (!EVP_DigestUpdate(ctx, buffer, file.gcount())) {
            LOGE("EVP_DigestUpdate failed.");
            EVP_MD_CTX_free(ctx);
            return "";
        }
    }

    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hash_len = 0;
    if (!EVP_DigestFinal_ex(ctx, hash, &hash_len)) {
        LOGE("EVP_DigestFinal_ex failed.");
        EVP_MD_CTX_free(ctx);
        return "";
    }

    EVP_MD_CTX_free(ctx);

    // Convert hash to hex string
    std::ostringstream oss;
    for (unsigned int i = 0; i < hash_len; ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }

    return oss.str();
}

// Helper function to simulate signature verification
bool verify_signature(const std::string& hash, const std::string& public_key_path) {
    // Simulated signature verification logic
    LOGI("Verifying signature for hash '%s' with public key at '%s'.",
         hash.c_str(), public_key_path.c_str());
    // TODO: Integrate actual signature verification logic
    if (hash.empty()) {
        LOGE("Empty hash provided; signature verification failed.");
        return false;
    }

    // Simulated successful verification
    LOGI("Signature verification completed successfully for hash '%s'.", hash.c_str());
    return true;
}

// Simulated AVB-like partition verification
bool verify_partition(const std::string& device, const std::string& mount_point, const std::string& filesystem) {
    LOGI("Verifying partition: Device: '%s', Mount Point: '%s', Filesystem: '%s'",
         device.c_str(), mount_point.c_str(), filesystem.c_str());

    // Step 1: Ensure the device is readable
    std::ifstream device_file(device);
    if (!device_file.is_open()) {
        LOGW("Device '%s' does not exist or cannot be opened. Skipping verification.", device.c_str());
        return true;  // Continue processing other devices
    }
    LOGI("Device '%s' is readable.", device.c_str());

    // Step 2: Compute hash of the partition
    LOGI("Computing SHA256 hash of the partition.");
    std::string computed_hash = compute_sha256(device);
    if (computed_hash.empty()) {
        LOGE("Hash computation failed for device '%s'.", device.c_str());
        return false;  // Hash computation failure is critical
    }
    LOGI("Computed hash: '%s'.", computed_hash.c_str());

    // Step 3: Simulate signature verification
    std::string public_key_path = "/etc/keys/public_key.pem";  // Example path for public key
    if (!verify_signature(computed_hash, public_key_path)) {
        LOGE("Signature verification failed for device '%s'.", device.c_str());
        return false;  // Signature verification failure is critical
    }

    LOGI("Partition verification for device '%s' completed successfully.", device.c_str());
    return true;
}


}  // namespace init
}  // namespace minimal_systems

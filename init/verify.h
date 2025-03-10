// frameworks/base/init/verify.h

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

#pragma once

#include <string>

namespace minimal_systems {
namespace init {

/**
 * Computes the SHA256 hash of a given file.
 *
 * @param filepath Path to the file (e.g., partition device).
 * @return Hexadecimal representation of the SHA256 hash.
 */
std::string ComputeSHA256(const std::string& filepath);

/**
 * Verifies the digital signature of a hash using a public key.
 *
 * @param hash The computed hash of the file/partition.
 * @param public_key_path Path to the public key used for verification.
 * @return `true` if verification succeeds, `false` otherwise.
 */
bool VerifySignature(const std::string& hash,
                     const std::string& public_key_path);

/**
 * Verifies the integrity and authenticity of a partition before mounting.
 *
 * @param device Path to the partition device (e.g., `/dev/block/...`).
 * @param mount_point Where the partition will be mounted.
 * @param filesystem Filesystem type (e.g., `ext4`, `f2fs`, `vfat`).
 * @return `true` if verification passes, `false` if verification fails.
 */
bool VerifyPartition(const std::string& device, const std::string& mount_point,
                     const std::string& filesystem);

}  // namespace init
}  // namespace minimal_systems

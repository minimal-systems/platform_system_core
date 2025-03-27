/*
 * Copyright (C) 2024 The Android Open Source Project
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

#include <sys/types.h>
#include <string>
#include <unordered_map>

/**
 * @brief Runs an external binary with specified UID, GID, and environment.
 *
 * This function forks the current process, switches to the provided UID/GID,
 * sets environment variables from the provided map, redirects output pipes,
 * and executes the specified command via `execv`.
 *
 * Captures and returns trimmed stdout if the child exits successfully.
 * Logs any stderr output from the handler. If the process fails (fork, exec, crash),
 * an empty string is returned.
 *
 * @param handler The command line to execute (e.g., "/bin/script --flag").
 * @param uid The UID to set before executing the command.
 * @param gid The GID to set before executing the command.
 * @param envs_map A map of environment variable key-value pairs.
 * @return Captured stdout output if successful, or empty string on failure.
 */
std::string RunExternalHandler(const std::string& handler, uid_t uid, gid_t gid,
                               std::unordered_map<std::string, std::string>& envs_map);

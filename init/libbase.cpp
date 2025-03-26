/*
 * Copyright (C) 2011 The Android Open Source Project
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

#include "libbase.h"

#include <stdio.h>
#include <unistd.h>
#include <climits>  // for INT_MAX
#include <fstream>
#include <regex>
#include <sstream>
#include <string>

namespace minimal_systems {
namespace base {

void StringAppendV(std::string* dst, const char* format, va_list ap) {
    // First try with a small fixed size buffer
    char space[1024] __attribute__((__uninitialized__));

    // Copy the va_list before first use
    va_list backup_ap;
    va_copy(backup_ap, ap);
    int result = vsnprintf(space, sizeof(space), format, backup_ap);
    va_end(backup_ap);

    if (result >= 0 && result < static_cast<int>(sizeof(space))) {
        if (result > 0) {
            // Normal case -- everything fit.
            dst->append(space, static_cast<std::string::size_type>(result));
        }
        return;
    }

    if (result < 0) {
        // Just an error.
        return;
    }

    // Increase buffer size to accommodate result + null terminator
    size_t length = static_cast<size_t>(result) + 1;
    char* buf = new char[length];

    // Restore the va_list before second use
    va_copy(backup_ap, ap);
    int safe_len = static_cast<int>(std::min(length, static_cast<size_t>(INT_MAX)));
    result = vsnprintf(buf, safe_len, format, backup_ap);
    va_end(backup_ap);

    if (result >= 0 && static_cast<size_t>(result) < length) {
        // It fit
        if (result > 0) {
            dst->append(buf, static_cast<std::string::size_type>(result));
        }
    }
    delete[] buf;
}

std::string StringPrintf(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    std::string result;
    StringAppendV(&result, fmt, ap);
    va_end(ap);
    return result;
}

void StringAppendF(std::string* dst, const char* format, ...) {
    va_list ap;
    va_start(ap, format);
    StringAppendV(dst, format, ap);
    va_end(ap);
}

bool ReadFileToString(const std::string& path, std::string* content) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    *content = buffer.str();
    return true;
}

/**
 * Cleans a raw command line string:
 *  - Strips everything after '#' (comments)
 *  - Collapses whitespace to a single space
 *  - Trims leading/trailing whitespace
 */
std::string CleanCmdline(const std::string& input) {
    std::stringstream ss(input);
    std::string line;
    std::string result;

    while (std::getline(ss, line)) {
        // Trim leading/trailing whitespace
        line = std::regex_replace(line, std::regex(R"(^\s+|\s+$)"), "");

        // Skip empty or full-line comments
        if (line.empty() || line[0] == '#') continue;

        // Strip inline comment
        size_t comment_pos = line.find('#');
        if (comment_pos != std::string::npos) {
            line = line.substr(0, comment_pos);
            line = std::regex_replace(line, std::regex(R"(^\s+|\s+$)"), "");
        }

        // Collapse inner whitespace
        line = std::regex_replace(line, std::regex(R"([\s]+)"), " ");

        if (!line.empty()) {
            if (!result.empty()) result += ' ';
            result += line;
        }
    }

    return result;
}

/**
 * Appends the cleaned contents of `./.cmdline` to `merged_cmdline` if present.
 */
void AppendLocalCmdline(std::string* merged_cmdline) {
    std::string local_cmdline;
    if (!ReadFileToString("./.cmdline", &local_cmdline)) {
        return;
    }

    local_cmdline = CleanCmdline(local_cmdline);
    if (local_cmdline.empty()) {
        return;
    }

    if (!merged_cmdline->empty() && merged_cmdline->back() != ' ') {
        *merged_cmdline += ' ';
    }

    *merged_cmdline += local_cmdline;
}

}  // namespace base
}  // namespace minimal_systems

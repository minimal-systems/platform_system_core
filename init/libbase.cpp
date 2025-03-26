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
#include <fstream>
#include <sstream>
#include <string>
#include <regex>
#include <string>
#include <unistd.h>


namespace minimal_systems {
namespace base {

void StringAppendV(std::string* dst, const char* format, va_list ap) {
    // First try with a small fixed size buffer
    char space[1024] __attribute__((__uninitialized__));

    // It's possible for methods that use a va_list to invalidate
    // the data in it upon use.  The fix is to make a copy
    // of the structure before using it and use that copy instead.
    va_list backup_ap;
    va_copy(backup_ap, ap);
    int result = vsnprintf(space, sizeof(space), format, backup_ap);
    va_end(backup_ap);

    if (result < static_cast<int>(sizeof(space))) {
        if (result >= 0) {
            // Normal case -- everything fit.
            dst->append(space, result);
            return;
        }

        if (result < 0) {
            // Just an error.
            return;
        }
    }

    // Increase the buffer size to the size requested by vsnprintf,
    // plus one for the closing \0.
    int length = result + 1;
    char* buf = new char[length];

    // Restore the va_list before we use it again
    va_copy(backup_ap, ap);
    result = vsnprintf(buf, length, format, backup_ap);
    va_end(backup_ap);

    if (result >= 0 && result < length) {
        // It fit
        dst->append(buf, result);
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

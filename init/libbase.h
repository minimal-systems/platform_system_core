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

#pragma once

#include <stdarg.h>
#include <string>

#define AID_READPROC 3006
#define WORLD_WRITABLE_KMSG 1
#define _PATH_DEFPATH "/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin"

namespace minimal_systems {
namespace base {

// These printf-like functions are implemented in terms of vsnprintf, so they
// use the same attribute for compile-time format string checking.

// Returns a string corresponding to printf-like formatting of the arguments.
std::string StringPrintf(const char* fmt, ...) __attribute__((__format__(__printf__, 1, 2)));

// Appends a printf-like formatting of the arguments to 'dst'.
void StringAppendF(std::string* dst, const char* fmt, ...)
        __attribute__((__format__(__printf__, 2, 3)));

// Appends a printf-like formatting of the arguments to 'dst'.
void StringAppendV(std::string* dst, const char* format, va_list ap)
        __attribute__((__format__(__printf__, 2, 0)));

// Reads the contents of a file into a std::string.
// Returns true on success, false on failure.
bool ReadFileToString(const std::string& path, std::string* content);

/**
 * Cleans a raw command line string:
 * - Strips comments starting with '#'
 * - Normalizes all whitespace to single spaces
 * - Trims leading/trailing spaces
 */
std::string CleanCmdline(const std::string& input);

/**
 * Appends the cleaned contents of `./.cmdline` to `merged_cmdline` if present.
 */
void AppendLocalCmdline(std::string* merged_cmdline);

}  // namespace base
}  // namespace minimal_systems

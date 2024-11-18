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

#include <sys/cdefs.h>

#include "log.h"

__BEGIN_DECLS

int __linux_log_print(int prio, const char* tag, const char* fmt, ...)
    __attribute__((__format__(printf, 3, 4)));

/*
 * #define LOG_TAG before including this header to control the log tag used by
 * the macros below. If not defined, the global tag will be used.
 */
#ifndef LOG_TAG
#define LOG_TAG NULL
#endif

#define LOGE(fmt, ...) \
  ((void)__linux_log_print(LINUX_LOG_ERROR, (LOG_TAG), (fmt)__VA_OPT__(, ) __VA_ARGS__))
#define LOGW(fmt, ...) \
  ((void)__linux_log_print(LINUX_LOG_WARN, (LOG_TAG), (fmt)__VA_OPT__(, ) __VA_ARGS__))
#define LOGI(fmt, ...) \
  ((void)__linux_log_print(LINUX_LOG_INFO, (LOG_TAG), (fmt)__VA_OPT__(, ) __VA_ARGS__))
#define LOGD(fmt, ...) \
  ((void)__linux_log_print(LINUX_LOG_DEBUG, (LOG_TAG), (fmt)__VA_OPT__(, ) __VA_ARGS__))
#define LOGV(fmt, ...) \
  ((void)__linux_log_print(LINUX_LOG_VERBOSE, (LOG_TAG), (fmt)__VA_OPT__(, ) __VA_ARGS__))

__END_DECLS
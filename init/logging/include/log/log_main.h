/*
 * Copyright (C) 2005-2017 The Android Open Source Project
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

#include <stdbool.h>
#include <sys/cdefs.h>
#include <sys/types.h>

#include <minimal_systems/log.h>

__BEGIN_DECLS

/*
 * Normally we strip the effects of LOGV (VERBOSE messages),
 * LOG_FATAL and LOG_FATAL_IF (FATAL assert messages) from the
 * release builds by defining NDEBUG.  You can modify this (for
 * example with "#define LOG_NDEBUG 0" at the top of your source
 * file) to change that behavior.
 */

#ifndef LOG_NDEBUG
#ifdef NDEBUG
#define LOG_NDEBUG 1
#else
#define LOG_NDEBUG 0
#endif
#endif

/* --------------------------------------------------------------------- */

/*
 * This file uses ", ## __VA_ARGS__" zero-argument token pasting to
 * work around issues with debug-only syntax errors in assertions
 * that are missing format strings.  See commit
 * 19299904343daf191267564fe32e6cd5c165cd42
 */
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#endif

/*
 * Use __VA_ARGS__ if running a static analyzer,
 * to avoid warnings of unused variables in __VA_ARGS__.
 * Use constexpr function in C++ mode, so these macros can be used
 * in other constexpr functions without warning.
 */
#ifdef __clang_analyzer__
#ifdef __cplusplus
extern "C++" {
template <typename... Ts>
constexpr int __fake_use_va_args(Ts...) {
  return 0;
}
}
#else
extern int __fake_use_va_args(int, ...);
#endif /* __cplusplus */
#define __FAKE_USE_VA_ARGS(...) ((void)__fake_use_va_args(0, ##__VA_ARGS__))
#else
#define __FAKE_USE_VA_ARGS(...) ((void)(0))
#endif /* __clang_analyzer__ */

#ifndef __predict_false
#define __predict_false(exp) __builtin_expect((exp) != 0, 0)
#endif

#define linux_writeLog(prio, tag, text) __linux_log_write(prio, tag, text)

#define linux_printLog(prio, tag, ...) \
  __linux_log_print(prio, tag, __VA_ARGS__)

#define linux_vprintLog(prio, cond, tag, ...) \
  __linux_log_vprint(prio, tag, __VA_ARGS__)

/*
 * Log macro that allows you to specify a number for the priority.
 */
#ifndef LOG_PRI
#define LOG_PRI(priority, tag, ...) linux_printLog(priority, tag, __VA_ARGS__)
#endif

/*
 * Log macro that allows you to pass in a varargs ("args" is a va_list).
 */
#ifndef LOG_PRI_VA
#define LOG_PRI_VA(priority, tag, fmt, args) \
  linux_vprintLog(priority, NULL, tag, fmt, args)
#endif

/* --------------------------------------------------------------------- */

/* XXX Macros to work around syntax errors in places where format string
 * arg is not passed to LOG_ASSERT, LOG_ALWAYS_FATAL or LOG_ALWAYS_FATAL_IF
 * (happens only in debug builds).
 */

/* Returns 2nd arg.  Used to substitute default value if caller's vararg list
 * is empty.
 */
#define __linux_second(dummy, second, ...) second

/* If passed multiple args, returns ',' followed by all but 1st arg, otherwise
 * returns nothing.
 */
#define __linux_rest(first, ...) , ##__VA_ARGS__

#define linux_printAssert(cond, tag, ...)                     \
  __linux_log_assert(cond, tag,                               \
                       __linux_second(0, ##__VA_ARGS__, NULL) \
                           __linux_rest(__VA_ARGS__))

/*
 * Log a fatal error.  If the given condition fails, this stops program
 * execution like a normal assertion, but also generating the given message.
 * It is NOT stripped from release builds.  Note that the condition test
 * is -inverted- from the normal assert() semantics.
 */
#ifndef LOG_ALWAYS_FATAL_IF
#define LOG_ALWAYS_FATAL_IF(cond, ...)                                                    \
  ((__predict_false(cond)) ? (__FAKE_USE_VA_ARGS(__VA_ARGS__),                            \
                              ((void)linux_printAssert(#cond, LOG_TAG, ##__VA_ARGS__))) \
                           : ((void)0))
#endif

#ifndef LOG_ALWAYS_FATAL
#define LOG_ALWAYS_FATAL(...) \
  (((void)linux_printAssert(NULL, LOG_TAG, ##__VA_ARGS__)))
#endif

/*
 * Versions of LOG_ALWAYS_FATAL_IF and LOG_ALWAYS_FATAL that
 * are stripped out of release builds.
 */

#if LOG_NDEBUG

#ifndef LOG_FATAL_IF
#define LOG_FATAL_IF(cond, ...) __FAKE_USE_VA_ARGS(__VA_ARGS__)
#endif
#ifndef LOG_FATAL
#define LOG_FATAL(...) __FAKE_USE_VA_ARGS(__VA_ARGS__)
#endif

#else

#ifndef LOG_FATAL_IF
#define LOG_FATAL_IF(cond, ...) LOG_ALWAYS_FATAL_IF(cond, ##__VA_ARGS__)
#endif
#ifndef LOG_FATAL
#define LOG_FATAL(...) LOG_ALWAYS_FATAL(__VA_ARGS__)
#endif

#endif

/*
 * Assertion that generates a log message when the assertion fails.
 * Stripped out of release builds.  Uses the current LOG_TAG.
 */
#ifndef LOG_ASSERT
#define LOG_ASSERT(cond, ...) LOG_FATAL_IF(!(cond), ##__VA_ARGS__)
#endif

/* --------------------------------------------------------------------- */

/*
 * C/C++ logging functions.  See the logging documentation for API details.
 *
 * We'd like these to be available from C code (in case we import some from
 * somewhere), so this has a C interface.
 *
 * The output will be correct when the log file is shared between multiple
 * threads and/or multiple processes so long as the operating system
 * supports O_APPEND.  These calls have mutex-protected data structures
 * and so are NOT reentrant.  Do not use LOG in a signal handler.
 */

/* --------------------------------------------------------------------- */

/*
 * Simplified macro to send a verbose log message using the current LOG_TAG.
 */
#ifndef LOGV
#define __LOGV(...) ((void)LOG(LOG_VERBOSE, LOG_TAG, __VA_ARGS__))
#if LOG_NDEBUG
#define LOGV(...)                   \
  do {                               \
    __FAKE_USE_VA_ARGS(__VA_ARGS__); \
    if (false) {                     \
      __LOGV(__VA_ARGS__);          \
    }                                \
  } while (false)
#else
#define LOGV(...) __LOGV(__VA_ARGS__)
#endif
#endif

#ifndef LOGV_IF
#if LOG_NDEBUG
#define LOGV_IF(cond, ...) __FAKE_USE_VA_ARGS(__VA_ARGS__)
#else
#define LOGV_IF(cond, ...)                                                               \
  ((__predict_false(cond))                                                                \
       ? (__FAKE_USE_VA_ARGS(__VA_ARGS__), (void)LOG(LOG_VERBOSE, LOG_TAG, __VA_ARGS__)) \
       : ((void)0))
#endif
#endif

/*
 * Simplified macro to send a debug log message using the current LOG_TAG.
 */
#ifndef LOGD
#define LOGD(...) ((void)LOG(LOG_DEBUG, LOG_TAG, __VA_ARGS__))
#endif

#ifndef LOGD_IF
#define LOGD_IF(cond, ...)                                                             \
  ((__predict_false(cond))                                                              \
       ? (__FAKE_USE_VA_ARGS(__VA_ARGS__), (void)LOG(LOG_DEBUG, LOG_TAG, __VA_ARGS__)) \
       : ((void)0))
#endif

/*
 * Simplified macro to send an info log message using the current LOG_TAG.
 */
#ifndef LOGI
#define LOGI(...) ((void)LOG(LOG_INFO, LOG_TAG, __VA_ARGS__))
#endif

#ifndef LOGI_IF
#define LOGI_IF(cond, ...)                                                            \
  ((__predict_false(cond))                                                             \
       ? (__FAKE_USE_VA_ARGS(__VA_ARGS__), (void)LOG(LOG_INFO, LOG_TAG, __VA_ARGS__)) \
       : ((void)0))
#endif

/*
 * Simplified macro to send a warning log message using the current LOG_TAG.
 */
#ifndef LOGW
#define LOGW(...) ((void)LOG(LOG_WARN, LOG_TAG, __VA_ARGS__))
#endif

#ifndef LOGW_IF
#define LOGW_IF(cond, ...)                                                            \
  ((__predict_false(cond))                                                             \
       ? (__FAKE_USE_VA_ARGS(__VA_ARGS__), (void)LOG(LOG_WARN, LOG_TAG, __VA_ARGS__)) \
       : ((void)0))
#endif

/*
 * Simplified macro to send an error log message using the current LOG_TAG.
 */
#ifndef LOGE
#define LOGE(...) ((void)LOG(LOG_ERROR, LOG_TAG, __VA_ARGS__))
#endif

#ifndef LOGE_IF
#define LOGE_IF(cond, ...)                                                             \
  ((__predict_false(cond))                                                              \
       ? (__FAKE_USE_VA_ARGS(__VA_ARGS__), (void)LOG(LOG_ERROR, LOG_TAG, __VA_ARGS__)) \
       : ((void)0))
#endif

/* --------------------------------------------------------------------- */

/*
 * Conditional based on whether the current LOG_TAG is enabled at
 * verbose priority.
 */
#ifndef IF_LOGV
#if LOG_NDEBUG
#define IF_LOGV() if (false)
#else
#define IF_LOGV() IF_LOG(LOG_VERBOSE, LOG_TAG)
#endif
#endif

/*
 * Conditional based on whether the current LOG_TAG is enabled at
 * debug priority.
 */
#ifndef IF_LOGD
#define IF_LOGD() IF_LOG(LOG_DEBUG, LOG_TAG)
#endif

/*
 * Conditional based on whether the current LOG_TAG is enabled at
 * info priority.
 */
#ifndef IF_LOGI
#define IF_LOGI() IF_LOG(LOG_INFO, LOG_TAG)
#endif

/*
 * Conditional based on whether the current LOG_TAG is enabled at
 * warn priority.
 */
#ifndef IF_LOGW
#define IF_LOGW() IF_LOG(LOG_WARN, LOG_TAG)
#endif

/*
 * Conditional based on whether the current LOG_TAG is enabled at
 * error priority.
 */
#ifndef IF_LOGE
#define IF_LOGE() IF_LOG(LOG_ERROR, LOG_TAG)
#endif

/* --------------------------------------------------------------------- */

/*
 * Basic log message macro.
 *
 * Example:
 *  LOG(LOG_WARN, NULL, "Failed with error %d", errno);
 *
 * The second argument may be NULL or "" to indicate the "global" tag.
 */
#ifndef LOG
#define LOG(priority, tag, ...) LOG_PRI(LINUX_##priority, tag, __VA_ARGS__)
#endif

/*
 * Conditional given a desired logging priority and tag.
 */
#ifndef IF_LOG
#define IF_LOG(priority, tag) if (linux_testLog(linux_##priority, tag))
#endif

/* --------------------------------------------------------------------- */

/*
 *    IF_LOG uses linux_testLog, but IF_LOG can be overridden.
 *    linux_testLog will remain constant in its purpose as a wrapper
 *        for Android logging filter policy, and can be subject to
 *        change. It can be reused by the developers that override
 *        IF_LOG as a convenient means to reimplement their policy
 *        over Android.
 */

#if LOG_NDEBUG /* Production */
#define linux_testLog(prio, tag) \
  (__linux_log_is_loggable_len(prio, tag, (tag) ? strlen(tag) : 0, linux_LOG_DEBUG) != 0)
#else
#define linux_testLog(prio, tag) \
  (__linux_log_is_loggable_len(prio, tag, (tag) ? strlen(tag) : 0, linux_LOG_VERBOSE) != 0)
#endif

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

__END_DECLS
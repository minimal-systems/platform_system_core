/*
 * Copyright (C) 2006 The Android Open Source Project
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

#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>

#include <log/event_tag_map.h>
#include <minimal_systems/log.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    /* Verbs */
    FORMAT_OFF = 0,
    FORMAT_BRIEF,
    FORMAT_PROCESS,
    FORMAT_TAG,
    FORMAT_THREAD,
    FORMAT_RAW,
    FORMAT_TIME,
    FORMAT_THREADTIME,
    FORMAT_LONG,
    /* Adverbs. The following are modifiers to above format verbs */
    FORMAT_MODIFIER_COLOR,     /* converts priority to color */
    FORMAT_MODIFIER_TIME_USEC, /* switches from msec to usec time precision */
    FORMAT_MODIFIER_PRINTABLE, /* converts non-printable to printable escapes */
    FORMAT_MODIFIER_YEAR,      /* Adds year to date */
    FORMAT_MODIFIER_ZONE,      /* Adds zone to date, + UTC */
    FORMAT_MODIFIER_EPOCH,     /* Print time as seconds since Jan 1 1970 */
    FORMAT_MODIFIER_MONOTONIC, /* Print cpu time as seconds since start */
    FORMAT_MODIFIER_UID,       /* Adds uid */
    FORMAT_MODIFIER_DESCRIPT,  /* Adds descriptive */
    /* private, undocumented */
    FORMAT_MODIFIER_TIME_NSEC, /* switches from msec to nsec time precision */
} LinuxLogPrintFormat;

typedef struct LinuxLogFormat_t LinuxLogFormat;

typedef struct LinuxLogEntry_t {
    time_t tv_sec;
    long tv_nsec;
    linux_LogPriority priority;
    int32_t uid;
    int32_t pid;
    int32_t tid;
    const char* tag;
    size_t tagLen;
    size_t messageLen;
    const char* message;
} LinuxLogEntry;

LinuxLogFormat* linux_log_format_new();

void linux_log_format_free(LinuxLogFormat* p_format);

/* currently returns 0 if format is a modifier, 1 if not */
int linux_log_setPrintFormat(LinuxLogFormat* p_format, LinuxLogPrintFormat format);

/**
 * Returns FORMAT_OFF on invalid string
 */
LinuxLogPrintFormat linux_log_formatFromString(const char* s);

/**
 * filterExpression: a single filter expression
 * eg "AT:d"
 *
 * returns 0 on success and -1 on invalid expression
 *
 * Assumes single threaded execution
 *
 */

int linux_log_addFilterRule(LinuxLogFormat* p_format, const char* filterExpression);

/**
 * filterString: a whitespace-separated set of filter expressions
 * eg "AT:d *:i"
 *
 * returns 0 on success and -1 on invalid expression
 *
 * Assumes single threaded execution
 *
 */

int linux_log_addFilterString(LinuxLogFormat* p_format, const char* filterString);

/**
 * returns 1 if this log line should be printed based on its priority
 * and tag, and 0 if it should not
 */
int linux_log_shouldPrintLine(LinuxLogFormat* p_format, const char* tag, linux_LogPriority pri);

/**
 * Splits a wire-format buffer into a LinuxLogEntry
 * entry allocated by caller. Pointers will point directly into buf
 *
 * Returns 0 on success and -1 on invalid wire format (entry will be
 * in unspecified state)
 */
int linux_log_processLogBuffer(struct logger_entry* buf, LinuxLogEntry* entry);

/**
 * Like linux_log_processLogBuffer, but for binary logs.
 *
 * If "map" is non-NULL, it will be used to convert the log tag number
 * into a string.
 */
int linux_log_processBinaryLogBuffer(struct logger_entry* buf, LinuxLogEntry* entry,
                                     const EventTagMap* map, char* messageBuf, int messageBufLen);

/**
 * Formats a log message into a buffer
 *
 * Uses defaultBuffer if it can, otherwise malloc()'s a new buffer
 * If return value != defaultBuffer, caller must call free()
 * Returns NULL on malloc error
 */

char* linux_log_formatLogLine(LinuxLogFormat* p_format, char* defaultBuffer,
                              size_t defaultBufferSize, const LinuxLogEntry* p_line,
                              size_t* p_outLength);

/**
 * Formats a log message into a FILE*.
 */
size_t linux_log_printLogLine(LinuxLogFormat* p_format, FILE* fp, const LinuxLogEntry* entry);

#ifdef __cplusplus
}
#endif

# pragma once
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
# include "log.h"


#define DEBUG_COLOR "\e[33m"
#define INFO_COLOR  "\e[0m"
#define WARN_COLOR  "\e[35m"
#define ERROR_COLOR "\e[31m"

#define LOG_FORMAT "%4c%5d%4d  %-20s"
#define LOG_END "\e[39m\n"

int n1 = 245;
int n2 = 285;


static FILE *logfile;

int prepare_log ()
{
    logfile = fopen("dev/log/console","w");
    if (logfile == NULL) {
        LOGE("couldn't create an logging file");
        return -2;
    }
    return 0;
}

static int logger(const char *tag, char type, const char *format, va_list ap)
{
    va_list ap2;
    va_copy(ap2, ap);

    if (type == 'D') {
        fwrite(DEBUG_COLOR, 1, strlen(DEBUG_COLOR), stderr);
    } else if (type == 'I') {
        fwrite(INFO_COLOR, 1, strlen(INFO_COLOR), stderr);
    } else if (type == 'W') {
        fwrite(WARN_COLOR, 1, strlen(WARN_COLOR), stderr);
    } else if (type == 'E') {
        fwrite(ERROR_COLOR, 1, strlen(ERROR_COLOR), stderr);
    }

    if (logfile) {
        fprintf(logfile, LOG_FORMAT, type, n1, n2, tag);
        vfprintf(logfile, format, ap);
        fputs("\n", logfile);
    }

    int count = fprintf(stderr, LOG_FORMAT, type, n1, n2, tag);
    count += vfprintf(stderr, format, ap2);
    if (fputs(LOG_END, stderr)) {
        count += strlen(LOG_END);
    }
    return count;
}

int pr_debug(const char *tag, const char *format, ...)
{
    va_list args;

    va_start(args, format);
    int count = logger(tag, 'D', format, args);
    va_end(args);
    return count;
}

int pr_err(const char *tag, const char *format, ...)
{
    va_list args;

    va_start(args, format);
    int count = logger(tag, 'E', format, args);
    va_end(args);
    return count;
}


int pr_info(const char *tag, const char *format, ...)
{
    va_list args;

    va_start(args, format);
    int count = logger(tag, 'I', format, args);
    va_end(args);
    return count;
}

int pr_warn(const char *tag, const char *format, ...)
{
    va_list args;

    va_start(args, format);
    int count = logger(tag, 'W', format, args);
    va_end(args);
    return count;
}

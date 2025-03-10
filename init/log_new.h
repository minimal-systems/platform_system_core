#pragma once

#include <log/logprint.h>
#include <pthread.h>
#include <unistd.h>

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <ctime>

#ifdef __cplusplus
extern "C" {
#endif

#include <logging/logging.h>

// Core logging function with enforced newlines
#define LOG_PRINT(priority, tag, fmt, ...)                   \
  do {                                                       \
    print_log_to_console(priority, tag, fmt, ##__VA_ARGS__); \
  } while (0)

// Function to print log directly to the console
static inline void print_log_to_console(linux_LogPriority priority,
                                        const char* tag, const char* fmt, ...) {
  const char* priority_str;
  switch (priority) {
    case LINUX_LOG_VERBOSE:
      priority_str = "VERBOSE";
      break;
    case LINUX_LOG_DEBUG:
      priority_str = "DEBUG";
      break;
    case LINUX_LOG_INFO:
      priority_str = "INFO";
      break;
    case LINUX_LOG_WARN:
      priority_str = "WARN";
      break;
    case LINUX_LOG_ERROR:
      priority_str = "ERROR";
      break;
    case LINUX_LOG_FATAL:
      priority_str = "FATAL";
      break;
    default:
      priority_str = "UNKNOWN";
      break;
  }

  char timestamp[64];
  time_t now = time(NULL);
  struct tm* tm_info = localtime(&now);
  strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);

  va_list args;
  va_start(args, fmt);

  // Print timestamp, priority, and tag
  fprintf(stdout, "[%s] %s: [%s] ", timestamp, priority_str,
          tag ? tag : "NO_TAG");
  // Print the message
  vfprintf(stdout, fmt, args);
  va_end(args);

  // Append a newline and flush the output
  fprintf(stdout, "\n");
  fflush(stdout);
}

#ifdef __cplusplus
}
#endif

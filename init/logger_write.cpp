#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>

#include <minimal_systems/log.h>
#include <log/logprint.h>

#define LOG_BUF_SIZE 1024

/*
 * Define constants for color mappings
 */
#define LINUX_COLOR_BLUE 34
#define LINUX_COLOR_DEFAULT 39
#define LINUX_COLOR_GREEN 32
#define LINUX_COLOR_RED 31
#define LINUX_COLOR_YELLOW 33

static char priorityToChar(int prio) {
    switch (prio) {
        case 2: return 'V'; // Verbose
        case 3: return 'D'; // Debug
        case 4: return 'I'; // Info
        case 5: return 'W'; // Warn
        case 6: return 'E'; // Error
        case 7: return 'F'; // Fatal
        default: return 'U'; // Unknown
    }
}

// Function to map priority to a color identifier
static int colorFromPri(int prio) {
    switch (prio) {
        case 2: return LINUX_COLOR_DEFAULT; // Verbose
        case 3: return LINUX_COLOR_BLUE;    // Debug
        case 4: return LINUX_COLOR_GREEN;   // Info
        case 5: return LINUX_COLOR_YELLOW;  // Warn
        case 6: return LINUX_COLOR_RED;     // Error
        case 7: return LINUX_COLOR_RED;     // Fatal
        default: return LINUX_COLOR_DEFAULT; // Unknown
    }
}

// Format the current timestamp
static void formatTimestamp(char* buffer, size_t bufferSize) {
    struct timespec ts;
    struct tm tm;
    clock_gettime(CLOCK_REALTIME, &ts);
    localtime_r(&ts.tv_sec, &tm);

    snprintf(buffer, bufferSize, "%02d-%02d %02d:%02d:%02d.%03ld",
             tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec,
             ts.tv_nsec / 1000000);
}


int __linux_log_print(int prio, const char* tag, const char* fmt, ...) {
    va_list args;

    // Start variable argument processing
    va_start(args, fmt);

    // Allocate a buffer for the raw log message
    char buffer[LOG_BUF_SIZE];
    vsnprintf(buffer, sizeof(buffer), fmt, args);

    // End variable argument processing
    va_end(args);

    // Ensure a newline at the end of the message
    size_t len = strlen(buffer);
    if (len == 0 || buffer[len - 1] != '\n') {
        if (len + 1 < sizeof(buffer)) {
            buffer[len] = '\n';
            buffer[len + 1] = '\0';
        } else {
            buffer[sizeof(buffer) - 2] = '\n';
            buffer[sizeof(buffer) - 1] = '\0';
        }
    }

    // Get current timestamp
    time_t now = time(NULL);
    struct tm tm_info;
    localtime_r(&now, &tm_info);

    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%m-%d %H:%M:%S", &tm_info);

    // Map priority to character and color
    char prioChar = priorityToChar(prio);
    int color = colorFromPri(prio);

    // Get current UID
    uid_t uid = getuid();

    // Print the formatted log message with priority, tag, UID, and message
    fprintf(stderr, "\033[0;%dm%s %-8s %-8d %-8u %c %s\033[0m",
            color, timestamp, tag ? tag : "default", getpid(), uid, prioChar, buffer);

    return 1; // Return success
}

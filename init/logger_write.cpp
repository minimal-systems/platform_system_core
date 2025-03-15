#include <errno.h>
#include <fcntl.h>
#include <log/logprint.h>
#include <minimal_systems/log.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define LOG_BUF_SIZE 1024

/*
 * Define constants for color mappings
 */
#define LINUX_COLOR_BLUE 34
#define LINUX_COLOR_DEFAULT 39
#define LINUX_COLOR_GREEN 32
#define LINUX_COLOR_RED 31
#define LINUX_COLOR_YELLOW 33
#define KMSG_PATH "/dev/kmsg"

static bool kernel_logging_enabled = true;

static char priorityToChar(int prio) {
    switch (prio) {
        case 2:
            return 'V';  // Verbose
        case 3:
            return 'D';  // Debug
        case 4:
            return 'I';  // Info
        case 5:
            return 'W';  // Warn
        case 6:
            return 'E';  // Error
        case 7:
            return 'F';  // Fatal
        default:
            return 'U';  // Unknown
    }
}

// Function to map priority to a color identifier
static int colorFromPri(int prio) {
    switch (prio) {
        case 2:
            return LINUX_COLOR_DEFAULT;  // Verbose
        case 3:
            return LINUX_COLOR_BLUE;  // Debug
        case 4:
            return LINUX_COLOR_GREEN;  // Info
        case 5:
            return LINUX_COLOR_YELLOW;  // Warn
        case 6:
            return LINUX_COLOR_RED;  // Error
        case 7:
            return LINUX_COLOR_RED;  // Fatal
        default:
            return LINUX_COLOR_DEFAULT;  // Unknown
    }
}

// Format the current timestamp
static void formatTimestamp(char* buffer, size_t bufferSize) {
    struct timespec ts;
    struct tm tm;
    clock_gettime(CLOCK_REALTIME, &ts);
    localtime_r(&ts.tv_sec, &tm);

    snprintf(buffer, bufferSize, "%02d-%02d %02d:%02d:%02d.%03ld", tm.tm_mon + 1, tm.tm_mday,
             tm.tm_hour, tm.tm_min, tm.tm_sec, ts.tv_nsec / 1000000);
}

// Log to kernel message buffer
static void log_to_kernel(int prio, const char* message) {
    // If kernel logging is disabled due to a previous failure, return early
    if (!kernel_logging_enabled) {
        return;
    }

    int fd = open(KMSG_PATH, O_WRONLY | O_APPEND);
    if (fd < 0) {
        if (errno == EACCES) {
            // Disable kernel logging to prevent spam
            kernel_logging_enabled = false;
        }
        perror("Failed to open /dev/kmsg");
        return;
    }

    // Kernel priority levels (0 - emergency, 7 - debug)
    int kernel_prio;
    switch (prio) {
        case 2:
            kernel_prio = 7;
            break;  // Verbose -> Debug
        case 3:
            kernel_prio = 6;
            break;  // Debug -> Info
        case 4:
            kernel_prio = 5;
            break;  // Info -> Notice
        case 5:
            kernel_prio = 4;
            break;  // Warn -> Warning
        case 6:
            kernel_prio = 3;
            break;  // Error -> Error
        case 7:
            kernel_prio = 2;
            break;  // Fatal -> Critical
        default:
            kernel_prio = 5;
            break;  // Default to Notice
    }

    // Ensure no trailing newline before writing
    size_t len = strlen(message);
    char msg_clean[LOG_BUF_SIZE];

    if (len > 0 && message[len - 1] == '\n') {
        snprintf(msg_clean, sizeof(msg_clean), "Init: %.*s", (int)(len - 1), message);
    } else {
        snprintf(msg_clean, sizeof(msg_clean), "Init: %s", message);
    }

    // Write formatted message to /dev/kmsg
    dprintf(fd, "<%d>%s", kernel_prio, msg_clean);
    close(fd);
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
    fprintf(stderr, "\033[0;%dm%s %-8s %-8d %-8u %c %s\033[0m", color, timestamp,
            tag ? tag : "default", getpid(), uid, prioChar, buffer);

    // If the log priority is WARN or higher (>=5), log to the kernel
    if (prio >= 5) {
        log_to_kernel(prio, buffer);
    }

    return 1;  // Return success
}

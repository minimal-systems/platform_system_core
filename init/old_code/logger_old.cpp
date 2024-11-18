#include <iostream>
#include <fstream>
#include <cstdarg>
#include <cstdlib>
#include <cstring>
#include <string>
#include <iomanip>
#include <vector>
#include <map>
#include "log_new.h"

#define LOG_TAG "init"


// ANSI color codes for log levels
constexpr const char* DEBUG_COLOR = "\033[33m"; // Yellow for DEBUG
constexpr const char* INFO_COLOR = "\033[0m";   // Default for INFO
constexpr const char* WARN_COLOR = "\033[35m";  // Magenta for WARN
constexpr const char* ERROR_COLOR = "\033[31m"; // Red for ERROR
constexpr const char* LOG_END = "\033[39m\n";   // Reset color

// Log format
constexpr const char* LOG_FORMAT = "%4c %5d %4d  %-20s ";

int n1 = 245;
int n2 = 285;

static std::ofstream logfile;
static std::map<std::string, int> log_severity;
static int minimum_severity = 'I';

// Function to prepare the log file
int prepare_log() {
    std::system("mkdir -p mnt/var/log");
    logfile.open("mnt/var/system.log", std::ios::out);
    if (!logfile.is_open()) {
        std::cerr << ERROR_COLOR << "Couldn't create a logging file" << LOG_END;
        return -2;
    }
    return 0;
}

// Core logger function
static int logger(const char* tag, char type, const char* format, va_list args) {
    if (type < minimum_severity) {
        return 0; // Skip logging messages below the minimum severity level
    }

    const char* color = INFO_COLOR;
    if (!tag || !format) {
        std::cerr << ERROR_COLOR << "Null pointer passed to logger" << LOG_END;
        return -1;
    }

    switch (type) {
        case 'D': color = DEBUG_COLOR; break;
        case 'E': color = ERROR_COLOR; break;
        case 'W': color = WARN_COLOR; break;
    }

    if (logfile.is_open()) {
        logfile << type << " " << n1 << " " << n2 << "  " 
                << std::setw(20) << std::left << tag << " ";
        char buffer[1024];
        vsnprintf(buffer, sizeof(buffer), format, args);
        logfile << buffer << "\n";
    }

    std::cerr << color;
    int count = fprintf(stderr, LOG_FORMAT, type, n1, n2, tag);
    va_list args_copy;
    va_copy(args_copy, args);
    count += vfprintf(stderr, format, args_copy);
    va_end(args_copy);
    std::cerr << LOG_END;

    return count;
}

// Logging functions
int pr_debug(const char* tag, const char* format, ...) {
    va_list args;
    va_start(args, format);
    int result = logger(tag, 'D', format, args);
    va_end(args);
    return result;
}

int pr_err(const char* tag, const char* format, ...) {
    va_list args;
    va_start(args, format);
    int result = logger(tag, 'E', format, args);
    va_end(args);
    return result;
}

int pr_info(const char* tag, const char* format, ...) {
    va_list args;
    va_start(args, format);
    int result = logger(tag, 'I', format, args);
    va_end(args);
    return result;
}

int pr_warn(const char* tag, const char* format, ...) {
    va_list args;
    va_start(args, format);
    int result = logger(tag, 'W', format, args);
    va_end(args);
    return result;
}

int pr_verbose(const char* tag, const char* format, ...) {
    va_list args;
    va_start(args, format);
    int result = logger(tag, 'V', format, args);
    va_end(args);
    return result;
}

int pr_fatal(const char* tag, const char* format, ...) {
    va_list args;
    va_start(args, format);
    fprintf(stderr, "[%s] FATAL: ", tag);
    int result = vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
    va_end(args);

    fflush(stderr);
    std::exit(EXIT_FAILURE);

    return result;
}

// Split function to tokenize strings
std::vector<std::string> Split(const std::string& str, const std::string& delim) {
    std::vector<std::string> tokens;
    size_t start = 0, end;
    while ((end = str.find(delim, start)) != std::string::npos) {
        tokens.push_back(str.substr(start, end - start));
        start = end + delim.length();
    }
    tokens.push_back(str.substr(start));
    return tokens;
}

void SetLogger(std::function<int(const char*, char, const char*, va_list)>&& logger) {
    static std::function<int(const char*, char, const char*, va_list)> gLogger;
    gLogger = std::move(logger);
}

void SetAborter(std::function<void(const char*)>&& aborter) {
    static std::function<void(const char*)> gAborter;
    gAborter = std::move(aborter);
}

void SetDefaultTag(const std::string& tag) {
    static std::string gDefaultTag;
    gDefaultTag = tag;
}

// InitLogging function
void InitLogging(char* argv[], LogFunction&& logger, AbortFunction&& aborter) {
    SetLogger(std::forward<LogFunction>(logger));
    SetAborter(std::forward<AbortFunction>(aborter));

    static bool initialized = false;
    if (initialized) {
        return;
    }

    initialized = true;

    if (argv != nullptr) {
        SetDefaultTag(argv[0]);
    }

    const char* tags = getenv("LINUX_LOG_TAGS");
    if (tags == nullptr) {
        return;
    }

    std::vector<std::string> specs = Split(tags, " ");
    for (const auto& spec : specs) {
        if (spec.size() == 3 && spec[1] == ':') {
            char level = spec[2];
            switch (level) {
                case 'v': minimum_severity = 'D'; break; // Verbose
                case 'd': minimum_severity = 'D'; break; // Debug
                case 'i': minimum_severity = 'I'; break; // Info
                case 'w': minimum_severity = 'W'; break; // Warn
                case 'e': minimum_severity = 'E'; break; // Error
                case 'f': minimum_severity = 'F'; break; // Fatal
                case 's': minimum_severity = 'F'; break; // Silent
                default:
                    pr_fatal(LOG_TAG, "Unsupported log level in LINUX_LOG_TAGS: %c", level);
            }
        }
    }
}

static void KernelLogLine(LogSeverity severity, const char* tag, const std::string& line) {
    switch (severity) {
        case DEBUG:
            pr_debug(tag, "%s", line.c_str());
            break;
        case INFO:
            pr_info(tag, "%s", line.c_str());
            break;
        case WARNING:
            pr_warn(tag, "%s", line.c_str());
            break;
        case ERROR:
            pr_err(tag, "%s", line.c_str());
            break;
        case VERBOSE:
            pr_verbose(tag, "%s", line.c_str());
            break;
        case FATAL_WITHOUT_ABORT:
        case FATAL:
            pr_fatal(tag, "%s", line.c_str());
            break;
        default:
            pr_err(tag, "Unexpected log severity: %d, message: %s", severity, line.c_str());
            break;
    }
}

template <typename F>
void SplitByLines(const char* msg, const F& log_function, LogSeverity severity, const char* tag) {
    const char* newline = strchr(msg, '\n');
    while (newline != nullptr) {
        log_function(severity, tag, std::string(msg, newline - msg));
        msg = newline + 1;
        newline = strchr(msg, '\n');
    }

    if (*msg != '\0') {  // Log the last line if it's non-empty
        log_function(severity, tag, std::string(msg));
    }
}

void KernelLogger(int log_id, LogSeverity severity, const char* tag, const char* file,
                  unsigned int line, const char* full_message) {
    (void)log_id;  // Unused
    (void)file;    // Unused
    (void)line;    // Unused
    SplitByLines(full_message, KernelLogLine, severity, tag);
}

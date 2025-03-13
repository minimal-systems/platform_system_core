// init_stdio.cpp
// Corrected and improved version of standard I/O initialization routines.

#define LOG_TAG "init"

#include "util.h"

#include <fcntl.h>
#include <pwd.h>
#include <unistd.h>

#include <cctype>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <string>
#include <regex>

#include "log_new.h"

namespace minimal_systems {
namespace init {

const std::string kDataDirPrefix("/home");

/**
 * Decodes a username or UID string into a numeric UID.
 *
 * This function attempts to resolve a given username to a UID. If the input is numeric,
 * it directly converts it to a UID.
 *
 * @param name Username or numeric UID as a string.
 * @return Result containing the UID on success or an error message on failure.
 */
Result<uid_t> DecodeUid(const std::string &name) {
    if (name.empty()) {
        return Result<uid_t>::Failure("Username/UID string is empty.");
    }

    // Check if the name starts with an alphabet, indicating a username.
    if (std::isalpha(name[0])) {
        errno = 0;
        passwd *pwd = getpwnam(name.c_str());
        if (!pwd) {
            int saved_errno = errno;
            return Result<uid_t>::Failure(
                "getpwnam failed: " + std::string(strerror(saved_errno)));
        }
        return Result<uid_t>::Success(pwd->pw_uid);
    }

    // If the name is numeric, convert it to UID.
    errno = 0;
    char *endptr = nullptr;
    uid_t result = static_cast<uid_t>(std::strtoul(name.c_str(), &endptr, 0));
    if (errno || endptr == name.c_str() || *endptr != '\0') {
        int saved_errno = errno;
        return Result<uid_t>::Failure(
            "strtoul failed: " + std::string(strerror(saved_errno)));
    }

    return Result<uid_t>::Success(result);
}

/**
 * Redirects stdin to /dev/null and stdout/stderr to the controlling terminal (/dev/tty).
 *
 * This function ensures proper error handling and resource cleanup during initialization.
 * It helps avoid unintended interactive input and ensures logs are directed to the terminal.
 *
 * @param argv Command-line arguments (unused, kept for compatibility).
 */
void SetStdioToDevNull(char **argv) {
    int fd_null = open("/dev/null", O_RDWR | O_CLOEXEC);
    if (fd_null == -1) {
        int saved_errno = errno;
        LOGE("Failed to open /dev/null: %s", strerror(saved_errno));
        std::exit(EXIT_FAILURE);
    }

    int fd_terminal = open("/dev/tty", O_RDWR | O_CLOEXEC);
    if (fd_terminal == -1) {
        int saved_errno = errno;
        LOGE("Failed to open /dev/tty: %s", strerror(saved_errno));
        close(fd_null);
        std::exit(EXIT_FAILURE);
    }

    // Redirect standard input to /dev/null.
    if (dup2(fd_null, STDIN_FILENO) == -1) {
        int saved_errno = errno;
        LOGE("Failed to redirect stdin to /dev/null: %s", strerror(saved_errno));
        close(fd_null);
        close(fd_terminal);
        std::exit(EXIT_FAILURE);
    }

    // Redirect standard output and error to /dev/tty.
    if (dup2(fd_terminal, STDOUT_FILENO) == -1 ||
        dup2(fd_terminal, STDERR_FILENO) == -1) {
        int saved_errno = errno;
        LOGE("Failed to redirect stdout/stderr to /dev/tty: %s", strerror(saved_errno));
        close(fd_null);
        close(fd_terminal);
        std::exit(EXIT_FAILURE);
    }

    if (fd_null > STDERR_FILENO) {
        close(fd_null);
    }
    if (fd_terminal > STDERR_FILENO) {
        close(fd_terminal);
    }

    LOGI("Standard I/O successfully redirected: stdin -> /dev/null, stdout/stderr -> /dev/tty");
}

/**
 * Initializes kernel logging facilities.
 *
 * This function sets up logging for kernel-related events and ensures that critical
 * errors result in appropriate system handling (e.g., reboot, shutdown, or panic).
 *
 * @param argv Command-line arguments (unused, kept for compatibility).
 */
void InitKernelLogging(char **argv) {
    // TODO: Implement or clarify SetFatalRebootTarget() behavior.
    SetFatalRebootTarget();
    LOGI("Kernel logging initialized successfully.");
}

/**
 * Extracts the UUID of the root filesystem from the kernel command line.
 *
 * This function searches for the `root=UUID=<UUID>` pattern in the given command-line
 * string and extracts the UUID if found.
 *
 * @param cmdline The kernel command line string.
 * @return Extracted root filesystem UUID or an empty string if not found.
 */
std::string ExtractRootUUID(const std::string &cmdline) {
    std::smatch match;
    if (std::regex_search(cmdline, match, std::regex(R"(root=UUID=([a-fA-F0-9-]+))"))) {
        return match[1];
    }
    return "";
}

/**
 * Normalizes a file path by ensuring proper formatting.
 *
 * - Converts absolute paths (`/path`) to relative (`./path`).
 * - Removes any trailing slashes except for root (`/`).
 *
 * @param path The input path to normalize.
 * @return A normalized version of the path.
 */
std::string NormalizePath(const std::string &path) {
    if (path.empty()) {
        return "./";
    }

    std::string normalized = path;

    // Ensure the path starts with "./" if it was absolute.
    if (normalized[0] == '/') {
        normalized[0] = '.';
    }

    // Remove trailing slash if present.
    if (normalized.length() > 1 && normalized.back() == '/') {
        normalized.pop_back();
    }

    return normalized;
}

/**
 * Joins a directory and file name into a complete path.
 *
 * This function ensures that paths are properly formatted and combined, avoiding
 * duplicate or missing slashes.
 *
 * @param dir The directory path.
 * @param file The filename.
 * @return A properly formatted path.
 */
std::string JoinPath(const std::string &dir, const std::string &file) {
    if (dir.empty()) {
        return NormalizePath(file);
    }

    std::string normalized_dir = NormalizePath(dir);

    if (normalized_dir.back() != '/') {
        return normalized_dir + "/" + file;
    }

    return normalized_dir + file;
}

}  // namespace init
}  // namespace minimal_systems

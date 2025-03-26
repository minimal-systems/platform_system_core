// init_stdio.cpp
// Corrected and improved version of standard I/O initialization routines.

#define LOG_TAG "init"

#include "util.h"

#include <fcntl.h>
#include <pwd.h>
#include <unistd.h>

#include <sys/stat.h>
#include <cctype>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <regex>
#include <string>

#include "log_new.h"
#include "bootcfg.h"
#include "property_manager.h"

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
Result<uid_t> DecodeUid(const std::string& name) {
    if (name.empty()) {
        return Result<uid_t>::Failure("Username/UID string is empty.");
    }

    // Check if the name starts with an alphabet, indicating a username.
    if (std::isalpha(name[0])) {
        errno = 0;
        passwd* pwd = getpwnam(name.c_str());
        if (!pwd) {
            int saved_errno = errno;
            return Result<uid_t>::Failure("getpwnam failed: " + std::string(strerror(saved_errno)));
        }
        return Result<uid_t>::Success(pwd->pw_uid);
    }

    // If the name is numeric, convert it to UID.
    errno = 0;
    char* endptr = nullptr;
    uid_t result = static_cast<uid_t>(std::strtoul(name.c_str(), &endptr, 0));
    if (errno || endptr == name.c_str() || *endptr != '\0') {
        int saved_errno = errno;
        return Result<uid_t>::Failure("strtoul failed: " + std::string(strerror(saved_errno)));
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
void SetStdioToDevNull(char** argv) {
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
    if (dup2(fd_terminal, STDOUT_FILENO) == -1 || dup2(fd_terminal, STDERR_FILENO) == -1) {
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
void InitKernelLogging(char** argv) {
    // TODO: Implement or clarify SetFatalRebootTarget() behavior.
    // SetFatalRebootTarget();
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
std::string ExtractRootUUID(const std::string& cmdline) {
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
std::string NormalizePath(const std::string& path) {
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
std::string JoinPath(const std::string& dir, const std::string& file) {
    if (dir.empty()) {
        return NormalizePath(file);
    }

    std::string normalized_dir = NormalizePath(dir);

    if (normalized_dir.back() != '/') {
        return normalized_dir + "/" + file;
    }

    return normalized_dir + file;
}

/**
 * @brief Checks if the system is currently running inside a ramdisk.
 *
 * @return true if running inside a ramdisk, false otherwise.
 */
bool IsRunningInRamdisk() {
    std::ifstream mounts("/proc/mounts");
    if (!mounts.is_open()) {
        LOGE("Failed to open /proc/mounts");
        return false;
    }

    std::string line;
    while (std::getline(mounts, line)) {
        if (line.find(" / ") != std::string::npos &&
            (line.find("tmpfs") != std::string::npos || line.find("ramfs") != std::string::npos)) {
            LOGI("Detected root filesystem is on a ramdisk");
            return true;
        }
    }

    LOGI("Root filesystem is not on a ramdisk");
    return false;
}

/**
 * Retrieves a system property using bootcfg first, then falling back to getprop().
 *
 * If the property exists in early boot config (`bootcfg`), returns it directly.
 * Otherwise, uses the dynamic property system (e.g., from `property_service`).
 *
 * @param key Property key to look up.
 * @return Corresponding property value or empty string.
 */
std::string GetProperty(const std::string& key) {
    std::string value = minimal_systems::bootcfg::Get(key);
    if (!value.empty()) {
        return value;
    }
    return getprop(key);
}

/**
 * Checks if a file exists using stat().
 *
 * @param path Absolute or relative file path.
 * @return true if file exists, false otherwise.
 */
bool FileExists(const std::string& path) {
    struct stat buffer;
    return stat(path.c_str(), &buffer) == 0;
}

/**
 * Reads and returns the first line of a file.
 *
 * Useful for small configuration files or single-line device files.
 *
 * @param path Path to file.
 * @return First line of file or empty string if not accessible.
 */
std::string ReadFirstLine(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return "";
    std::string line;
    std::getline(file, line);
    return line;
}

/**
 * Trims whitespace characters from both ends of the input string.
 *
 * Whitespace includes space, tab, newline, and carriage return.
 *
 * @param str Input string.
 * @return Trimmed string.
 */
std::string Trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    size_t end = str.find_last_not_of(" \t\r\n");
    return (start == std::string::npos) ? "" : str.substr(start, end - start + 1);
}

/**
 * Searches for a substring token in a file.
 *
 * Efficiently reads line-by-line and returns early on first match.
 *
 * @param path File path.
 * @param token Substring to search.
 * @return true if token is found, false otherwise.
 */
bool FileContains(const std::string& path, const std::string& token) {
    std::ifstream file(path);
    std::string line;
    while (std::getline(file, line)) {
        if (line.find(token) != std::string::npos) return true;
    }
    return false;
}

/**
 * Reads the entire contents of a file into a string.
 *
 * Suitable for small to medium-sized files. Not memory-safe for very large files.
 *
 * @param path File path.
 * @return File content as string.
 */
std::string ReadFile(const std::string& path) {
    std::ifstream file(path);
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

/**
 * Detects the GPU type by inspecting the DRM subsystem and known vendor files.
 *
 * - Checks for NVIDIA first via /proc.
 * - Checks for vendor IDs via sysfs (e.g., AMD, Intel).
 * - Checks DRM uevent for ARM-based GPUs (Mali, PowerVR).
 * - Falls back to CPU type or sets to "none".
 */
void DetectAndSetGPUType() {
    // NVIDIA check via proprietary driver interface
    if (FileExists("/proc/driver/nvidia/version")) {
        setprop("ro.boot.gpu", "nvidia");
        LOGI("GPU: NVIDIA detected");
        return;
    }

    const std::string vendor_path = "/sys/class/drm/card0/device/vendor";
    const std::string uevent_path = "/sys/class/drm/card0/device/uevent";

    if (FileExists(vendor_path)) {
        std::string vendor = Trim(ReadFirstLine(vendor_path));
        std::string gpu_type;

        if (vendor == "0x1002") {
            gpu_type = "amd";
        } else if (vendor == "0x8086") {
            gpu_type = "intel";
        } else if (FileExists(uevent_path)) {
            std::string uevent = ReadFile(uevent_path);
            if (uevent.find("mali") != std::string::npos ||
                uevent.find("MALI") != std::string::npos) {
                gpu_type = "mali";
            } else if (uevent.find("powervr") != std::string::npos) {
                gpu_type = "powervr";
            } else {
                gpu_type = "unknown";
            }
        } else {
            gpu_type = "unknown";
        }

        setprop("ro.boot.gpu", gpu_type);
        setprop("ro.boot.gpu.vendor_id", vendor);
        LOGI("GPU: %s detected (vendor=%s)", gpu_type.c_str(), vendor.c_str());
        return;
    }

    // Fallback for ARM devices without DRM
    if (FileContains("/proc/cpuinfo", "ARM") || FileContains("/proc/cpuinfo", "aarch64")) {
        setprop("ro.boot.gpu", "arm");
        LOGI("GPU: ARM64/ARM platform detected");
        return;
    }

    // Unknown/unsupported GPU
    setprop("ro.boot.gpu", "none");
    LOGI("GPU: Not detected");
}

/**
 * Writes the provided string to a file descriptor.
 *
 * Returns false if partial write or write error occurred.
 *
 * @param content String data to write.
 * @param fd Valid file descriptor to write to.
 * @return true if entire content was successfully written.
 */
bool WriteStringToFd(const std::string& content, int fd) {
    ssize_t written = write(fd, content.c_str(), content.size());
    return written == static_cast<ssize_t>(content.size());
}


}  // namespace init
}  // namespace minimal_systems

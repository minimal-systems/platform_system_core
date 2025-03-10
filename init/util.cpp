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

#include "log_new.h"

namespace minimal_systems {
namespace init {

const std::string kDataDirPrefix("/home");

/**
 * Decodes a username or UID string into a numeric UID.
 *
 * @param name Username or numeric UID as string.
 * @return Result containing the UID on success or an error message on failure.
 */
Result<uid_t> DecodeUid(const std::string& name) {
  if (name.empty()) {
    return Result<uid_t>::Failure("Username/UID string is empty.");
  }

  if (std::isalpha(name[0])) {
    errno = 0;
    passwd* pwd = getpwnam(name.c_str());
    if (!pwd) {
      int saved_errno = errno;
      return Result<uid_t>::Failure("getpwnam failed: " +
                                    std::string(strerror(saved_errno)));
    }
    return Result<uid_t>::Success(pwd->pw_uid);
  }

  errno = 0;
  char* endptr = nullptr;
  uid_t result = static_cast<uid_t>(std::strtoul(name.c_str(), &endptr, 0));
  if (errno || endptr == name.c_str() || *endptr != '\0') {
    int saved_errno = errno;
    return Result<uid_t>::Failure("strtoul failed: " +
                                  std::string(strerror(saved_errno)));
  }

  return Result<uid_t>::Success(result);
}

/**
 * Redirects stdin to /dev/null and stdout/stderr to the controlling terminal
 * (/dev/tty). Ensures proper error handling and resource cleanup.
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

  if (dup2(fd_null, STDIN_FILENO) == -1) {
    int saved_errno = errno;
    LOGE("Failed to redirect stdin to /dev/null: %s", strerror(saved_errno));
    close(fd_null);
    close(fd_terminal);
    std::exit(EXIT_FAILURE);
  }

  if (dup2(fd_terminal, STDOUT_FILENO) == -1 ||
      dup2(fd_terminal, STDERR_FILENO) == -1) {
    int saved_errno = errno;
    LOGE("Failed to redirect stdout/stderr to /dev/tty: %s",
         strerror(saved_errno));
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

  LOGI(
      "Standard I/O successfully redirected: stdin -> /dev/null, stdout/stderr "
      "-> /dev/tty");
}

/**
 * Initializes kernel logging facilities.
 * Sets fatal reboot targets or related kernel logging behavior.
 *
 * @param argv Command-line arguments (unused, kept for compatibility).
 */
void InitKernelLogging(char** argv) {
  // TODO: Implement or clarify SetFatalRebootTarget() behavior.
  SetFatalRebootTarget();
  LOGI("Kernel logging initialized successfully.");
}

}  // namespace init
}  // namespace minimal_systems

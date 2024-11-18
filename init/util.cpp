#define LOG_TAG "init"

#include <fcntl.h>
#include <unistd.h>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <pwd.h>
#include <string>
#include <cctype>
#include "log_new.h"
#include "util.h"


namespace minimal_systems {
namespace init {

const std::string kDataDirPrefix("/home");

template <typename T>

Result<uid_t> DecodeUid(const std::string& name) {
    if (std::isalpha(name[0])) {
        passwd* pwd = getpwnam(name.c_str());
        if (!pwd) {
            return Result<uid_t>::Failure("getpwnam failed: " + std::string(strerror(errno)));
        }
        return Result<uid_t>::Success(pwd->pw_uid);
    }

    errno = 0;
    char* endptr = nullptr;
    uid_t result = static_cast<uid_t>(std::strtoul(name.c_str(), &endptr, 0));
    if (errno || endptr == name.c_str() || *endptr != '\0') {
        return Result<uid_t>::Failure("strtoul failed: " + std::string(strerror(errno)));
    }

    return Result<uid_t>::Success(result);
}

void SetStdioToDevNull(char** argv) {
    int fd_null = open("/dev/null", O_RDWR);
    if (fd_null == -1) {
        int saved_errno = errno;
        LOGE("Error: Couldn't open /dev/null: %s", strerror(saved_errno));
        std::exit(EXIT_FAILURE);
    }

    int fd_terminal = open("/dev/tty", O_RDWR);
    if (fd_terminal == -1) {
        int saved_errno = errno;
        LOGE("Error: Couldn't open terminal (/dev/tty): %s", strerror(saved_errno));
        close(fd_null);
        std::exit(EXIT_FAILURE);
    }

    if (dup2(fd_null, STDIN_FILENO) == -1) {
        int saved_errno = errno;
        LOGE("Error: Couldn't redirect stdin to /dev/null: %s", strerror(saved_errno));
        close(fd_null);
        close(fd_terminal);
        std::exit(EXIT_FAILURE);
    }

    if (dup2(fd_terminal, STDOUT_FILENO) == -1 || dup2(fd_terminal, STDERR_FILENO) == -1) {
        int saved_errno = errno;
        LOGE("Error: Couldn't redirect stdout/stderr to terminal: %s", strerror(saved_errno));
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

    LOGI("Standard I/O redirected: stdin to /dev/null, stdout and stderr to terminal.");
}


void InitKernelLogging(char** argv) {
    // Set up the fatal reboot target, if applicable
    SetFatalRebootTarget();

    LOGI("Kernel logging initialized.");
}


}  // namespace init
}  // namespace minimal_systems

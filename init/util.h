#ifndef MINIMAL_SYSTEMS_INIT_UTILS_H_
#define MINIMAL_SYSTEMS_INIT_UTILS_H_

#include <sys/stat.h>
#include <sys/types.h>
#include <fstream>
#include <optional>
#include <string>

namespace minimal_systems {
namespace init {

template <typename T>
class Result {
  public:
    static Result<T> Success(const T& value) { return Result(value); }

    static Result<T> Failure(const std::string& error) { return Result(error); }

    bool IsSuccess() const { return success_; }

    T Value() const { return value_; }

    std::string Error() const { return error_; }

  private:
    Result(const T& value) : success_(true), value_(value) {}
    Result(const std::string& error) : success_(false), error_(error) {}

    bool success_;
    T value_;
    std::string error_;
};

// Function prototypes
Result<uid_t> DecodeUid(const std::string& name);
void SetStdioToDevNull(char** argv);
void InitKernelLogging(char** argv);

// Utility functions
std::string ExtractRootUUID(const std::string& cmdline);
std::string NormalizePath(const std::string& path);
std::string JoinPath(const std::string& dir, const std::string& file);
bool IsRunningInRamdisk();

// Additional utilities
std::string GetProperty(const std::string& key);
bool FileExists(const std::string& path);
std::string ReadFirstLine(const std::string& path);

// GPU detection
void DetectAndSetGPUType();

// Read the first line from a file
std::string ReadFirstLine(const std::string& path);

// Get system property
std::string GetProperty(const std::string& key);

/**
 * Writes the given string to the specified file descriptor.
 * @param content The string to write.
 * @param fd The file descriptor.
 * @return true on success, false on failure.
 */
bool WriteStringToFd(const std::string& content, int fd);
}  // namespace init
}  // namespace minimal_systems

#endif  // MINIMAL_SYSTEMS_INIT_UTILS_H_

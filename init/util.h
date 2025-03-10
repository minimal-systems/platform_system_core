#ifndef MINIMAL_SYSTEMS_INIT_UTILS_H_
#define MINIMAL_SYSTEMS_INIT_UTILS_H_

#include <sys/types.h>

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

Result<uid_t> DecodeUid(const std::string& name);
void SetStdioToDevNull(char** argv);
void InitKernelLogging(char** argv);
// reboot_utils.h
inline void SetFatalRebootTarget(
    const std::optional<std::string>& = std::nullopt) {}
}  // namespace init
}  // namespace minimal_systems

#endif  // MINIMAL_SYSTEMS_INIT_UTILS_H_

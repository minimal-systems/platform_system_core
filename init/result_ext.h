// result.h - A simple Result class template for handling success and errors
#ifndef RESULT_H
#define RESULT_H

#include <iostream>
#include <optional>
#include <string>
#include <utility>

/**
 * @brief A template class for handling operation results, supporting both success and error states.
 *
 * @tparam T The type of the successful result.
 */
template <typename T>
class Result {
  public:
    // Factory methods for success and error states
    static Result<T> Success(T value) { return Result(std::move(value)); }

    static Result<T> Error(std::string error_message) { return Result(std::move(error_message)); }

    // Check if the result is successful
    bool IsSuccess() const { return success_; }

    // Retrieve the successful value (should only be called if IsSuccess() is true)
    T& Value() {
        if (!success_) {
            throw std::runtime_error("Attempted to access Value() on an error result.");
        }
        return *value_;
    }

    // Retrieve the error message (should only be called if IsSuccess() is false)
    const std::string& ErrorMessage() const {
        if (success_) {
            throw std::runtime_error("Attempted to access ErrorMessage() on a successful result.");
        }
        return error_message_;
    }

  private:
    // Private constructors to enforce controlled creation
    explicit Result(T value) : success_(true), value_(std::move(value)), error_message_("") {}

    explicit Result(std::string error_message)
        : success_(false), value_(std::nullopt), error_message_(std::move(error_message)) {}

    bool success_;
    std::optional<T> value_;
    std::string error_message_;
};

#endif  // RESULT_H

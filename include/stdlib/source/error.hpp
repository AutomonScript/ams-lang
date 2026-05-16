#pragma once

#include <string>
#include <stdexcept>
#include <utility>
#include <optional>

namespace ams {
namespace stdlib {
namespace source {

// Error codes for LOG_SOURCE operations
enum class LogSourceError {
    FILE_NOT_FOUND,
    PERMISSION_DENIED,
    IO_ERROR,
    PARSE_ERROR,
    INVALID_MODE,
    WRITE_NOT_PERMITTED,
    DISK_FULL,
    INVALID_RECORD,
    INVALID_REGEX,
    FILE_TOO_LARGE
};

// Exception type for LOG_SOURCE errors
struct LogSourceException : public std::runtime_error {
    LogSourceError error_code;
    std::string filepath;
    std::string details;
    
    LogSourceException(
        LogSourceError code,
        const std::string& path,
        const std::string& msg
    ) : std::runtime_error(msg),
        error_code(code),
        filepath(path),
        details(msg) {}
};

// Result type for error handling without exceptions
template<typename T>
class Result {
public:
    // Factory methods
    static Result<T> Ok(T value) {
        Result<T> result;
        result.is_ok_ = true;
        result.value_ = std::move(value);
        return result;
    }
    
    static Result<T> Err(LogSourceError error, const std::string& message) {
        Result<T> result;
        result.is_ok_ = false;
        result.error_ = error;
        result.error_message_ = message;
        return result;
    }
    
    // Query methods
    bool isOk() const { return is_ok_; }
    bool isErr() const { return !is_ok_; }
    
    // Value access
    T unwrap() const {
        if (!is_ok_ || !value_.has_value()) {
            throw LogSourceException(error_, "", error_message_);
        }
        return value_.value();
    }
    
    // Move-based unwrap for move-only types like unique_ptr
    T unwrap() {
        if (!is_ok_ || !value_.has_value()) {
            throw LogSourceException(error_, "", error_message_);
        }
        return std::move(value_.value());
    }
    
    T unwrapOr(T default_value) const {
        return (is_ok_ && value_.has_value()) ? value_.value() : default_value;
    }
    
    // Error access
    LogSourceError getError() const {
        return error_;
    }
    
    std::string getErrorMessage() const {
        return error_message_;
    }
    
private:
    Result() : is_ok_(false), error_(LogSourceError::IO_ERROR) {}
    
    bool is_ok_;
    std::optional<T> value_;
    LogSourceError error_;
    std::string error_message_;
};

// Specialization for void
template<>
class Result<void> {
public:
    static Result<void> Ok() {
        Result<void> result;
        result.is_ok_ = true;
        return result;
    }
    
    static Result<void> Err(LogSourceError error, const std::string& message) {
        Result<void> result;
        result.is_ok_ = false;
        result.error_ = error;
        result.error_message_ = message;
        return result;
    }
    
    bool isOk() const { return is_ok_; }
    bool isErr() const { return !is_ok_; }
    
    void unwrap() const {
        if (!is_ok_) {
            throw LogSourceException(error_, "", error_message_);
        }
    }
    
    LogSourceError getError() const {
        return error_;
    }
    
    std::string getErrorMessage() const {
        return error_message_;
    }
    
private:
    Result() : is_ok_(false), error_(LogSourceError::IO_ERROR) {}
    
    bool is_ok_;
    LogSourceError error_;
    std::string error_message_;
};

} // namespace source
} // namespace stdlib
} // namespace ams

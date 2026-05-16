#pragma once

#include <string>
#include <chrono>
#include <unordered_map>
#include <optional>
#include <sstream>
#include <iomanip>

namespace ams {
namespace stdlib {
namespace source {

// Log level enumeration
enum class LogLevel {
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL
};

// Immutable structure representing a single log entry
struct LOG_RECORD {
    std::chrono::system_clock::time_point timestamp;
    LogLevel level;
    std::string message;
    std::unordered_map<std::string, std::string> metadata;
    
    // Field access methods (read-only)
    std::string getTimestampString() const {
        auto time_t_val = std::chrono::system_clock::to_time_t(timestamp);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            timestamp.time_since_epoch()
        ) % 1000;
        
        std::tm tm_val;
        #ifdef _WIN32
        gmtime_s(&tm_val, &time_t_val);
        #else
        gmtime_r(&time_t_val, &tm_val);
        #endif
        
        std::ostringstream oss;
        oss << std::put_time(&tm_val, "%Y-%m-%dT%H:%M:%S");
        oss << '.' << std::setfill('0') << std::setw(3) << ms.count() << 'Z';
        return oss.str();
    }
    
    std::string getLevelString() const {
        switch (level) {
            case LogLevel::DEBUG: return "DEBUG";
            case LogLevel::INFO:  return "INFO";
            case LogLevel::WARN:  return "WARN";
            case LogLevel::ERROR: return "ERROR";
            case LogLevel::FATAL: return "FATAL";
            default: return "UNKNOWN";
        }
    }
    
    const std::string& getMessage() const {
        return message;
    }
    
    std::optional<std::string> getMetadata(const std::string& key) const {
        auto it = metadata.find(key);
        if (it != metadata.end()) {
            return it->second;
        }
        return std::nullopt;
    }
    
    // Comparison operators
    bool operator==(const LOG_RECORD& other) const {
        // Compare timestamps with millisecond precision
        auto this_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            timestamp.time_since_epoch()
        ).count();
        auto other_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            other.timestamp.time_since_epoch()
        ).count();
        
        return this_ms == other_ms &&
               level == other.level &&
               message == other.message &&
               metadata == other.metadata;
    }
    
    bool operator!=(const LOG_RECORD& other) const {
        return !(*this == other);
    }
};

} // namespace source
} // namespace stdlib
} // namespace ams

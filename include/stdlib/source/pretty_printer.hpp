#pragma once

#include "log_record.hpp"
#include <string>
#include <sstream>
#include <vector>

namespace ams {
namespace stdlib {
namespace source {

// Pretty printer for formatting LOG_RECORD objects back to log format
class Pretty_Printer {
public:
    Pretty_Printer() = default;
    
    // Format single LOG_RECORD to string
    std::string format(const LOG_RECORD& record) const {
        std::ostringstream oss;
        
        // Format: [TIMESTAMP] [LEVEL] MESSAGE [METADATA]
        oss << formatTimestamp(record.timestamp) << " ";
        oss << formatLogLevel(record.level) << " ";
        oss << record.message;
        
        if (!record.metadata.empty()) {
            oss << " " << formatMetadata(record.metadata);
        }
        
        return oss.str();
    }
    
    // Format batch of LOG_RECORDs
    std::string formatBatch(const std::vector<LOG_RECORD>& records) const {
        std::ostringstream oss;
        
        for (size_t i = 0; i < records.size(); ++i) {
            oss << format(records[i]);
            if (i < records.size() - 1) {
                oss << "\n";
            }
        }
        
        return oss.str();
    }
    
private:
    // Format timestamp to ISO 8601 format with milliseconds
    std::string formatTimestamp(
        const std::chrono::system_clock::time_point& timestamp
    ) const {
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
    
    // Format log level to uppercase string
    std::string formatLogLevel(LogLevel level) const {
        switch (level) {
            case LogLevel::DEBUG: return "DEBUG";
            case LogLevel::INFO:  return "INFO";
            case LogLevel::WARN:  return "WARN";
            case LogLevel::ERROR: return "ERROR";
            case LogLevel::FATAL: return "FATAL";
            default: return "UNKNOWN";
        }
    }
    
    // Format metadata as space-separated key=value pairs
    std::string formatMetadata(
        const std::unordered_map<std::string, std::string>& metadata
    ) const {
        std::ostringstream oss;
        bool first = true;
        
        for (const auto& [key, value] : metadata) {
            if (!first) {
                oss << " ";
            }
            oss << key << "=" << value;
            first = false;
        }
        
        return oss.str();
    }
};

} // namespace source
} // namespace stdlib
} // namespace ams

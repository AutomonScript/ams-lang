#pragma once

#include "log_record.hpp"
#include "error.hpp"
#include "pretty_printer.hpp"
#include <vector>
#include <regex>
#include <string>

namespace ams {
namespace stdlib {
namespace source {

// Component for regex-based pattern matching on log entries
class Pattern_Matcher {
public:
    Pattern_Matcher() = default;
    
    // Match operations
    // Returns a vector of LOG_RECORDs that match the regex pattern
    // Returns error if regex pattern is invalid
    Result<std::vector<LOG_RECORD>> match(
        const std::vector<LOG_RECORD>& records,
        const std::string& regex_pattern
    ) const {
        // Try to compile the regex pattern
        std::regex pattern;
        try {
            pattern = std::regex(regex_pattern, std::regex::ECMAScript);
        } catch (const std::regex_error& e) {
            return Result<std::vector<LOG_RECORD>>::Err(
                LogSourceError::INVALID_REGEX,
                "Invalid regex pattern: " + std::string(e.what())
            );
        }
        
        // Filter records that match the pattern
        std::vector<LOG_RECORD> result;
        result.reserve(records.size()); // Optimize for common case
        
        for (const auto& record : records) {
            if (matchesPattern(record, pattern)) {
                result.push_back(record);
            }
        }
        
        return Result<std::vector<LOG_RECORD>>::Ok(std::move(result));
    }
    
private:
    // Check if record matches the regex pattern
    // Pattern is matched against the entire log entry (timestamp, level, message, metadata)
    bool matchesPattern(
        const LOG_RECORD& record,
        const std::regex& pattern
    ) const {
        // Convert record to string format for matching
        std::string record_str = recordToString(record);
        
        // Perform case-sensitive regex matching
        return std::regex_search(record_str, pattern);
    }
    
    // Convert LOG_RECORD to string representation
    // Uses Pretty_Printer format: [TIMESTAMP] [LEVEL] MESSAGE [METADATA]
    std::string recordToString(const LOG_RECORD& record) const {
        Pretty_Printer printer;
        return printer.format(record);
    }
};

} // namespace source
} // namespace stdlib
} // namespace ams

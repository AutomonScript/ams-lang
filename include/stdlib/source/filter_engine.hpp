#pragma once

#include "log_record.hpp"
#include "filter_criteria.hpp"
#include <vector>
#include <algorithm>

namespace ams {
namespace stdlib {
namespace source {

// Component for filtering LOG_DATA collections based on criteria
class Filter_Engine {
public:
    Filter_Engine() = default;
    
    // Filter operations
    // Returns a new vector containing only records matching all criteria (AND logic)
    std::vector<LOG_RECORD> filter(
        const std::vector<LOG_RECORD>& records,
        const FilterCriteria& criteria
    ) const {
        std::vector<LOG_RECORD> result;
        result.reserve(records.size()); // Optimize for common case
        
        for (const auto& record : records) {
            if (matchesCriteria(record, criteria)) {
                result.push_back(record);
            }
        }
        
        return result;
    }
    
private:
    // Check if record matches the specified log level
    bool matchesLevel(const LOG_RECORD& record, LogLevel level) const {
        return record.level == level;
    }
    
    // Check if record timestamp is within the specified range (inclusive)
    bool matchesTimeRange(
        const LOG_RECORD& record,
        const std::chrono::system_clock::time_point& start,
        const std::chrono::system_clock::time_point& end
    ) const {
        return record.timestamp >= start && record.timestamp <= end;
    }
    
    // Check if record message contains the specified substring (case-sensitive)
    bool matchesMessage(
        const LOG_RECORD& record,
        const std::string& substring
    ) const {
        return record.message.find(substring) != std::string::npos;
    }
    
    // Check if record metadata contains all specified key-value pairs
    bool matchesMetadata(
        const LOG_RECORD& record,
        const std::unordered_map<std::string, std::string>& filters
    ) const {
        // All metadata filters must match (AND logic)
        for (const auto& [key, value] : filters) {
            auto it = record.metadata.find(key);
            if (it == record.metadata.end() || it->second != value) {
                return false;
            }
        }
        return true;
    }
    
    // Check if record matches all specified criteria (AND logic)
    bool matchesCriteria(
        const LOG_RECORD& record,
        const FilterCriteria& criteria
    ) const {
        // Check log level filter
        if (criteria.level.has_value()) {
            if (!matchesLevel(record, criteria.level.value())) {
                return false;
            }
        }
        
        // Check time range filter
        if (criteria.start_time.has_value() && criteria.end_time.has_value()) {
            if (!matchesTimeRange(record, criteria.start_time.value(), criteria.end_time.value())) {
                return false;
            }
        }
        
        // Check message substring filter
        if (criteria.message_substring.has_value()) {
            if (!matchesMessage(record, criteria.message_substring.value())) {
                return false;
            }
        }
        
        // Check metadata filters
        if (!criteria.metadata_filters.empty()) {
            if (!matchesMetadata(record, criteria.metadata_filters)) {
                return false;
            }
        }
        
        // All criteria matched
        return true;
    }
};

} // namespace source
} // namespace stdlib
} // namespace ams

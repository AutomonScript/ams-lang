#pragma once

#include "log_record.hpp"
#include <optional>
#include <chrono>
#include <string>
#include <unordered_map>

namespace ams {
namespace stdlib {
namespace source {

// Filtering criteria for log records
struct FilterCriteria {
    std::optional<LogLevel> level;
    std::optional<std::chrono::system_clock::time_point> start_time;
    std::optional<std::chrono::system_clock::time_point> end_time;
    std::optional<std::string> message_substring;
    std::unordered_map<std::string, std::string> metadata_filters;
    
    // Builder pattern for easy construction
    static FilterCriteria byLevel(LogLevel level) {
        FilterCriteria criteria;
        criteria.level = level;
        return criteria;
    }
    
    static FilterCriteria byTimeRange(
        std::chrono::system_clock::time_point start,
        std::chrono::system_clock::time_point end
    ) {
        FilterCriteria criteria;
        criteria.start_time = start;
        criteria.end_time = end;
        return criteria;
    }
    
    static FilterCriteria byMessage(const std::string& substring) {
        FilterCriteria criteria;
        criteria.message_substring = substring;
        return criteria;
    }
    
    // Fluent API for combining criteria (AND logic)
    FilterCriteria& andLevel(LogLevel lvl) {
        level = lvl;
        return *this;
    }
    
    FilterCriteria& andTimeRange(
        std::chrono::system_clock::time_point start,
        std::chrono::system_clock::time_point end
    ) {
        start_time = start;
        end_time = end;
        return *this;
    }
    
    FilterCriteria& andMessage(const std::string& substring) {
        message_substring = substring;
        return *this;
    }
    
    FilterCriteria& andMetadata(const std::string& key, const std::string& value) {
        metadata_filters[key] = value;
        return *this;
    }
};

} // namespace source
} // namespace stdlib
} // namespace ams

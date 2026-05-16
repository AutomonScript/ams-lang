#pragma once

#include "log_record.hpp"
#include "error.hpp"
#include <regex>
#include <sstream>
#include <vector>
#include <iostream>

namespace ams {
namespace stdlib {
namespace source {

// Parser for log file content
class Parser {
public:
    Parser() {
        // ISO 8601 timestamp pattern: 2024-01-15T10:30:45.123Z
        timestamp_pattern_ = std::regex(
            R"((\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}\.\d{3}Z))"
        );
        
        // Log level pattern: DEBUG, INFO, WARN, ERROR, FATAL
        log_level_pattern_ = std::regex(
            R"((DEBUG|INFO|WARN|ERROR|FATAL))"
        );
        
        // Full line pattern: [TIMESTAMP] [LEVEL] MESSAGE [METADATA]
        full_line_pattern_ = std::regex(
            R"(^(\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}\.\d{3}Z)\s+(DEBUG|INFO|WARN|ERROR|FATAL)\s+(.+?)(?:\s+(\S+=\S+(?:\s+\S+=\S+)*))?$)"
        );
    }
    
    // Parse entire content into LOG_RECORD vector
    Result<std::vector<LOG_RECORD>> parse(const std::string& content) const {
        std::vector<LOG_RECORD> records;
        std::istringstream stream(content);
        std::string line;
        int line_number = 0;
        
        while (std::getline(stream, line)) {
            line_number++;
            
            // Remove trailing carriage return if present (Windows line endings)
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            
            // Skip empty lines
            if (line.empty() || line.find_first_not_of(" \t\r\n") == std::string::npos) {
                continue;
            }
            
            auto result = parseLine(line);
            if (result.isOk()) {
                records.push_back(result.unwrap());
            } else {
                // Log warning but continue parsing
                std::cerr << "Warning: Failed to parse line " << line_number 
                         << ": " << line << std::endl;
            }
        }
        
        return Result<std::vector<LOG_RECORD>>::Ok(std::move(records));
    }
    
    // Parse single line into LOG_RECORD
    Result<LOG_RECORD> parseLine(const std::string& line) const {
        std::smatch match;
        
        if (!std::regex_match(line, match, full_line_pattern_)) {
            return Result<LOG_RECORD>::Err(
                LogSourceError::PARSE_ERROR,
                "Invalid log line format"
            );
        }
        
        // Extract components
        std::string timestamp_str = match[1].str();
        std::string level_str = match[2].str();
        std::string message = match[3].str();
        std::string metadata_str = match.size() > 4 ? match[4].str() : "";
        
        // Parse timestamp
        auto timestamp_result = parseTimestamp(timestamp_str);
        if (timestamp_result.isErr()) {
            return Result<LOG_RECORD>::Err(
                timestamp_result.getError(),
                timestamp_result.getErrorMessage()
            );
        }
        
        // Parse log level
        auto level_result = parseLogLevel(level_str);
        if (level_result.isErr()) {
            return Result<LOG_RECORD>::Err(
                level_result.getError(),
                level_result.getErrorMessage()
            );
        }
        
        // Parse metadata
        auto metadata_result = parseMetadata(metadata_str);
        if (metadata_result.isErr()) {
            return Result<LOG_RECORD>::Err(
                metadata_result.getError(),
                metadata_result.getErrorMessage()
            );
        }
        
        LOG_RECORD record{
            timestamp_result.unwrap(),
            level_result.unwrap(),
            message,
            metadata_result.unwrap()
        };
        
        return Result<LOG_RECORD>::Ok(std::move(record));
    }
    
    // Check if line is valid log format
    bool isValidLogLine(const std::string& line) const {
        return std::regex_match(line, full_line_pattern_);
    }
    
private:
    // Parse ISO 8601 timestamp
    Result<std::chrono::system_clock::time_point> parseTimestamp(
        const std::string& timestamp_str
    ) const {
        std::tm tm = {};
        int milliseconds = 0;
        
        // Parse: 2024-01-15T10:30:45.123Z
        std::istringstream ss(timestamp_str);
        ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
        
        if (ss.fail()) {
            return Result<std::chrono::system_clock::time_point>::Err(
                LogSourceError::PARSE_ERROR,
                "Failed to parse timestamp"
            );
        }
        
        // Parse milliseconds
        char dot;
        ss >> dot; // '.'
        ss >> milliseconds;
        
        // Convert to time_point
        auto time_t_val = std::mktime(&tm);
        auto tp = std::chrono::system_clock::from_time_t(time_t_val);
        tp += std::chrono::milliseconds(milliseconds);
        
        return Result<std::chrono::system_clock::time_point>::Ok(tp);
    }
    
    // Parse log level string
    Result<LogLevel> parseLogLevel(const std::string& level_str) const {
        if (level_str == "DEBUG") return Result<LogLevel>::Ok(LogLevel::DEBUG);
        if (level_str == "INFO")  return Result<LogLevel>::Ok(LogLevel::INFO);
        if (level_str == "WARN")  return Result<LogLevel>::Ok(LogLevel::WARN);
        if (level_str == "ERROR") return Result<LogLevel>::Ok(LogLevel::ERROR);
        if (level_str == "FATAL") return Result<LogLevel>::Ok(LogLevel::FATAL);
        
        return Result<LogLevel>::Err(
            LogSourceError::PARSE_ERROR,
            "Unknown log level: " + level_str
        );
    }
    
    // Parse metadata key=value pairs
    Result<std::unordered_map<std::string, std::string>> parseMetadata(
        const std::string& metadata_str
    ) const {
        std::unordered_map<std::string, std::string> metadata;
        
        if (metadata_str.empty()) {
            return Result<std::unordered_map<std::string, std::string>>::Ok(
                std::move(metadata)
            );
        }
        
        // Split by spaces and parse key=value pairs
        std::istringstream stream(metadata_str);
        std::string pair;
        
        while (stream >> pair) {
            size_t eq_pos = pair.find('=');
            if (eq_pos == std::string::npos) {
                continue; // Skip invalid pairs
            }
            
            std::string key = pair.substr(0, eq_pos);
            std::string value = pair.substr(eq_pos + 1);
            metadata[key] = value;
        }
        
        return Result<std::unordered_map<std::string, std::string>>::Ok(
            std::move(metadata)
        );
    }
    
    std::regex timestamp_pattern_;
    std::regex log_level_pattern_;
    std::regex full_line_pattern_;
};

} // namespace source
} // namespace stdlib
} // namespace ams

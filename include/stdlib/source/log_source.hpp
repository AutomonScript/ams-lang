#pragma once

#include "log_file.hpp"
#include "log_data.hpp"
#include "log_record.hpp"
#include "filter_criteria.hpp"
#include "parser.hpp"
#include "pretty_printer.hpp"
#include "error.hpp"
#include <memory>
#include <string>
#include <vector>
#include <chrono>

namespace ams {
namespace stdlib {
namespace source {

// Main interface for log file operations
class LOG_SOURCE {
public:
    // Factory method to open a log source
    static Result<LOG_SOURCE> open(const std::string& filepath, AccessMode mode) {
        // Open the underlying file
        auto file_result = LOG_FILE::open(filepath, mode);
        if (file_result.isErr()) {
            return Result<LOG_SOURCE>::Err(
                file_result.getError(),
                file_result.getErrorMessage()
            );
        }
        
        auto file = file_result.unwrap();
        return Result<LOG_SOURCE>::Ok(LOG_SOURCE(std::move(file), mode));
    }
    
    // Destructor (default, relies on unique_ptr cleanup)
    ~LOG_SOURCE() = default;
    
    // Disable copy, enable move
    LOG_SOURCE(const LOG_SOURCE&) = delete;
    LOG_SOURCE& operator=(const LOG_SOURCE&) = delete;
    LOG_SOURCE(LOG_SOURCE&&) noexcept = default;
    LOG_SOURCE& operator=(LOG_SOURCE&&) noexcept = default;
    
    // Core operations
    
    // Filter log records based on criteria
    LOG_DATA filter(const FilterCriteria& criteria) const {
        if (!isOpen()) {
            return LOG_DATA(); // Return empty LOG_DATA
        }
        
        // Read all content from file
        auto content_result = file_->readAll();
        if (content_result.isErr()) {
            return LOG_DATA(); // Return empty LOG_DATA on error
        }
        
        // Parse content into LOG_RECORDs
        Parser parser;
        auto parse_result = parser.parse(content_result.unwrap());
        if (parse_result.isErr()) {
            return LOG_DATA(); // Return empty LOG_DATA on parse error
        }
        
        // Create LOG_DATA and apply filter
        LOG_DATA data(std::move(parse_result.unwrap()));
        return data.filter(criteria);
    }
    
    // Match log records using regex pattern
    LOG_DATA match(const std::string& regex_pattern) const {
        if (!isOpen()) {
            return LOG_DATA(); // Return empty LOG_DATA
        }
        
        // Read all content from file
        auto content_result = file_->readAll();
        if (content_result.isErr()) {
            return LOG_DATA(); // Return empty LOG_DATA on error
        }
        
        // Parse content into LOG_RECORDs
        Parser parser;
        auto parse_result = parser.parse(content_result.unwrap());
        if (parse_result.isErr()) {
            return LOG_DATA(); // Return empty LOG_DATA on parse error
        }
        
        // Create LOG_DATA and apply match
        LOG_DATA data(std::move(parse_result.unwrap()));
        return data.match(regex_pattern);
    }
    
    // Check if file has been updated since last check
    bool hasUpdated() const {
        if (!isOpen()) {
            return false;
        }
        
        // On first call, just initialize and return false
        if (!has_checked_) {
            has_checked_ = true;
            last_check_time_ = std::chrono::system_clock::now();
            last_read_position_ = file_->getSize();
            return false;
        }
        
        // Check if file has changed
        bool changed = file_->hasChanged();
        
        if (changed) {
            // Update last check time
            last_check_time_ = std::chrono::system_clock::now();
            
            // Update last read position to current file size
            last_read_position_ = file_->getSize();
            
            // Update file metadata so next call doesn't report the same change
            file_->updateMetadata();
        }
        
        return changed;
    }
    
    // Insert a single log record (WRITE mode only)
    Result<void> insert(const LOG_RECORD& record) {
        if (!isOpen()) {
            return Result<void>::Err(
                LogSourceError::IO_ERROR,
                "LOG_SOURCE is not open"
            );
        }
        
        if (mode_ != AccessMode::WRITE) {
            return Result<void>::Err(
                LogSourceError::WRITE_NOT_PERMITTED,
                "Cannot insert records in READ mode"
            );
        }
        
        // Format the record
        Pretty_Printer printer;
        std::string formatted = printer.format(record) + "\n";
        
        // Append to file
        auto append_result = file_->append(formatted);
        if (append_result.isErr()) {
            return append_result;
        }
        
        // Flush immediately to ensure durability
        return file_->flush();
    }
    
    // Insert multiple log records (WRITE mode only)
    Result<void> insert(const std::vector<LOG_RECORD>& records) {
        if (!isOpen()) {
            return Result<void>::Err(
                LogSourceError::IO_ERROR,
                "LOG_SOURCE is not open"
            );
        }
        
        if (mode_ != AccessMode::WRITE) {
            return Result<void>::Err(
                LogSourceError::WRITE_NOT_PERMITTED,
                "Cannot insert records in READ mode"
            );
        }
        
        // Format all records
        Pretty_Printer printer;
        std::string formatted = printer.formatBatch(records);
        if (!formatted.empty() && formatted.back() != '\n') {
            formatted += "\n";
        }
        
        // Append to file
        auto append_result = file_->append(formatted);
        if (append_result.isErr()) {
            return append_result;
        }
        
        // Flush immediately to ensure durability
        return file_->flush();
    }
    
    // Metadata
    
    std::string getFilePath() const {
        if (!isOpen()) {
            return filepath_;
        }
        return file_->getFilePath();
    }
    
    AccessMode getMode() const {
        return mode_;
    }
    
    size_t getRecordCount() const {
        if (!isOpen()) {
            return 0;
        }
        
        // Read all content and parse to count records
        auto content_result = file_->readAll();
        if (content_result.isErr()) {
            return 0;
        }
        
        Parser parser;
        auto parse_result = parser.parse(content_result.unwrap());
        if (parse_result.isErr()) {
            return 0;
        }
        
        return parse_result.unwrap().size();
    }
    
    // Resource management
    
    Result<void> close() {
        if (!isOpen()) {
            return Result<void>::Err(
                LogSourceError::IO_ERROR,
                "LOG_SOURCE is already closed"
            );
        }
        
        // Reset the file pointer (closes the file)
        file_.reset();
        
        return Result<void>::Ok();
    }
    
    bool isOpen() const {
        return file_ != nullptr;
    }
    
private:
    // Private constructor
    LOG_SOURCE(std::unique_ptr<LOG_FILE> file, AccessMode mode)
        : file_(std::move(file))
        , mode_(mode)
        , filepath_(file_ ? file_->getFilePath() : "")
        , last_read_position_(0)
        , last_check_time_(std::chrono::system_clock::now())
        , has_checked_(false)
    {
    }
    
    std::unique_ptr<LOG_FILE> file_;
    AccessMode mode_;
    std::string filepath_;
    mutable size_t last_read_position_;
    mutable std::chrono::system_clock::time_point last_check_time_;
    mutable bool has_checked_;
};

} // namespace source
} // namespace stdlib
} // namespace ams

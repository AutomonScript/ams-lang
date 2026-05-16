#pragma once

#include "log_record.hpp"
#include "filter_criteria.hpp"
#include "filter_engine.hpp"
#include "pattern_matcher.hpp"
#include "error.hpp"
#include <vector>
#include <stdexcept>

namespace ams {
namespace stdlib {
namespace source {

// Polymorphic container that can hold single or multiple LOG_RECORDs
class LOG_DATA {
public:
    // Type alias for iterator
    using iterator = std::vector<LOG_RECORD>::const_iterator;
    
    // Constructors
    LOG_DATA() : type_(ContainerType::EMPTY), records_() {}
    
    explicit LOG_DATA(const LOG_RECORD& single_record) 
        : type_(ContainerType::SINGLE), records_{single_record} {}
    
    explicit LOG_DATA(std::vector<LOG_RECORD>&& records) 
        : records_(std::move(records)) {
        updateType();
    }
    
    // Container queries
    size_t count() const {
        return records_.size();
    }
    
    bool isSingle() const {
        return type_ == ContainerType::SINGLE;
    }
    
    bool isEmpty() const {
        return type_ == ContainerType::EMPTY;
    }
    
    // Access operations
    const LOG_RECORD& operator[](size_t index) const {
        if (index >= records_.size()) {
            throw std::out_of_range("LOG_DATA index out of range");
        }
        return records_[index];
    }
    
    const LOG_RECORD& first() const {
        if (isEmpty()) {
            throw std::runtime_error("Cannot access first() on empty LOG_DATA");
        }
        return records_[0];
    }
    
    std::vector<LOG_RECORD> toVector() const {
        return records_;
    }
    
    // Iteration support
    iterator begin() const {
        return records_.begin();
    }
    
    iterator end() const {
        return records_.end();
    }
    
    // Conversion operators
    explicit operator bool() const {
        return !isEmpty();
    }
    
    // Filtering and matching - returns new LOG_DATA with filtered/matched records
    LOG_DATA filter(const FilterCriteria& criteria) const {
        Filter_Engine engine;
        auto filtered_records = engine.filter(records_, criteria);
        return LOG_DATA(std::move(filtered_records));
    }
    
    LOG_DATA match(const std::string& regex_pattern) const {
        Pattern_Matcher matcher;
        auto result = matcher.match(records_, regex_pattern);
        
        // If pattern matching fails (invalid regex), return empty LOG_DATA
        if (result.isErr()) {
            return LOG_DATA();
        }
        
        auto matched_records = result.unwrap();
        return LOG_DATA(std::move(matched_records));
    }
    
private:
    enum class ContainerType {
        EMPTY,
        SINGLE,
        MULTIPLE
    };
    
    void updateType() {
        if (records_.empty()) {
            type_ = ContainerType::EMPTY;
        } else if (records_.size() == 1) {
            type_ = ContainerType::SINGLE;
        } else {
            type_ = ContainerType::MULTIPLE;
        }
    }
    
    ContainerType type_;
    std::vector<LOG_RECORD> records_;
};

} // namespace source
} // namespace stdlib
} // namespace ams

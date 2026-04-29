#pragma once
#include "snapshot.hpp"
#include <map>
#include <mutex>
#include <variant>
#include <string>

namespace ams {

class VariableRegistry {
public:
    VariableRegistry() = default;

    // Set or update a variable of ANY supported type using std::variant
    void setValue(const std::string& sourceName, const std::string& varName, const Snapshot::Value& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        state_[sourceName][varName] = value;
    }

    // Retrieve a thread-safe, point-in-time copy of a source's state
    Snapshot getSnapshot(const std::string& sourceName) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = state_.find(sourceName);
        if (it != state_.end()) {
            return Snapshot(it->second);
        }
        return Snapshot(); // Return empty snapshot if source doesn't exist yet
    }

    bool hasVariable(const std::string& sourceName, const std::string& varName) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto srcIt = state_.find(sourceName);
        if (srcIt != state_.end()) {
            return srcIt->second.count(varName) > 0;
        }
        return false;
    }

private:
    // state_[SourceName][VariableName] = Value
    std::map<std::string, std::map<std::string, Snapshot::Value>> state_;
    
    // mutable allows locking within const functions like getSnapshot()
    mutable std::mutex mutex_; 
};

} // namespace ams
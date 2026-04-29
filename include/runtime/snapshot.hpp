#pragma once
#include <string>
#include <map>
#include <variant>
#include <stdexcept>

namespace ams {

class Snapshot {
public:
    // Unified type definition for all variable types in AutomonScript
    using Value = std::variant<int, double, std::string, bool>;

    Snapshot() = default;
    Snapshot(const std::map<std::string, Value>& data) : data_(data) {}

    bool has(const std::string& key) const {
        return data_.count(key) > 0;
    }

    int getInt(const std::string& key) const {
        auto it = data_.find(key);
        if (it != data_.end()) {
            if (auto val = std::get_if<int>(&it->second)) return *val;
            throw std::runtime_error("Type mismatch: Variable '" + key + "' is not an INT.");
        }
        return 0; // Default if not found
    }

    double getDouble(const std::string& key) const {
        auto it = data_.find(key);
        if (it != data_.end()) {
            if (auto val = std::get_if<double>(&it->second)) return *val;
            // Graceful promotion: If they ask for a double but it's an int, allow it
            if (auto val = std::get_if<int>(&it->second)) return static_cast<double>(*val);
            throw std::runtime_error("Type mismatch: Variable '" + key + "' is not a FLOAT.");
        }
        return 0.0;
    }

    std::string getString(const std::string& key) const {
        auto it = data_.find(key);
        if (it != data_.end()) {
            if (auto val = std::get_if<std::string>(&it->second)) return *val;
            throw std::runtime_error("Type mismatch: Variable '" + key + "' is not a STRING.");
        }
        return "";
    }

    bool getBool(const std::string& key) const {
        auto it = data_.find(key);
        if (it != data_.end()) {
            if (auto val = std::get_if<bool>(&it->second)) return *val;
            throw std::runtime_error("Type mismatch: Variable '" + key + "' is not a BOOL.");
        }
        return false;
    }

    const std::map<std::string, Value>& getRawData() const {
        return data_;
    }

private:
    std::map<std::string, Value> data_;
};

} // namespace ams
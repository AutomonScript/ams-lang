#pragma once
#include <iostream>
#include <string>
#include <mutex>
#include <atomic> // Added for lock-free thread safety

namespace ams {

enum class Verbosity {
    NONE = 0,      // No logging
    USER = 1,      // User-level verbose
    DEV = 2        // Developer-level verbose
};

class Logger {
public:
    static Logger& instance() {
        static Logger instance;
        return instance;
    }

    void setVerbosity(Verbosity v) {
        // Atomic store, no mutex needed just to update the level
        verbosity_.store(v, std::memory_order_relaxed);
    }

    Verbosity getVerbosity() const {
        // Atomic load
        return verbosity_.load(std::memory_order_relaxed);
    }

    template<typename... Args>
    void dev(const std::string& tag, Args&&... args) {
        if (verbosity_.load(std::memory_order_relaxed) >= Verbosity::DEV) {
            std::lock_guard<std::mutex> lock(mutex_);
            std::cout << "[DEV] [" << tag << "] ";
            (std::cout << ... << args);
            std::cout << std::endl;
        }
    }

    template<typename... Args>
    void user(const std::string& tag, Args&&... args) {
        if (verbosity_.load(std::memory_order_relaxed) >= Verbosity::USER) {
            std::lock_guard<std::mutex> lock(mutex_);
            std::cout << "[USER] [" << tag << "] ";
            (std::cout << ... << args);
            std::cout << std::endl;
        }
    }

    template<typename... Args>
    void error(const std::string& tag, Args&&... args) {
        // Errors usually print regardless of verbosity, but we still lock to prevent text garbling
        std::lock_guard<std::mutex> lock(mutex_);
        std::cerr << "[ERROR] [" << tag << "] ";
        (std::cerr << ... << args);
        std::cerr << std::endl;
    }

    template<typename... Args>
    void warn(const std::string& tag, Args&&... args) {
        if (verbosity_.load(std::memory_order_relaxed) >= Verbosity::USER) {
            std::lock_guard<std::mutex> lock(mutex_);
            std::cout << "[WARN] [" << tag << "] ";
            (std::cout << ... << args);
            std::cout << std::endl;
        }
    }

private:
    Logger() : verbosity_(Verbosity::NONE) {}
    ~Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    std::atomic<Verbosity> verbosity_; // Now thread-safe for lock-free reading!
    mutable std::mutex mutex_;         // Keeps console output synchronized
};

// Convenience macros
#define AMS_DEV_LOG(tag, ...) ams::Logger::instance().dev(tag, __VA_ARGS__)
#define AMS_USER_LOG(tag, ...) ams::Logger::instance().user(tag, __VA_ARGS__)
#define AMS_ERROR_LOG(tag, ...) ams::Logger::instance().error(tag, __VA_ARGS__)
#define AMS_WARN_LOG(tag, ...) ams::Logger::instance().warn(tag, __VA_ARGS__)

} // namespace ams
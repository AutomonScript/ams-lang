#pragma once
#include "logger.hpp"
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <chrono>
#include <vector>
#include <memory>
#include <atomic>
#include <algorithm>

namespace ams {

struct ScheduledTask {
    std::string name;
    std::function<void()> task;
    std::chrono::steady_clock::time_point nextRun;
    std::chrono::milliseconds interval;
    bool repeating;
    bool cancelled = false;
};

class Scheduler {
public:
    Scheduler(size_t numWorkers = 4) : stop_(false) {
        for (size_t i = 0; i < numWorkers; ++i) {
            workers_.emplace_back(&Scheduler::workerLoop, this);
        }
        schedulerThread_ = std::thread(&Scheduler::scheduleLoop, this);
        AMS_DEV_LOG("Scheduler", "Initialized with ", numWorkers, " workers");
    }

    ~Scheduler() {
        shutdown();
    }

    void shutdown() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            stop_ = true;
        }
        cv_.notify_all();
        if (schedulerThread_.joinable()) schedulerThread_.join();
        for (auto& w : workers_) {
            if (w.joinable()) w.join();
        }
    }

    void schedulePeriodic(const std::string& name, int intervalMs, std::function<void()> task) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto now = std::chrono::steady_clock::now();
        tasks_.push_back({
            name, task, now + std::chrono::milliseconds(intervalMs),
            std::chrono::milliseconds(intervalMs), true, false
        });
        cv_.notify_one(); // Wake up scheduler to recalculate sleep time
    }

    void scheduleOnce(const std::string& name, int delayMs, std::function<void()> task) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto now = std::chrono::steady_clock::now();
        tasks_.push_back({
            name, task, now + std::chrono::milliseconds(delayMs),
            std::chrono::milliseconds(delayMs), false, false
        });
        cv_.notify_one();
    }

    void runContinuously(const std::string& name, std::function<void()> task) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto now = std::chrono::steady_clock::now();
        tasks_.push_back({
            name, task, now, std::chrono::milliseconds(0), true, false
        });
        cv_.notify_one();
    }

private:
    std::vector<ScheduledTask> tasks_;
    std::vector<std::thread> workers_;
    std::thread schedulerThread_;
    std::mutex mutex_;
    std::condition_variable cv_;
    std::atomic<bool> stop_;
    std::queue<std::function<void()>> taskQueue_;

    void workerLoop() {
        while (!stop_) {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(mutex_);
                cv_.wait(lock, [this] { return stop_ || !taskQueue_.empty(); });
                if (stop_ && taskQueue_.empty()) return;
                
                if (!taskQueue_.empty()) {
                    task = std::move(taskQueue_.front());
                    taskQueue_.pop();
                }
            }
            if (task) {
                try {
                    task();
                } catch (const std::exception& e) {
                    AMS_ERROR_LOG("Scheduler", "Task error: ", e.what());
                }
            }
        }
    }

    void scheduleLoop() {
        while (!stop_) {
            std::unique_lock<std::mutex> lock(mutex_);
            
            // 1. Calculate the exact next wake-up time
            auto nextWake = std::chrono::steady_clock::time_point::max();
            for (const auto& t : tasks_) {
                if (!t.cancelled && t.nextRun < nextWake) {
                    nextWake = t.nextRun;
                }
            }

            // 2. Sleep until precisely that time (or until a new task wakes us up)
            if (nextWake == std::chrono::steady_clock::time_point::max()) {
                cv_.wait(lock, [this] { return stop_.load() || !tasks_.empty(); });
            } else {
                cv_.wait_until(lock, nextWake, [this] { return stop_.load(); });
            }
            
            if (stop_) break;

            // 3. Queue due tasks
            auto now = std::chrono::steady_clock::now();
            for (auto& t : tasks_) {
                if (t.cancelled) continue;
                if (now >= t.nextRun) {
                    taskQueue_.push(t.task);
                    if (t.repeating) {
                        t.nextRun = now + t.interval;
                    } else {
                        t.cancelled = true; // Mark for cleanup
                    }
                }
            }
            
            // 4. Clean up memory (Remove cancelled tasks)
            tasks_.erase(
                std::remove_if(tasks_.begin(), tasks_.end(), [](const ScheduledTask& t) { return t.cancelled; }),
                tasks_.end()
            );

            cv_.notify_all(); // Wake up workers
        }
    }
};

} // namespace ams
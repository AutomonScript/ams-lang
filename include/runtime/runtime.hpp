#pragma once
#include "logger.hpp"
#include "snapshot.hpp"
#include "event_bus.hpp"
#include "scheduler.hpp"
#include "variable_registry.hpp"
#include <string>
#include <functional>
#include <chrono>

namespace ams {

class Runtime {
public:
    Runtime() : scheduler_(4) {} // Initialize with 4 worker threads

    ~Runtime() {
        shutdown();
    }

    void init() {
        AMS_USER_LOG("Runtime", "AutomonScript Engine Initialized.");
    }

    void shutdown() {
        AMS_USER_LOG("Runtime", "Shutting down engine...");
        scheduler_.shutdown();
    }

    // ================= State Management =================
    template <typename T>
    void updateVar(const std::string& src, const std::string& var, T val) {
        registry_.setValue(src, var, val);

        EventPayload payload;
        payload.sourceName = src;
        payload.eventName = "SYS_VAR_UPDATED";
        payload.timestamp = std::chrono::system_clock::now().time_since_epoch().count();
        
        eventBus_.publish("SYS_VAR_UPDATED", payload);
    }

    Snapshot getSnapshot(const std::string& src) const {
        return registry_.getSnapshot(src);
    }

    // ================= Scheduling =================
    void schedulePeriodic(const std::string& name, int ms, const std::string& unit, std::function<void()> task) {
        int interval = convertMs(ms, unit);
        scheduler_.schedulePeriodic(name, interval, task);
    }

    void scheduleOnce(const std::string& name, int ms, std::function<void()> task) {
        scheduler_.scheduleOnce(name, ms, task);
    }

    void runContinuously(const std::string& name, std::function<void()> task) {
        scheduler_.runContinuously(name, task);
    }

    // ================= Events & Observers =================
    void registerEvent(const std::string& evt, const std::string& src) {
        AMS_DEV_LOG("Runtime", "Registered Event: ", evt, " on Source: ", src);
    }

    void setSignalCondition(const std::string& evt, const std::string& src, std::function<bool(const Snapshot&)> cond) {
        eventBus_.subscribe("SYS_VAR_UPDATED", "cond_" + evt, [this, evt, src, cond](const EventPayload& p) {
            if (p.sourceName != src) return;

            Snapshot snap = getSnapshot(src);
            if (cond(snap)) {
                EventPayload userEvent;
                userEvent.sourceName = src;
                userEvent.eventName = evt;
                userEvent.timestamp = std::chrono::system_clock::now().time_since_epoch().count();
                eventBus_.publish(evt, userEvent);
            }
        });
    }

    void registerObserver(const std::string& obs, const std::string& evt, std::function<void(const Snapshot&)> handler) {
        eventBus_.subscribe(evt, obs, [this, handler](const EventPayload& p) {
            Snapshot snap = getSnapshot(p.sourceName);
            handler(snap);
        });
    }

private:
    VariableRegistry registry_;
    EventBus eventBus_;
    Scheduler scheduler_;

    int convertMs(int val, const std::string& u) {
        std::string unit = u;
        for (auto& c : unit) c = toupper(c);

        if (unit == "MS") return val;
        if (unit == "SEC" || unit == "S") return val * 1000;
        if (unit == "MIN" || unit == "M") return val * 60000;
        if (unit == "HOUR" || unit == "H") return val * 3600000;
        return val;
    }
};

} // namespace ams
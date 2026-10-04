/*
 * LTempGuard - Linux-Based IoT Temperature Monitoring and Alert System
 * File: TemperatureMonitor.hpp
 * Description: Core periodic monitoring engine orchestrating sensor polling,
 *              analysis, alerts, and logging.
 * Author: LTempGuard Engineering Team
 */

#ifndef LTEMPGUARD_TEMPERATURE_MONITOR_HPP
#define LTEMPGUARD_TEMPERATURE_MONITOR_HPP

#include "device/TemperatureDevice.hpp"
#include "analysis/TemperatureAnalyzer.hpp"
#include "alert/AlertManager.hpp"
#include "logging/Logger.hpp"

#include <chrono>
#include <atomic>
#include <functional>
#include <optional>

namespace ltempguard {

struct TelemetrySample {
    double temperatureC{0.0};
    MonitoringState state{MonitoringState::NORMAL};
    bool deviceOnline{false};
    std::chrono::system_clock::time_point timestamp{};
    bool transitionOccurred{false};
    std::string message{};
};

/**
 * TemperatureMonitor - Autonomous engine polling device, executing state evaluations,
 * and dispatching alerts and log entries.
 */
class TemperatureMonitor {
public:
    using TelemetryCallback = std::function<void(const TelemetrySample&)>;

    TemperatureMonitor(ITemperatureDevice& device,
                       TemperatureAnalyzer& analyzer,
                       AlertManager& alertManager,
                       Logger& logger);
    ~TemperatureMonitor();

    // Single non-blocking polling cycle
    TelemetrySample pollOnce();

    // Start continuous monitoring in the current thread (or until stop() requested)
    void run(std::chrono::milliseconds interval,
             std::function<bool()> shouldContinuePredicate,
             TelemetryCallback onSample = nullptr);

    void requestStop() noexcept;
    [[nodiscard]] bool isRunning() const noexcept;

private:
    ITemperatureDevice& device_;
    TemperatureAnalyzer& analyzer_;
    AlertManager& alertManager_;
    Logger& logger_;

    std::atomic<bool> isRunning_{false};
};

} // namespace ltempguard

#endif // LTEMPGUARD_TEMPERATURE_MONITOR_HPP

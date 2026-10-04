/*
 * LTempGuard - Linux-Based IoT Temperature Monitoring and Alert System
 * File: AlertManager.hpp
 * Description: Edge-triggered alert dispatch and subscriber management.
 * Author: LTempGuard Engineering Team
 */

#ifndef LTEMPGUARD_ALERT_MANAGER_HPP
#define LTEMPGUARD_ALERT_MANAGER_HPP

#include "analysis/TemperatureAnalyzer.hpp"
#include <string>
#include <vector>
#include <functional>
#include <chrono>

namespace ltempguard {

enum class AlertSeverity {
    INFO,
    WARNING,
    CRITICAL
};

[[nodiscard]] std::string_view toString(AlertSeverity severity) noexcept;

struct AlertEvent {
    AlertSeverity severity{AlertSeverity::INFO};
    MonitoringState fromState{MonitoringState::NORMAL};
    MonitoringState toState{MonitoringState::NORMAL};
    double temperatureC{0.0};
    std::chrono::system_clock::time_point timestamp{};
    std::string message{};
    std::string formattedBanner{};
};

/**
 * AlertManager - Coordinates alert generation, deduplication, and subscriber dispatching.
 */
class AlertManager {
public:
    using AlertCallback = std::function<void(const AlertEvent&)>;

    AlertManager() = default;

    void subscribe(AlertCallback callback);
    void processTransition(const StateTransitionResult& transition);

    [[nodiscard]] const std::vector<AlertEvent>& getAlertHistory() const noexcept;
    void clearHistory() noexcept;

    static std::string formatTerminalBanner(const AlertEvent& event);

private:
    std::vector<AlertCallback> subscribers_;
    std::vector<AlertEvent> alertHistory_;

    void dispatch(const AlertEvent& event);
};

} // namespace ltempguard

#endif // LTEMPGUARD_ALERT_MANAGER_HPP

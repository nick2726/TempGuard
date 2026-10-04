/*
 * LTempGuard - Linux-Based IoT Temperature Monitoring and Alert System
 * File: AlertManager.cpp
 * Description: Implementation of alert generation, deduplication, and banner formatting.
 * Author: LTempGuard Engineering Team
 */

#include "alert/AlertManager.hpp"
#include <sstream>
#include <iomanip>
#include <ctime>

namespace ltempguard {

std::string_view toString(AlertSeverity severity) noexcept
{
    switch (severity) {
    case AlertSeverity::INFO:     return "INFO";
    case AlertSeverity::WARNING:  return "WARNING";
    case AlertSeverity::CRITICAL: return "CRITICAL";
    }
    return "UNKNOWN";
}

void AlertManager::subscribe(AlertCallback callback)
{
    subscribers_.push_back(std::move(callback));
}

void AlertManager::processTransition(const StateTransitionResult& transition)
{
    // Deduplication rule: Only dispatch an alert if the state actually changed!
    if (!transition.stateChanged) {
        return;
    }

    AlertEvent event;
    event.fromState = transition.previousState;
    event.toState = transition.currentState;
    event.temperatureC = transition.temperature;
    event.timestamp = transition.timestamp;
    event.message = transition.description;

    switch (transition.currentState) {
    case MonitoringState::CRITICAL:
        event.severity = AlertSeverity::CRITICAL;
        break;
    case MonitoringState::WARNING:
        event.severity = AlertSeverity::WARNING;
        break;
    case MonitoringState::NORMAL:
    case MonitoringState::FAULT:
    default:
        event.severity = AlertSeverity::INFO;
        break;
    }

    event.formattedBanner = formatTerminalBanner(event);
    dispatch(event);
}

void AlertManager::dispatch(const AlertEvent& event)
{
    alertHistory_.push_back(event);
    for (const auto& cb : subscribers_) {
        if (cb) {
            cb(event);
        }
    }
}

const std::vector<AlertEvent>& AlertManager::getAlertHistory() const noexcept
{
    return alertHistory_;
}

void AlertManager::clearHistory() noexcept
{
    alertHistory_.clear();
}

std::string AlertManager::formatTerminalBanner(const AlertEvent& event)
{
    std::ostringstream oss;
    std::string colorCode;
    std::string resetCode = "\033[0m";

    switch (event.severity) {
    case AlertSeverity::CRITICAL:
        colorCode = "\033[1;31m"; // Bold Red
        break;
    case AlertSeverity::WARNING:
        colorCode = "\033[1;33m"; // Bold Yellow
        break;
    case AlertSeverity::INFO:
    default:
        colorCode = "\033[1;32m"; // Bold Green
        break;
    }

    oss << colorCode
        << "============================================================\n"
        << "  [ALERT: " << toString(event.severity) << "] "
        << toString(event.fromState) << " -> " << toString(event.toState) << "\n"
        << "  Temperature : " << std::fixed << std::setprecision(1) << event.temperatureC << " °C\n"
        << "  Message     : " << event.message << "\n"
        << "============================================================"
        << resetCode << "\n";

    return oss.str();
}

} // namespace ltempguard

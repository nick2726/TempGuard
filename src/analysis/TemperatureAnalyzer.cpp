/*
 * LTempGuard - Linux-Based IoT Temperature Monitoring and Alert System
 * File: TemperatureAnalyzer.cpp
 * Description: Implementation of thermal state classification and transition engine.
 * Author: LTempGuard Engineering Team
 */

#include "analysis/TemperatureAnalyzer.hpp"
#include <sstream>
#include <iomanip>

namespace ltempguard {

std::string_view toString(MonitoringState state) noexcept
{
    switch (state) {
    case MonitoringState::NORMAL:   return "NORMAL";
    case MonitoringState::WARNING:  return "WARNING";
    case MonitoringState::CRITICAL: return "CRITICAL";
    case MonitoringState::FAULT:    return "FAULT";
    }
    return "UNKNOWN";
}

std::ostream& operator<<(std::ostream& os, MonitoringState state)
{
    return os << toString(state);
}

TemperatureAnalyzer::TemperatureAnalyzer(ThresholdConfig config)
    : thresholds_(config)
{
    if (!thresholds_.isValid()) {
        thresholds_ = ThresholdConfig{};
    }
}

MonitoringState TemperatureAnalyzer::classify(double temperatureC) const noexcept
{
    // Physical sensor sanity bounds check
    if (temperatureC < -50.0 || temperatureC > 150.0) {
        return MonitoringState::FAULT;
    }

    if (temperatureC >= thresholds_.criticalThresholdC) {
        return MonitoringState::CRITICAL;
    }
    if (temperatureC >= thresholds_.warningThresholdC) {
        return MonitoringState::WARNING;
    }
    return MonitoringState::NORMAL;
}

StateTransitionResult TemperatureAnalyzer::evaluate(double temperatureC)
{
    MonitoringState newState = classify(temperatureC);
    bool changed = (newState != currentState_);

    StateTransitionResult result;
    result.temperature = temperatureC;
    result.previousState = currentState_;
    result.currentState = newState;
    result.stateChanged = changed;
    result.timestamp = std::chrono::system_clock::now();

    if (changed) {
        std::ostringstream oss;
        oss << "Transition from " << toString(currentState_)
            << " to " << toString(newState)
            << " at " << std::fixed << std::setprecision(1) << temperatureC << " °C";

        if (newState == MonitoringState::CRITICAL) {
            oss << " (EXCEEDED CRITICAL THRESHOLD " << thresholds_.criticalThresholdC << " °C)";
        } else if (newState == MonitoringState::WARNING) {
            oss << " (EXCEEDED WARNING THRESHOLD " << thresholds_.warningThresholdC << " °C)";
        } else if (newState == MonitoringState::NORMAL) {
            oss << " (RECOVERED BELOW WARNING THRESHOLD)";
        } else if (newState == MonitoringState::FAULT) {
            oss << " (SENSOR READING OUT OF VALID BOUNDS)";
        }

        result.description = oss.str();
        currentState_ = newState;
    } else {
        std::ostringstream oss;
        oss << "Steady state " << toString(currentState_)
            << " at " << std::fixed << std::setprecision(1) << temperatureC << " °C";
        result.description = oss.str();
    }

    return result;
}

bool TemperatureAnalyzer::setThresholds(ThresholdConfig config)
{
    if (!config.isValid()) {
        return false;
    }
    thresholds_ = config;
    return true;
}

ThresholdConfig TemperatureAnalyzer::getThresholds() const noexcept
{
    return thresholds_;
}

MonitoringState TemperatureAnalyzer::getCurrentState() const noexcept
{
    return currentState_;
}

void TemperatureAnalyzer::reset(MonitoringState initialState) noexcept
{
    currentState_ = initialState;
}

} // namespace ltempguard

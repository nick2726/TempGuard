/*
 * LTempGuard - Linux-Based IoT Temperature Monitoring and Alert System
 * File: TemperatureAnalyzer.hpp
 * Description: High-precision thermal state classification and transition engine.
 * Author: LTempGuard Engineering Team
 */

#ifndef LTEMPGUARD_TEMPERATURE_ANALYZER_HPP
#define LTEMPGUARD_TEMPERATURE_ANALYZER_HPP

#include <string>
#include <string_view>
#include <chrono>
#include <optional>

namespace ltempguard {

enum class MonitoringState {
    NORMAL,
    WARNING,
    CRITICAL,
    FAULT
};

[[nodiscard]] std::string_view toString(MonitoringState state) noexcept;
std::ostream& operator<<(std::ostream& os, MonitoringState state);

struct ThresholdConfig {
    double warningThresholdC{40.0};
    double criticalThresholdC{60.0};

    [[nodiscard]] bool isValid() const noexcept {
        return warningThresholdC < criticalThresholdC &&
               warningThresholdC >= -50.0 &&
               criticalThresholdC <= 150.0;
    }
};

struct StateTransitionResult {
    double temperature{0.0};
    MonitoringState previousState{MonitoringState::NORMAL};
    MonitoringState currentState{MonitoringState::NORMAL};
    bool stateChanged{false};
    std::chrono::system_clock::time_point timestamp{};
    std::string description{};
};

/**
 * TemperatureAnalyzer - Evaluates temperature readings against configurable thresholds,
 * classifies operational states, and identifies edge-triggered transitions.
 */
class TemperatureAnalyzer {
public:
    explicit TemperatureAnalyzer(ThresholdConfig config = ThresholdConfig{});

    [[nodiscard]] MonitoringState classify(double temperatureC) const noexcept;
    StateTransitionResult evaluate(double temperatureC);

    bool setThresholds(ThresholdConfig config);
    [[nodiscard]] ThresholdConfig getThresholds() const noexcept;
    [[nodiscard]] MonitoringState getCurrentState() const noexcept;
    void reset(MonitoringState initialState = MonitoringState::NORMAL) noexcept;

private:
    ThresholdConfig thresholds_;
    MonitoringState currentState_{MonitoringState::NORMAL};
};

} // namespace ltempguard

#endif // LTEMPGUARD_TEMPERATURE_ANALYZER_HPP

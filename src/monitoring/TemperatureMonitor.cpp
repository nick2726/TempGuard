/*
 * LTempGuard - Linux-Based IoT Temperature Monitoring and Alert System
 * File: TemperatureMonitor.cpp
 * Description: Implementation of periodic temperature sampling and event dispatching.
 * Author: LTempGuard Engineering Team
 */

#include "monitoring/TemperatureMonitor.hpp"

#include <thread>
#include <iostream>

namespace ltempguard {

TemperatureMonitor::TemperatureMonitor(ITemperatureDevice& device,
                                       TemperatureAnalyzer& analyzer,
                                       AlertManager& alertManager,
                                       Logger& logger)
    : device_(device)
    , analyzer_(analyzer)
    , alertManager_(alertManager)
    , logger_(logger)
{
}

TemperatureMonitor::~TemperatureMonitor()
{
    requestStop();
}

void TemperatureMonitor::requestStop() noexcept
{
    isRunning_ = false;
}

bool TemperatureMonitor::isRunning() const noexcept
{
    return isRunning_;
}

TelemetrySample TemperatureMonitor::pollOnce()
{
    TelemetrySample sample;
    sample.timestamp = std::chrono::system_clock::now();

    auto maybeTemp = device_.readTemperature();
    if (!maybeTemp.has_value()) {
        sample.deviceOnline = false;
        sample.state = MonitoringState::FAULT;
        sample.message = "Device offline or read error";

        LogRecord rec;
        rec.timestamp = sample.timestamp;
        rec.temperatureC = 0.0;
        rec.previousState = analyzer_.getCurrentState();
        rec.currentState = MonitoringState::FAULT;
        rec.eventType = "DEVICE_FAULT";
        rec.message = "Failed to read from " + device_.getDevicePath();
        logger_.logEvent(rec);

        return sample;
    }

    sample.deviceOnline = true;
    sample.temperatureC = maybeTemp.value();

    // Evaluate state machine
    StateTransitionResult transition = analyzer_.evaluate(sample.temperatureC);
    sample.state = transition.currentState;
    sample.transitionOccurred = transition.stateChanged;
    sample.message = transition.description;

    if (transition.stateChanged) {
        // Trigger alert and log state transition
        alertManager_.processTransition(transition);
        logger_.logTransition(transition, "STATE_TRANSITION");
    } else {
        // Periodic heartbeat telemetry log
        LogRecord rec;
        rec.timestamp = sample.timestamp;
        rec.temperatureC = sample.temperatureC;
        rec.previousState = transition.previousState;
        rec.currentState = transition.currentState;
        rec.eventType = "TELEMETRY";
        rec.message = "Periodic sensor reading";
        logger_.logEvent(rec);
    }

    return sample;
}

void TemperatureMonitor::run(std::chrono::milliseconds interval,
                             std::function<bool()> shouldContinuePredicate,
                             TelemetryCallback onSample)
{
    isRunning_ = true;

    while (isRunning_ && (!shouldContinuePredicate || shouldContinuePredicate())) {
        auto sample = pollOnce();
        if (onSample) {
            onSample(sample);
        }

        // Sleep with early interrupt checking
        auto deadline = std::chrono::steady_clock::now() + interval;
        while (isRunning_ && std::chrono::steady_clock::now() < deadline) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            if (shouldContinuePredicate && !shouldContinuePredicate()) {
                break;
            }
        }
    }

    isRunning_ = false;
}

} // namespace ltempguard

/*
 * LTempGuard - Linux-Based IoT Temperature Monitoring and Alert System
 * File: Application.cpp
 * Description: Implementation of top-level application coordinator.
 * Author: LTempGuard Engineering Team
 */

#include "application/Application.hpp"
#include <iostream>

namespace ltempguard {

Application::Application(std::string configPath)
    : configPath_(std::move(configPath))
    , config_(configPath_)
{
}

Application::~Application()
{
    shutdown();
}

bool Application::initialize()
{
    // 1. Load configuration
    config_.load();
    const auto& appCfg = config_.get();

    // 2. Initialize logger
    logger_ = std::make_unique<Logger>(appCfg.textLogPath, appCfg.csvLogPath);
    if (!logger_->initialize()) {
        std::cerr << "APPLICATION WARNING: Logging initialization failed. Operating in degraded mode.\n";
    }
    logger_->log(LogLevel::INFO, "=== LTempGuard System Starting ===");

    // 3. Initialize device abstraction
    device_ = std::make_unique<TemperatureDevice>(appCfg.devicePath);
    if (!device_->openDevice()) {
        std::cerr << "APPLICATION NOTICE: Could not open " << appCfg.devicePath
                  << " immediately. (" << static_cast<TemperatureDevice*>(device_.get())->getLastError()
                  << "). Will retry upon first access.\n";
        logger_->log(LogLevel::WARNING, "Device node " + appCfg.devicePath + " not opened at startup.");
    } else {
        logger_->log(LogLevel::INFO, "Device node " + appCfg.devicePath + " opened successfully.");
        // Sync hardware thresholds with config
        device_->setThresholds(appCfg.warningThresholdC, appCfg.criticalThresholdC);
    }

    // 4. Initialize analyzer with config thresholds
    analyzer_ = std::make_unique<TemperatureAnalyzer>(appCfg.toThresholdConfig());

    // 5. Initialize alert manager
    alertManager_ = std::make_unique<AlertManager>();

    // 6. Initialize monitor engine
    monitor_ = std::make_unique<TemperatureMonitor>(*device_, *analyzer_, *alertManager_, *logger_);

    // 7. Initialize CLI console
    cli_ = std::make_unique<CLI>(*device_, *analyzer_, *monitor_, *alertManager_, config_, *logger_);

    isInitialized_ = true;
    return true;
}

int Application::run()
{
    if (!isInitialized_ && !initialize()) {
        std::cerr << "FATAL: Application initialization failed.\n";
        return 1;
    }

    cli_->runMenuLoop();
    return 0;
}

void Application::shutdown()
{
    if (monitor_ && monitor_->isRunning()) {
        monitor_->requestStop();
    }
    if (logger_) {
        logger_->log(LogLevel::INFO, "=== LTempGuard System Shutdown Cleanly ===");
        logger_->close();
    }
    if (device_) {
        device_->closeDevice();
    }
    isInitialized_ = false;
}

} // namespace ltempguard

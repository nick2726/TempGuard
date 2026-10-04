/*
 * LTempGuard - Linux-Based IoT Temperature Monitoring and Alert System
 * File: CLI.hpp
 * Description: Interactive console dashboard and user interface.
 * Author: LTempGuard Engineering Team
 */

#ifndef LTEMPGUARD_CLI_HPP
#define LTEMPGUARD_CLI_HPP

#include "device/TemperatureDevice.hpp"
#include "analysis/TemperatureAnalyzer.hpp"
#include "monitoring/TemperatureMonitor.hpp"
#include "alert/AlertManager.hpp"
#include "configuration/Configuration.hpp"
#include "logging/Logger.hpp"

namespace ltempguard {

/**
 * CLI - Provides formatted menus, telemetry dashboards, and command dispatching.
 */
class CLI {
public:
    CLI(ITemperatureDevice& device,
        TemperatureAnalyzer& analyzer,
        TemperatureMonitor& monitor,
        AlertManager& alertManager,
        Configuration& config,
        Logger& logger);

    void runMenuLoop();

    void displayDashboard();
    void handleReadTemperature();
    void handleSetTemperature();
    void handleConfigureThresholds();
    void handleStartMonitoring();
    void handleViewDeviceStatus();
    void handleViewLogs();
    void handleResetStats();

private:
    ITemperatureDevice& device_;
    TemperatureAnalyzer& analyzer_;
    TemperatureMonitor& monitor_;
    AlertManager& alertManager_;
    Configuration& config_;
    Logger& logger_;

    static void clearScreen();
    static void pauseForUser();
    static double promptDouble(const std::string& promptText, double minVal, double maxVal);
};

} // namespace ltempguard

#endif // LTEMPGUARD_CLI_HPP

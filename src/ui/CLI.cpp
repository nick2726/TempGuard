/*
 * LTempGuard - Linux-Based IoT Temperature Monitoring and Alert System
 * File: CLI.cpp
 * Description: Implementation of interactive console dashboard and menu flows.
 * Author: LTempGuard Engineering Team
 */

#include "ui/CLI.hpp"

#include <iostream>
#include <iomanip>
#include <fstream>
#include <limits>
#include <csignal>
#include <atomic>

namespace ltempguard {

static std::atomic<bool> g_stopRequested{false};

static void sigintHandler(int)
{
    g_stopRequested = true;
}

CLI::CLI(ITemperatureDevice& device,
         TemperatureAnalyzer& analyzer,
         TemperatureMonitor& monitor,
         AlertManager& alertManager,
         Configuration& config,
         Logger& logger)
    : device_(device)
    , analyzer_(analyzer)
    , monitor_(monitor)
    , alertManager_(alertManager)
    , config_(config)
    , logger_(logger)
{
}

void CLI::clearScreen()
{
    std::cout << "\033[2J\033[1;1H";
}

void CLI::pauseForUser()
{
    std::cout << "\nPress Enter to continue...";
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cin.get();
}

double CLI::promptDouble(const std::string& promptText, double minVal, double maxVal)
{
    while (true) {
        std::cout << promptText;
        double val;
        if (std::cin >> val) {
            if (val >= minVal && val <= maxVal) {
                return val;
            }
            std::cout << "ERROR: Value must be between " << minVal << " and " << maxVal << ".\n";
        } else {
            std::cout << "ERROR: Invalid numeric input.\n";
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        }
    }
}

void CLI::displayDashboard()
{
    auto maybeStatus = device_.getStatus();
    auto thresholds = analyzer_.getThresholds();

    double currentTemp = maybeStatus ? maybeStatus->currentTemperatureC : 0.0;
    bool isOnline = maybeStatus ? maybeStatus->isOnline : false;
    MonitoringState state = isOnline ? analyzer_.classify(currentTemp) : MonitoringState::FAULT;

    std::string stateColor = "\033[0m";
    switch (state) {
    case MonitoringState::NORMAL:   stateColor = "\033[1;32m"; break; // Green
    case MonitoringState::WARNING:  stateColor = "\033[1;33m"; break; // Yellow
    case MonitoringState::CRITICAL: stateColor = "\033[1;31m"; break; // Red
    case MonitoringState::FAULT:    stateColor = "\033[1;35m"; break; // Magenta
    }

    std::cout << "========================================\n"
              << "          LTempGuard Monitor\n"
              << "========================================\n\n"
              << "Device       : " << device_.getDevicePath() << "\n"
              << "Device State : " << (isOnline ? "\033[1;32mONLINE\033[0m" : "\033[1;31mOFFLINE\033[0m") << "\n"
              << "Mode         : SIMULATION\n\n";

    if (isOnline) {
        std::cout << "Temperature  : " << std::fixed << std::setprecision(1) << currentTemp << " °C\n"
                  << "Status       : " << stateColor << toString(state) << "\033[0m\n\n";
    } else {
        std::cout << "Temperature  : N/A\n"
                  << "Status       : \033[1;31mOFFLINE (Driver not loaded?)\033[0m\n\n";
    }

    std::cout << "Warning      : " << std::fixed << std::setprecision(1) << thresholds.warningThresholdC << " °C\n"
              << "Critical     : " << std::fixed << std::setprecision(1) << thresholds.criticalThresholdC << " °C\n"
              << "----------------------------------------\n"
              << "1. Read Temperature\n"
              << "2. Set Temperature (Simulation)\n"
              << "3. Configure Thresholds\n"
              << "4. Start Continuous Monitoring\n"
              << "5. View Device Status & Telemetry\n"
              << "6. View Recent Logs\n"
              << "7. Reset Telemetry Statistics\n"
              << "8. Exit\n"
              << "----------------------------------------\n"
              << "Select option [1-8]: ";
}

void CLI::handleReadTemperature()
{
    clearScreen();
    std::cout << "--- [1] One-Shot Temperature Reading ---\n";
    auto temp = device_.readTemperature();
    if (temp.has_value()) {
        auto state = analyzer_.classify(temp.value());
        std::cout << "Current Temperature : " << std::fixed << std::setprecision(2)
                  << temp.value() << " °C\n"
                  << "Assigned State      : " << toString(state) << "\n";
    } else {
        std::cout << "ERROR: Failed to read from device. Is /dev/temp_sensor loaded?\n"
                  << "Details: " << device_.getDevicePath() << "\n";
    }
    pauseForUser();
}

void CLI::handleSetTemperature()
{
    clearScreen();
    std::cout << "--- [2] Set Simulated Temperature ---\n";
    std::cout << "Allowed range: -50.0 °C to +150.0 °C\n";
    double newTemp = promptDouble("Enter new temperature (°C): ", -50.0, 150.0);

    if (device_.setTemperature(newTemp)) {
        std::cout << "SUCCESS: Simulated temperature updated to "
                  << std::fixed << std::setprecision(2) << newTemp << " °C.\n";

        // Evaluate immediately
        auto transition = analyzer_.evaluate(newTemp);
        if (transition.stateChanged) {
            alertManager_.processTransition(transition);
            logger_.logTransition(transition, "SIMULATION_UPDATE");
        }
    } else {
        std::cout << "ERROR: Failed to update simulated temperature.\n";
    }
    pauseForUser();
}

void CLI::handleConfigureThresholds()
{
    clearScreen();
    std::cout << "--- [3] Configure Thresholds ---\n";
    auto current = analyzer_.getThresholds();
    std::cout << "Current Warning  : " << current.warningThresholdC << " °C\n"
              << "Current Critical : " << current.criticalThresholdC << " °C\n\n";

    double warn = promptDouble("Enter new Warning threshold (°C): ", -40.0, 140.0);
    double crit = promptDouble("Enter new Critical threshold (°C): ", warn + 0.1, 150.0);

    ThresholdConfig newConfig{warn, crit};
    if (analyzer_.setThresholds(newConfig)) {
        device_.setThresholds(warn, crit);

        auto appCfg = config_.get();
        appCfg.warningThresholdC = warn;
        appCfg.criticalThresholdC = crit;
        config_.set(appCfg);
        config_.save();

        std::cout << "SUCCESS: Thresholds updated and saved to configuration.\n";
        logger_.log(LogLevel::INFO, "Thresholds reconfigured: Warning=" +
                                    std::to_string(warn) + " C, Critical=" +
                                    std::to_string(crit) + " C");
    } else {
        std::cout << "ERROR: Invalid thresholds. Warning must be strictly less than Critical.\n";
    }
    pauseForUser();
}

void CLI::handleStartMonitoring()
{
    clearScreen();
    std::cout << "============================================================\n"
              << "          LTempGuard Continuous Monitoring Mode\n"
              << "============================================================\n"
              << "Press Ctrl+C to stop monitoring and return to main menu.\n\n";

    g_stopRequested = false;
    struct sigaction sa {};
    sa.sa_handler = sigintHandler;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, nullptr);

    // Register a subscriber to print alert banners live
    alertManager_.subscribe([](const AlertEvent& event) {
        std::cout << "\n" << event.formattedBanner;
    });

    monitor_.run(
        config_.get().pollInterval,
        []() { return !g_stopRequested; },
        [](const TelemetrySample& sample) {
            std::cout << "[" << Logger::formatIso8601(sample.timestamp) << "] "
                      << "Temp: " << std::fixed << std::setprecision(1) << sample.temperatureC << " °C | "
                      << "State: " << toString(sample.state)
                      << (sample.deviceOnline ? " [ONLINE]" : " [OFFLINE]")
                      << "\r" << std::flush;
        }
    );

    // Restore standard SIG_DFL handler
    signal(SIGINT, SIG_DFL);
    std::cout << "\n\nMonitoring stopped.\n";
    pauseForUser();
}

void CLI::handleViewDeviceStatus()
{
    clearScreen();
    std::cout << "--- [5] Device Status & Kernel Telemetry ---\n";
    auto maybeStatus = device_.getStatus();
    if (maybeStatus.has_value()) {
        const auto& s = maybeStatus.value();
        std::cout << "Device Node          : " << s.devicePath << "\n"
                  << "Status               : " << (s.isOnline ? "ONLINE" : "OFFLINE") << "\n"
                  << "Simulation Mode      : " << (s.isSimulationMode ? "ENABLED" : "DISABLED") << "\n"
                  << "Current Temperature  : " << std::fixed << std::setprecision(2) << s.currentTemperatureC << " °C\n"
                  << "Warning Threshold    : " << s.warningThresholdC << " °C\n"
                  << "Critical Threshold   : " << s.criticalThresholdC << " °C\n"
                  << "VFS Read Sycalls     : " << s.readCount << "\n"
                  << "VFS Write Syscalls   : " << s.writeCount << "\n"
                  << "IOCTL Operations     : " << s.ioctlCount << "\n"
                  << "Last Kernel Update   : " << Logger::formatIso8601(s.lastUpdate) << "\n";
    } else {
        std::cout << "ERROR: Failed to retrieve status from device.\n";
    }
    pauseForUser();
}

void CLI::handleViewLogs()
{
    clearScreen();
    std::cout << "--- [6] Recent Log Entries (" << config_.get().textLogPath << ") ---\n\n";

    std::ifstream logFile(config_.get().textLogPath);
    if (logFile.is_open()) {
        std::vector<std::string> lines;
        std::string line;
        while (std::getline(logFile, line)) {
            lines.push_back(line);
        }

        size_t start = lines.size() > 15 ? lines.size() - 15 : 0;
        for (size_t i = start; i < lines.size(); ++i) {
            std::cout << lines[i] << "\n";
        }
        if (lines.empty()) {
            std::cout << "(Log file is empty)\n";
        }
    } else {
        std::cout << "ERROR: Unable to open log file '" << config_.get().textLogPath << "'.\n";
    }
    pauseForUser();
}

void CLI::handleResetStats()
{
    clearScreen();
    std::cout << "--- [7] Reset Telemetry Statistics ---\n";
    if (device_.resetStats()) {
        std::cout << "SUCCESS: Driver telemetry statistics reset to zero.\n";
        logger_.log(LogLevel::INFO, "Kernel telemetry statistics reset by operator.");
    } else {
        std::cout << "ERROR: Failed to reset telemetry statistics.\n";
    }
    pauseForUser();
}

void CLI::runMenuLoop()
{
    while (true) {
        clearScreen();
        displayDashboard();

        int choice = 0;
        if (!(std::cin >> choice)) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            continue;
        }

        switch (choice) {
        case 1: handleReadTemperature(); break;
        case 2: handleSetTemperature(); break;
        case 3: handleConfigureThresholds(); break;
        case 4: handleStartMonitoring(); break;
        case 5: handleViewDeviceStatus(); break;
        case 6: handleViewLogs(); break;
        case 7: handleResetStats(); break;
        case 8:
            std::cout << "\nExiting LTempGuard Monitor. Goodbye!\n";
            return;
        default:
            break;
        }
    }
}

} // namespace ltempguard

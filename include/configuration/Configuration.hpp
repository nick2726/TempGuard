/*
 * LTempGuard - Linux-Based IoT Temperature Monitoring and Alert System
 * File: Configuration.hpp
 * Description: Configuration file parser and parameter validator.
 * Author: LTempGuard Engineering Team
 */

#ifndef LTEMPGUARD_CONFIGURATION_HPP
#define LTEMPGUARD_CONFIGURATION_HPP

#include "analysis/TemperatureAnalyzer.hpp"
#include <string>
#include <chrono>

namespace ltempguard {

struct AppConfig {
    double warningThresholdC{40.0};
    double criticalThresholdC{60.0};
    std::chrono::milliseconds pollInterval{1000};
    std::string devicePath{"/dev/temp_sensor"};
    std::string textLogPath{"logs/ltempguard.log"};
    std::string csvLogPath{"logs/ltempguard_events.csv"};

    [[nodiscard]] bool isValid() const noexcept {
        return warningThresholdC < criticalThresholdC &&
               warningThresholdC >= -50.0 &&
               criticalThresholdC <= 150.0 &&
               pollInterval.count() >= 50 &&
               !devicePath.empty();
    }

    [[nodiscard]] ThresholdConfig toThresholdConfig() const noexcept {
        return ThresholdConfig{warningThresholdC, criticalThresholdC};
    }
};

/**
 * Configuration - Loads and persists system parameters from/to key-value files.
 */
class Configuration {
public:
    explicit Configuration(std::string configPath = "config/default.conf");

    bool load();
    bool save(const std::string& path = "");

    [[nodiscard]] const AppConfig& get() const noexcept;
    void set(const AppConfig& config);

    [[nodiscard]] const std::string& getPath() const noexcept;

private:
    std::string configPath_;
    AppConfig config_;

    static std::string trim(const std::string& str);
};

} // namespace ltempguard

#endif // LTEMPGUARD_CONFIGURATION_HPP

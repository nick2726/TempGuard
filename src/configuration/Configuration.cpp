/*
 * LTempGuard - Linux-Based IoT Temperature Monitoring and Alert System
 * File: Configuration.cpp
 * Description: Implementation of configuration loader and validator.
 * Author: LTempGuard Engineering Team
 */

#include "configuration/Configuration.hpp"

#include <fstream>
#include <sstream>
#include <iostream>

namespace ltempguard {

std::string Configuration::trim(const std::string& str)
{
    const auto strBegin = str.find_first_not_of(" \t\r\n");
    if (strBegin == std::string::npos)
        return "";

    const auto strEnd = str.find_last_not_of(" \t\r\n");
    const auto strRange = strEnd - strBegin + 1;

    return str.substr(strBegin, strRange);
}

Configuration::Configuration(std::string configPath)
    : configPath_(std::move(configPath))
{
}

bool Configuration::load()
{
    std::ifstream file(configPath_);
    if (!file.is_open()) {
        std::cerr << "CONFIG WARNING: Could not open '" << configPath_
                  << "', using built-in defaults.\n";
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') {
            continue;
        }

        auto sep = line.find('=');
        if (sep == std::string::npos) {
            continue;
        }

        std::string key = trim(line.substr(0, sep));
        std::string val = trim(line.substr(sep + 1));

        try {
            if (key == "warning_threshold") {
                config_.warningThresholdC = std::stod(val);
            } else if (key == "critical_threshold") {
                config_.criticalThresholdC = std::stod(val);
            } else if (key == "poll_interval_ms") {
                config_.pollInterval = std::chrono::milliseconds(std::stoll(val));
            } else if (key == "device_path") {
                config_.devicePath = val;
            } else if (key == "log_file") {
                config_.textLogPath = val;
            } else if (key == "csv_log_file") {
                config_.csvLogPath = val;
            }
        } catch (const std::exception& e) {
            std::cerr << "CONFIG WARNING: Malformed value for key '" << key << "': " << e.what() << "\n";
        }
    }

    if (!config_.isValid()) {
        std::cerr << "CONFIG ERROR: Configuration contains invalid values. Resetting to defaults.\n";
        config_ = AppConfig{};
        return false;
    }

    return true;
}

bool Configuration::save(const std::string& path)
{
    std::string target = path.empty() ? configPath_ : path;
    std::ofstream file(target);
    if (!file.is_open()) {
        return false;
    }

    file << "# LTempGuard Configuration File\n"
         << "warning_threshold=" << config_.warningThresholdC << "\n"
         << "critical_threshold=" << config_.criticalThresholdC << "\n"
         << "poll_interval_ms=" << config_.pollInterval.count() << "\n"
         << "device_path=" << config_.devicePath << "\n"
         << "log_file=" << config_.textLogPath << "\n"
         << "csv_log_file=" << config_.csvLogPath << "\n";

    return true;
}

const AppConfig& Configuration::get() const noexcept
{
    return config_;
}

void Configuration::set(const AppConfig& config)
{
    if (config.isValid()) {
        config_ = config;
    }
}

const std::string& Configuration::getPath() const noexcept
{
    return configPath_;
}

} // namespace ltempguard

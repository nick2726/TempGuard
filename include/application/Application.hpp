/*
 * LTempGuard - Linux-Based IoT Temperature Monitoring and Alert System
 * File: Application.hpp
 * Description: Top-level application coordinator managing subsystem lifecycles.
 * Author: LTempGuard Engineering Team
 */

#ifndef LTEMPGUARD_APPLICATION_HPP
#define LTEMPGUARD_APPLICATION_HPP

#include "configuration/Configuration.hpp"
#include "device/TemperatureDevice.hpp"
#include "analysis/TemperatureAnalyzer.hpp"
#include "alert/AlertManager.hpp"
#include "logging/Logger.hpp"
#include "monitoring/TemperatureMonitor.hpp"
#include "ui/CLI.hpp"

#include <memory>
#include <string>

namespace ltempguard {

class Application {
public:
    explicit Application(std::string configPath = "config/default.conf");
    ~Application();

    bool initialize();
    int run();
    void shutdown();

private:
    std::string configPath_;
    Configuration config_;
    std::unique_ptr<Logger> logger_;
    std::unique_ptr<ITemperatureDevice> device_;
    std::unique_ptr<TemperatureAnalyzer> analyzer_;
    std::unique_ptr<AlertManager> alertManager_;
    std::unique_ptr<TemperatureMonitor> monitor_;
    std::unique_ptr<CLI> cli_;
    bool isInitialized_{false};
};

} // namespace ltempguard

#endif // LTEMPGUARD_APPLICATION_HPP

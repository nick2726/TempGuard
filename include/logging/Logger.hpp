/*
 * LTempGuard - Linux-Based IoT Temperature Monitoring and Alert System
 * File: Logger.hpp
 * Description: High-reliability structured textual and CSV logging subsystem.
 * Author: LTempGuard Engineering Team
 */

#ifndef LTEMPGUARD_LOGGER_HPP
#define LTEMPGUARD_LOGGER_HPP

#include "analysis/TemperatureAnalyzer.hpp"
#include <string>
#include <fstream>
#include <mutex>
#include <chrono>

namespace ltempguard {

enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    CRITICAL
};

[[nodiscard]] std::string_view toString(LogLevel level) noexcept;

struct LogRecord {
    std::chrono::system_clock::time_point timestamp{};
    double temperatureC{0.0};
    MonitoringState previousState{MonitoringState::NORMAL};
    MonitoringState currentState{MonitoringState::NORMAL};
    std::string eventType{"TELEMETRY"};
    std::string message{};
};

/**
 * Logger - Thread-safe logging subsystem writing synchronized entries to text and CSV files.
 */
class Logger {
public:
    explicit Logger(std::string textLogPath = "logs/ltempguard.log",
                    std::string csvLogPath = "logs/ltempguard_events.csv");
    ~Logger();

    // Non-copyable, non-movable due to streams & mutex
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    bool initialize();
    void close();

    void log(LogLevel level, const std::string& message);
    void logEvent(const LogRecord& record);
    void logTransition(const StateTransitionResult& transition, const std::string& eventType = "STATE_CHANGE");

    [[nodiscard]] static std::string formatIso8601(std::chrono::system_clock::time_point tp);

private:
    std::string textLogPath_;
    std::string csvLogPath_;
    std::ofstream textStream_;
    std::ofstream csvStream_;
    std::mutex logMutex_;
    bool isInitialized_{false};

    void writeCsvHeader();
};

} // namespace ltempguard

#endif // LTEMPGUARD_LOGGER_HPP

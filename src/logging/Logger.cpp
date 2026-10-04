/*
 * LTempGuard - Linux-Based IoT Temperature Monitoring and Alert System
 * File: Logger.cpp
 * Description: Implementation of synchronized dual-sink text and CSV logging.
 * Author: LTempGuard Engineering Team
 */

#include "logging/Logger.hpp"

#include <iostream>
#include <iomanip>
#include <sstream>
#include <filesystem>
#include <ctime>

namespace ltempguard {

std::string_view toString(LogLevel level) noexcept
{
    switch (level) {
    case LogLevel::DEBUG:    return "DEBUG";
    case LogLevel::INFO:     return "INFO";
    case LogLevel::WARNING:  return "WARNING";
    case LogLevel::ERROR:    return "ERROR";
    case LogLevel::CRITICAL: return "CRITICAL";
    }
    return "UNKNOWN";
}

std::string Logger::formatIso8601(std::chrono::system_clock::time_point tp)
{
    auto epochDuration = tp.time_since_epoch();
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(epochDuration);
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(epochDuration) % 1000;

    std::time_t tt = seconds.count();
    std::tm tmVal {};
    gmtime_r(&tt, &tmVal);

    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tmVal);

    std::ostringstream oss;
    oss << buf << "." << std::setfill('0') << std::setw(3) << millis.count();
    return oss.str();
}

Logger::Logger(std::string textLogPath, std::string csvLogPath)
    : textLogPath_(std::move(textLogPath))
    , csvLogPath_(std::move(csvLogPath))
{
}

Logger::~Logger()
{
    close();
}

bool Logger::initialize()
{
    std::lock_guard<std::mutex> lock(logMutex_);

    try {
        namespace fs = std::filesystem;
        fs::path textPath(textLogPath_);
        if (textPath.has_parent_path()) {
            fs::create_directories(textPath.parent_path());
        }

        fs::path csvPath(csvLogPath_);
        if (csvPath.has_parent_path()) {
            fs::create_directories(csvPath.parent_path());
        }

        bool csvNeedsHeader = !fs::exists(csvPath) || fs::file_size(csvPath) == 0;

        textStream_.open(textLogPath_, std::ios::out | std::ios::app);
        csvStream_.open(csvLogPath_, std::ios::out | std::ios::app);

        if (!textStream_.is_open() || !csvStream_.is_open()) {
            std::cerr << "LOGGER ERROR: Failed to open log file streams.\n";
            return false;
        }

        if (csvNeedsHeader) {
            writeCsvHeader();
        }

        isInitialized_ = true;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "LOGGER EXCEPTION: " << e.what() << "\n";
        return false;
    }
}

void Logger::writeCsvHeader()
{
    csvStream_ << "Timestamp,Temperature_C,Previous_State,Current_State,Event_Type,Message\n";
    csvStream_.flush();
}

void Logger::close()
{
    std::lock_guard<std::mutex> lock(logMutex_);
    if (textStream_.is_open()) {
        textStream_.flush();
        textStream_.close();
    }
    if (csvStream_.is_open()) {
        csvStream_.flush();
        csvStream_.close();
    }
    isInitialized_ = false;
}

void Logger::log(LogLevel level, const std::string& message)
{
    std::lock_guard<std::mutex> lock(logMutex_);
    std::string timestamp = formatIso8601(std::chrono::system_clock::now());

    std::string line = timestamp + " [" + std::string(toString(level)) + "] " + message + "\n";

    if (textStream_.is_open()) {
        textStream_ << line;
        textStream_.flush();
    } else {
        std::cout << line;
    }
}

void Logger::logEvent(const LogRecord& record)
{
    std::lock_guard<std::mutex> lock(logMutex_);
    std::string timestamp = formatIso8601(record.timestamp);

    // Text format: 2026-10-04 17:30:20.123 | TEMP=38.2 | STATE=NORMAL | EVENT=TELEMETRY | Msg
    std::ostringstream textOss;
    textOss << timestamp << " | TEMP="
            << std::fixed << std::setprecision(1) << record.temperatureC
            << " | STATE=" << toString(record.currentState)
            << " | EVENT=" << record.eventType
            << " | " << record.message << "\n";

    if (textStream_.is_open()) {
        textStream_ << textOss.str();
        textStream_.flush();
    }

    // CSV format: Timestamp,Temperature_C,Previous_State,Current_State,Event_Type,Message
    std::ostringstream csvOss;
    csvOss << timestamp << ","
           << std::fixed << std::setprecision(2) << record.temperatureC << ","
           << toString(record.previousState) << ","
           << toString(record.currentState) << ","
           << "\"" << record.eventType << "\","
           << "\"" << record.message << "\"\n";

    if (csvStream_.is_open()) {
        csvStream_ << csvOss.str();
        csvStream_.flush();
    }
}

void Logger::logTransition(const StateTransitionResult& transition, const std::string& eventType)
{
    LogRecord record;
    record.timestamp = transition.timestamp;
    record.temperatureC = transition.temperature;
    record.previousState = transition.previousState;
    record.currentState = transition.currentState;
    record.eventType = eventType;
    record.message = transition.description;
    logEvent(record);
}

} // namespace ltempguard

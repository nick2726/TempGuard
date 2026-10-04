/*
 * LTempGuard - Linux-Based IoT Temperature Monitoring and Alert System
 * File: test_logger.cpp
 * Description: Unit tests for synchronized text and CSV logging.
 * Author: LTempGuard Engineering Team
 */

#include "TestHarness.hpp"
#include "logging/Logger.hpp"
#include <filesystem>
#include <fstream>

using namespace ltempguard;
using namespace ltempguard::testing;

REGISTER_TEST(LoggerTests, CreateAndAppendLogs)
{
    std::string testText = "build/test_log.log";
    std::string testCsv = "build/test_events.csv";

    std::filesystem::remove(testText);
    std::filesystem::remove(testCsv);

    Logger logger(testText, testCsv);
    ASSERT_TRUE(logger.initialize());

    LogRecord rec;
    rec.timestamp = std::chrono::system_clock::now();
    rec.temperatureC = 42.5;
    rec.previousState = MonitoringState::NORMAL;
    rec.currentState = MonitoringState::WARNING;
    rec.eventType = "TEST_EVENT";
    rec.message = "Threshold crossed";

    logger.logEvent(rec);
    logger.close();

    ASSERT_TRUE(std::filesystem::exists(testText));
    ASSERT_TRUE(std::filesystem::exists(testCsv));

    // Verify CSV contains header and event
    std::ifstream csv(testCsv);
    std::string line1, line2;
    std::getline(csv, line1);
    std::getline(csv, line2);

    ASSERT_FALSE(line1.empty());
    ASSERT_TRUE(line1.find("Temperature_C") != std::string::npos);
    ASSERT_TRUE(line2.find("42.50") != std::string::npos);
    ASSERT_TRUE(line2.find("WARNING") != std::string::npos);

    std::filesystem::remove(testText);
    std::filesystem::remove(testCsv);
}

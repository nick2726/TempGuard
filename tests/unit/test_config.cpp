/*
 * LTempGuard - Linux-Based IoT Temperature Monitoring and Alert System
 * File: test_config.cpp
 * Description: Unit tests for configuration parser, serialization, and validation.
 * Author: LTempGuard Engineering Team
 */

#include "TestHarness.hpp"
#include "configuration/Configuration.hpp"
#include <filesystem>
#include <fstream>

using namespace ltempguard;
using namespace ltempguard::testing;

REGISTER_TEST(ConfigTests, LoadDefaultConfiguration)
{
    Configuration config("config/default.conf");
    bool loaded = config.load();
    ASSERT_TRUE(loaded);

    const auto& c = config.get();
    ASSERT_EQ(c.warningThresholdC, 40.0);
    ASSERT_EQ(c.criticalThresholdC, 60.0);
    ASSERT_EQ(c.pollInterval.count(), 1000);
    ASSERT_EQ(c.devicePath, "/dev/temp_sensor");
    ASSERT_TRUE(c.isValid());
}

REGISTER_TEST(ConfigTests, RejectMalformedConfiguration)
{
    std::string testPath = "build/test_bad_config.conf";
    std::filesystem::create_directories("build");

    std::ofstream out(testPath);
    out << "warning_threshold=80.0\n"
        << "critical_threshold=40.0\n"; // Inverted thresholds!
    out.close();

    Configuration config(testPath);
    bool loaded = config.load();
    ASSERT_FALSE(loaded); // Should reject invalid thresholds
    ASSERT_TRUE(config.get().isValid()); // Should have reset to safe defaults

    std::filesystem::remove(testPath);
}

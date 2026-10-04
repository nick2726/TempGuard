/*
 * LTempGuard - Linux-Based IoT Temperature Monitoring and Alert System
 * File: test_driver_integration.cpp
 * Description: Hardware-in-the-loop and VFS integration tests against /dev/temp_sensor.
 * Author: LTempGuard Engineering Team
 */

#include "TestHarness.hpp"
#include "device/TemperatureDevice.hpp"
#include <filesystem>
#include <cmath>

using namespace ltempguard;
using namespace ltempguard::testing;

REGISTER_TEST(IntegrationTests, Test8_GracefulHandlingWhenDriverAbsent)
{
    // Scenario 8: Driver unloaded or invalid device path
    TemperatureDevice missingDevice("/dev/non_existent_temp_sensor_node");

    ASSERT_FALSE(missingDevice.openDevice());
    ASSERT_FALSE(missingDevice.isOpen());

    auto temp = missingDevice.readTemperature();
    ASSERT_FALSE(temp.has_value());

    bool setRes = missingDevice.setTemperature(45.0);
    ASSERT_FALSE(setRes);

    auto status = missingDevice.getStatus();
    ASSERT_FALSE(status.has_value());

    // Verify detailed error diagnostic is present
    ASSERT_FALSE(missingDevice.getLastError().empty());
}

REGISTER_TEST(IntegrationTests, LiveKernelDeviceInteraction)
{
    const std::string devPath = "/dev/temp_sensor";

    if (!std::filesystem::exists(devPath)) {
        std::cout << "  [NOTICE] /dev/temp_sensor not present. Skipping live kernel module tests.\n"
                  << "           (Run 'sudo make load' to activate live device node integration).\n";
        return;
    }

    TemperatureDevice device(devPath);
    ASSERT_TRUE(device.openDevice());
    ASSERT_TRUE(device.isOpen());

    // 1. Test simulation write and readback
    const double targetTemp = 52.5;
    ASSERT_TRUE(device.setTemperature(targetTemp));

    auto readTemp = device.readTemperature();
    ASSERT_TRUE(readTemp.has_value());
    ASSERT_TRUE(std::abs(readTemp.value() - targetTemp) < 0.1);

    // 2. Test IOCTL threshold configuration
    ASSERT_TRUE(device.setThresholds(42.0, 68.0));
    auto thresholds = device.getThresholds();
    ASSERT_TRUE(thresholds.has_value());
    ASSERT_TRUE(std::abs(thresholds->first - 42.0) < 0.1);
    ASSERT_TRUE(std::abs(thresholds->second - 68.0) < 0.1);

    // 3. Test telemetry status query
    auto status = device.getStatus();
    ASSERT_TRUE(status.has_value());
    ASSERT_TRUE(status->isOnline);
    ASSERT_TRUE(status->readCount > 0 || status->writeCount > 0);

    // 4. Test telemetry statistics reset
    ASSERT_TRUE(device.resetStats());
}

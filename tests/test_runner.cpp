/*
 * LTempGuard - Linux-Based IoT Temperature Monitoring and Alert System
 * File: test_runner.cpp
 * Description: Main entry point for automated test suite.
 * Author: LTempGuard Engineering Team
 */

#include "TestHarness.hpp"

int main()
{
    return ::ltempguard::testing::TestRegistry::instance().runAll();
}

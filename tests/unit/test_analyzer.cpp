/*
 * LTempGuard - Linux-Based IoT Temperature Monitoring and Alert System
 * File: test_analyzer.cpp
 * Description: Unit tests for state machine, classification, and threshold validation.
 * Author: LTempGuard Engineering Team
 */

#include "TestHarness.hpp"
#include "analysis/TemperatureAnalyzer.hpp"

using namespace ltempguard;
using namespace ltempguard::testing;

REGISTER_TEST(AnalyzerTests, Test1_30DegC_ExpectedNormal)
{
    TemperatureAnalyzer analyzer(ThresholdConfig{40.0, 60.0});
    ASSERT_EQ(analyzer.classify(30.0), MonitoringState::NORMAL);
}

REGISTER_TEST(AnalyzerTests, Test2_39DegC_ExpectedNormal)
{
    TemperatureAnalyzer analyzer(ThresholdConfig{40.0, 60.0});
    ASSERT_EQ(analyzer.classify(39.0), MonitoringState::NORMAL);
}

REGISTER_TEST(AnalyzerTests, Test3_40DegC_ExpectedWarning)
{
    TemperatureAnalyzer analyzer(ThresholdConfig{40.0, 60.0});
    ASSERT_EQ(analyzer.classify(40.0), MonitoringState::WARNING);
}

REGISTER_TEST(AnalyzerTests, Test4_50DegC_ExpectedWarning)
{
    TemperatureAnalyzer analyzer(ThresholdConfig{40.0, 60.0});
    ASSERT_EQ(analyzer.classify(50.0), MonitoringState::WARNING);
}

REGISTER_TEST(AnalyzerTests, Test5_60DegC_ExpectedCritical)
{
    TemperatureAnalyzer analyzer(ThresholdConfig{40.0, 60.0});
    ASSERT_EQ(analyzer.classify(60.0), MonitoringState::CRITICAL);
}

REGISTER_TEST(AnalyzerTests, Test6_75DegC_ExpectedCritical)
{
    TemperatureAnalyzer analyzer(ThresholdConfig{40.0, 60.0});
    ASSERT_EQ(analyzer.classify(75.0), MonitoringState::CRITICAL);
}

REGISTER_TEST(AnalyzerTests, Test7_CoolDown_CriticalToNormal)
{
    TemperatureAnalyzer analyzer(ThresholdConfig{40.0, 60.0});

    // Step 1: Prime state to CRITICAL with 65.0 C
    auto res1 = analyzer.evaluate(65.0);
    ASSERT_EQ(res1.currentState, MonitoringState::CRITICAL);
    ASSERT_TRUE(res1.stateChanged);

    // Step 2: Cool down to 35.0 C (below warning threshold 40.0 C)
    auto res2 = analyzer.evaluate(35.0);
    ASSERT_EQ(res2.previousState, MonitoringState::CRITICAL);
    ASSERT_EQ(res2.currentState, MonitoringState::NORMAL);
    ASSERT_TRUE(res2.stateChanged);
}

REGISTER_TEST(AnalyzerTests, Test7b_StepwiseCoolDown)
{
    TemperatureAnalyzer analyzer(ThresholdConfig{40.0, 60.0});

    analyzer.evaluate(70.0); // CRITICAL
    ASSERT_EQ(analyzer.getCurrentState(), MonitoringState::CRITICAL);

    auto resStep1 = analyzer.evaluate(45.0); // Step down to WARNING
    ASSERT_EQ(resStep1.currentState, MonitoringState::WARNING);
    ASSERT_TRUE(resStep1.stateChanged);

    auto resStep2 = analyzer.evaluate(25.0); // Step down to NORMAL
    ASSERT_EQ(resStep2.currentState, MonitoringState::NORMAL);
    ASSERT_TRUE(resStep2.stateChanged);
}

REGISTER_TEST(AnalyzerTests, Test9_InvalidThresholdConfiguration)
{
    TemperatureAnalyzer analyzer;

    // Case 1: Warning threshold >= Critical threshold
    ThresholdConfig invertedConfig{60.0, 40.0};
    ASSERT_FALSE(invertedConfig.isValid());
    ASSERT_FALSE(analyzer.setThresholds(invertedConfig));

    // Case 2: Equal thresholds
    ThresholdConfig equalConfig{50.0, 50.0};
    ASSERT_FALSE(equalConfig.isValid());
    ASSERT_FALSE(analyzer.setThresholds(equalConfig));

    // Case 3: Out-of-range thresholds (> 150 C)
    ThresholdConfig oobConfig{40.0, 200.0};
    ASSERT_FALSE(oobConfig.isValid());
    ASSERT_FALSE(analyzer.setThresholds(oobConfig));
}

REGISTER_TEST(AnalyzerTests, Test10_InvalidTemperatureSanity)
{
    TemperatureAnalyzer analyzer(ThresholdConfig{40.0, 60.0});

    // Sub-zero extreme below physical minimum (-50 C)
    ASSERT_EQ(analyzer.classify(-100.0), MonitoringState::FAULT);

    // Heat extreme above physical maximum (150 C)
    ASSERT_EQ(analyzer.classify(300.0), MonitoringState::FAULT);
}

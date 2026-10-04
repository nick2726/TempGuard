/*
 * LTempGuard - Linux-Based IoT Temperature Monitoring and Alert System
 * File: TestHarness.hpp
 * Description: Lightweight, zero-dependency C++ test harness.
 * Author: LTempGuard Engineering Team
 */

#ifndef LTEMPGUARD_TEST_HARNESS_HPP
#define LTEMPGUARD_TEST_HARNESS_HPP

#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <sstream>

namespace ltempguard::testing {

struct TestCase {
    std::string suite;
    std::string name;
    std::function<void()> func;
};

class TestRegistry {
public:
    static TestRegistry& instance() {
        static TestRegistry reg;
        return reg;
    }

    void addTest(std::string suite, std::string name, std::function<void()> func) {
        tests_.push_back({std::move(suite), std::move(name), std::move(func)});
    }

    int runAll() {
        int passed = 0;
        int failed = 0;

        std::cout << "============================================================\n"
                  << "        LTempGuard Automated Verification Test Suite        \n"
                  << "============================================================\n";

        for (const auto& test : tests_) {
            std::cout << "[RUN       ] " << test.suite << "." << test.name << "\n";
            try {
                test.func();
                std::cout << "\033[1;32m[       OK ]\033[0m " << test.suite << "." << test.name << "\n";
                passed++;
            } catch (const std::exception& e) {
                std::cout << "\033[1;31m[  FAILED  ]\033[0m " << test.suite << "." << test.name
                          << " (Exception: " << e.what() << ")\n";
                failed++;
            } catch (...) {
                std::cout << "\033[1;31m[  FAILED  ]\033[0m " << test.suite << "." << test.name
                          << " (Unknown non-standard exception)\n";
                failed++;
            }
        }

        std::cout << "============================================================\n"
                  << "Test Results Summary:\n"
                  << "  Total Tests  : " << (passed + failed) << "\n"
                  << "  Passed       : \033[1;32m" << passed << "\033[0m\n"
                  << "  Failed       : " << (failed > 0 ? "\033[1;31m" : "\033[1;32m")
                  << failed << "\033[0m\n"
                  << "============================================================\n";

        return (failed == 0) ? 0 : 1;
    }

private:
    std::vector<TestCase> tests_;
};

#define ASSERT_TRUE(cond) \
    do { \
        if (!(cond)) { \
            std::ostringstream _oss; \
            _oss << "Assertion failed: (" #cond ") at " << __FILE__ << ":" << __LINE__; \
            throw std::runtime_error(_oss.str()); \
        } \
    } while (0)

#define ASSERT_FALSE(cond) ASSERT_TRUE(!(cond))

#define ASSERT_EQ(val1, val2) \
    do { \
        if (!((val1) == (val2))) { \
            std::ostringstream _oss; \
            _oss << "Equality assertion failed: (" #val1 " == " #val2 ") at " << __FILE__ << ":" << __LINE__; \
            throw std::runtime_error(_oss.str()); \
        } \
    } while (0)

#define REGISTER_TEST(suite, name) \
    void suite##_##name(); \
    struct Register_##suite##_##name { \
        Register_##suite##_##name() { \
            ::ltempguard::testing::TestRegistry::instance().addTest(#suite, #name, suite##_##name); \
        } \
    } g_reg_##suite##_##name; \
    void suite##_##name()

} // namespace ltempguard::testing

#endif // LTEMPGUARD_TEST_HARNESS_HPP

# LTempGuard Test Suite CMake Configuration

set(TEST_SOURCES
    test_runner.cpp
    unit/test_analyzer.cpp
    unit/test_config.cpp
    unit/test_logger.cpp
    integration/test_driver_integration.cpp
)

add_executable(test_runner ${TEST_SOURCES})
target_link_libraries(test_runner PRIVATE ltempguard_core)
target_include_directories(test_runner PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../driver
)

add_test(NAME ComprehensiveTestSuite COMMAND test_runner)

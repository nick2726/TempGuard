cmake_minimum_required(VERSION 3.20)
project(LTempGuard VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Strong warning flags for modern C++ development
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
        -Wshadow
        -Wconversion
        -Wnon-virtual-dtor
        -Wold-style-cast
        -Wcast-align
        -Wunused
        -Woverloaded-virtual
    )
endif()

# Core library source files
set(CORE_SOURCES
    src/device/TemperatureDevice.cpp
    src/analysis/TemperatureAnalyzer.cpp
    src/alert/AlertManager.cpp
    src/logging/Logger.cpp
    src/configuration/Configuration.cpp
    src/monitoring/TemperatureMonitor.cpp
    src/ui/CLI.cpp
    src/application/Application.cpp
)

# Core library target
add_library(ltempguard_core STATIC ${CORE_SOURCES})
target_include_directories(ltempguard_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/driver
)

# Main CLI executable
add_executable(ltempguard_app src/main.cpp)
target_link_libraries(ltempguard_app PRIVATE ltempguard_core)

# Install rules
install(TARGETS ltempguard_app DESTINATION bin)

# Automated Test Suite
enable_testing()
add_subdirectory(tests)

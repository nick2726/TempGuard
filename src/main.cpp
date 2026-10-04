/*
 * LTempGuard - Linux-Based IoT Temperature Monitoring and Alert System
 * File: main.cpp
 * Description: Program entry point and CLI option dispatcher.
 * Author: LTempGuard Engineering Team
 */

#include "application/Application.hpp"

#include <iostream>
#include <string>
#include <vector>
#include <csignal>

static ltempguard::Application* g_activeApp{nullptr};

static void globalSignalHandler(int signalNumber)
{
    if (g_activeApp) {
        std::cout << "\n[System Signal " << signalNumber << " Received] Initiating shutdown...\n";
        g_activeApp->shutdown();
    }
}

static void printHelp(const char* progName)
{
    std::cout << "LTempGuard - Linux-Based IoT Temperature Monitoring and Alert System\n"
              << "Usage: " << progName << " [OPTIONS]\n\n"
              << "Options:\n"
              << "  -c, --config <file>   Specify custom configuration file (default: config/default.conf)\n"
              << "  -h, --help            Display this help dialog\n"
              << "  -v, --version         Display version information\n\n"
              << "Evaluator Mode:\n"
              << "  Ensure the kernel driver is loaded: sudo bash scripts/load_driver.sh\n";
}

int main(int argc, char* argv[])
{
    std::string configPath = "config/default.conf";

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            printHelp(argv[0]);
            return 0;
        } else if (arg == "-v" || arg == "--version") {
            std::cout << "LTempGuard v1.0.0 (Linux C/C++ Capstone Edition)\n"
                      << "Architecture: Linux Kernel Driver + Modern C++20 Monitor\n";
            return 0;
        } else if ((arg == "-c" || arg == "--config") && i + 1 < argc) {
            configPath = argv[++i];
        } else {
            std::cerr << "Unknown option: " << arg << "\n";
            printHelp(argv[0]);
            return 1;
        }
    }

    // Register OS termination signals
    std::signal(SIGINT, globalSignalHandler);
    std::signal(SIGTERM, globalSignalHandler);

    ltempguard::Application app(configPath);
    g_activeApp = &app;

    int exitCode = app.run();

    g_activeApp = nullptr;
    return exitCode;
}

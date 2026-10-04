/*
 * LTempGuard - Linux-Based IoT Temperature Monitoring and Alert System
 * File: TemperatureDevice.hpp
 * Description: Modern C++ RAII device abstraction for /dev/temp_sensor.
 * Author: LTempGuard Engineering Team
 */

#ifndef LTEMPGUARD_TEMPERATURE_DEVICE_HPP
#define LTEMPGUARD_TEMPERATURE_DEVICE_HPP

#include <string>
#include <optional>
#include <cstdint>
#include <chrono>
#include "temp_ioctl.h"

namespace ltempguard {

struct DeviceStatus {
    double currentTemperatureC{0.0};
    double warningThresholdC{40.0};
    double criticalThresholdC{60.0};
    uint32_t readCount{0};
    uint32_t writeCount{0};
    uint32_t ioctlCount{0};
    std::chrono::system_clock::time_point lastUpdate{};
    bool isOnline{false};
    bool isSimulationMode{true};
    std::string devicePath{};
};

/**
 * ITemperatureDevice - Pure virtual interface for temperature hardware abstraction.
 * Enables dependency injection and isolated unit testing with mocks.
 */
class ITemperatureDevice {
public:
    virtual ~ITemperatureDevice() = default;

    virtual bool openDevice() = 0;
    virtual void closeDevice() noexcept = 0;
    [[nodiscard]] virtual bool isOpen() const noexcept = 0;

    [[nodiscard]] virtual std::optional<double> readTemperature() = 0;
    virtual bool setTemperature(double tempDegC) = 0;
    virtual bool setThresholds(double warnDegC, double critDegC) = 0;
    [[nodiscard]] virtual std::optional<std::pair<double, double>> getThresholds() = 0;
    [[nodiscard]] virtual std::optional<DeviceStatus> getStatus() = 0;
    virtual bool resetStats() = 0;

    [[nodiscard]] virtual const std::string& getDevicePath() const noexcept = 0;
};

/**
 * TemperatureDevice - Production Linux Character Device implementation.
 * Encapsulates POSIX file descriptor calls (open, read, write, ioctl, close) using RAII.
 */
class TemperatureDevice : public ITemperatureDevice {
public:
    explicit TemperatureDevice(std::string devicePath = "/dev/temp_sensor");
    ~TemperatureDevice() override;

    // Non-copyable, movable
    TemperatureDevice(const TemperatureDevice&) = delete;
    TemperatureDevice& operator=(const TemperatureDevice&) = delete;
    TemperatureDevice(TemperatureDevice&& other) noexcept;
    TemperatureDevice& operator=(TemperatureDevice&& other) noexcept;

    bool openDevice() override;
    void closeDevice() noexcept override;
    [[nodiscard]] bool isOpen() const noexcept override;

    [[nodiscard]] std::optional<double> readTemperature() override;
    bool setTemperature(double tempDegC) override;
    bool setThresholds(double warnDegC, double critDegC) override;
    [[nodiscard]] std::optional<std::pair<double, double>> getThresholds() override;
    [[nodiscard]] std::optional<DeviceStatus> getStatus() override;
    bool resetStats() override;

    [[nodiscard]] const std::string& getDevicePath() const noexcept override;

    [[nodiscard]] const std::string& getLastError() const noexcept;

private:
    std::string devicePath_;
    int fd_{-1};
    mutable std::string lastError_;

    void setLastError(const std::string& err) const;
    static int32_t toMilliCelsius(double degC) noexcept;
    static double toDegreesCelsius(int32_t mC) noexcept;
};

} // namespace ltempguard

#endif // LTEMPGUARD_TEMPERATURE_DEVICE_HPP

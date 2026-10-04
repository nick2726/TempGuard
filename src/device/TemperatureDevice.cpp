/*
 * LTempGuard - Linux-Based IoT Temperature Monitoring and Alert System
 * File: TemperatureDevice.cpp
 * Description: Implementation of RAII Linux Character Device abstraction.
 * Author: LTempGuard Engineering Team
 */

#include "device/TemperatureDevice.hpp"

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <cstring>
#include <cerrno>
#include <sstream>
#include <iomanip>
#include <cmath>

namespace ltempguard {

TemperatureDevice::TemperatureDevice(std::string devicePath)
    : devicePath_(std::move(devicePath))
{
}

TemperatureDevice::~TemperatureDevice()
{
    closeDevice();
}

TemperatureDevice::TemperatureDevice(TemperatureDevice&& other) noexcept
    : devicePath_(std::move(other.devicePath_))
    , fd_(other.fd_)
    , lastError_(std::move(other.lastError_))
{
    other.fd_ = -1;
}

TemperatureDevice& TemperatureDevice::operator=(TemperatureDevice&& other) noexcept
{
    if (this != &other) {
        closeDevice();
        devicePath_ = std::move(other.devicePath_);
        fd_ = other.fd_;
        lastError_ = std::move(other.lastError_);
        other.fd_ = -1;
    }
    return *this;
}

bool TemperatureDevice::openDevice()
{
    if (fd_ >= 0) {
        return true;
    }

    fd_ = ::open(devicePath_.c_str(), O_RDWR);
    if (fd_ < 0) {
        std::ostringstream oss;
        oss << "Failed to open device node '" << devicePath_ << "': " << std::strerror(errno);
        setLastError(oss.str());
        return false;
    }

    lastError_.clear();
    return true;
}

void TemperatureDevice::closeDevice() noexcept
{
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
}

bool TemperatureDevice::isOpen() const noexcept
{
    return fd_ >= 0;
}

const std::string& TemperatureDevice::getDevicePath() const noexcept
{
    return devicePath_;
}

const std::string& TemperatureDevice::getLastError() const noexcept
{
    return lastError_;
}

void TemperatureDevice::setLastError(const std::string& err) const
{
    lastError_ = err;
}

int32_t TemperatureDevice::toMilliCelsius(double degC) noexcept
{
    return static_cast<int32_t>(std::round(degC * 1000.0));
}

double TemperatureDevice::toDegreesCelsius(int32_t mC) noexcept
{
    return static_cast<double>(mC) / 1000.0;
}

std::optional<double> TemperatureDevice::readTemperature()
{
    if (!isOpen() && !openDevice()) {
        return std::nullopt;
    }

    // Try fast atomic IOCTL query first
    int32_t temp_mC = 0;
    if (::ioctl(fd_, TEMP_IOC_GET_TEMP, &temp_mC) == 0) {
        return toDegreesCelsius(temp_mC);
    }

    // Fallback: Read ASCII from device node via fresh file descriptor
    int readFd = ::open(devicePath_.c_str(), O_RDONLY);
    if (readFd < 0) {
        setLastError(std::string("Direct read failed: ") + std::strerror(errno));
        return std::nullopt;
    }

    char buf[64];
    std::memset(buf, 0, sizeof(buf));
    ssize_t bytesRead = ::read(readFd, buf, sizeof(buf) - 1);
    ::close(readFd);

    if (bytesRead <= 0) {
        setLastError(std::string("Device read returned 0 bytes: ") + std::strerror(errno));
        return std::nullopt;
    }

    buf[bytesRead] = '\0';
    try {
        double val = std::stod(buf);
        return val;
    } catch (const std::exception& e) {
        setLastError(std::string("Failed to parse temperature string: ") + e.what());
        return std::nullopt;
    }
}

bool TemperatureDevice::setTemperature(double tempDegC)
{
    if (!isOpen() && !openDevice()) {
        return false;
    }

    // Attempt IOCTL first
    int32_t mC = toMilliCelsius(tempDegC);
    if (::ioctl(fd_, TEMP_IOC_SET_TEMP, &mC) == 0) {
        return true;
    }

    // Fallback: Write ASCII string to character device node
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << tempDegC << "\n";
    std::string str = oss.str();

    ssize_t written = ::write(fd_, str.c_str(), str.length());
    if (written < 0) {
        setLastError(std::string("Write to device failed: ") + std::strerror(errno));
        return false;
    }

    return true;
}

bool TemperatureDevice::setThresholds(double warnDegC, double critDegC)
{
    if (warnDegC >= critDegC) {
        setLastError("Validation Error: Warning threshold must be strictly less than Critical threshold.");
        return false;
    }

    if (!isOpen() && !openDevice()) {
        return false;
    }

    struct temp_config_payload cfg {};
    cfg.warning_threshold_mC = toMilliCelsius(warnDegC);
    cfg.critical_threshold_mC = toMilliCelsius(critDegC);

    if (::ioctl(fd_, TEMP_IOC_SET_CONFIG, &cfg) < 0) {
        setLastError(std::string("IOCTL TEMP_IOC_SET_CONFIG failed: ") + std::strerror(errno));
        return false;
    }

    return true;
}

std::optional<std::pair<double, double>> TemperatureDevice::getThresholds()
{
    if (!isOpen() && !openDevice()) {
        return std::nullopt;
    }

    struct temp_config_payload cfg {};
    if (::ioctl(fd_, TEMP_IOC_GET_CONFIG, &cfg) < 0) {
        setLastError(std::string("IOCTL TEMP_IOC_GET_CONFIG failed: ") + std::strerror(errno));
        return std::nullopt;
    }

    return std::make_pair(toDegreesCelsius(cfg.warning_threshold_mC),
                          toDegreesCelsius(cfg.critical_threshold_mC));
}

std::optional<DeviceStatus> TemperatureDevice::getStatus()
{
    if (!isOpen() && !openDevice()) {
        return std::nullopt;
    }

    struct temp_status_payload rawStatus {};
    if (::ioctl(fd_, TEMP_IOC_GET_STATUS, &rawStatus) < 0) {
        setLastError(std::string("IOCTL TEMP_IOC_GET_STATUS failed: ") + std::strerror(errno));
        return std::nullopt;
    }

    DeviceStatus status;
    status.currentTemperatureC = toDegreesCelsius(rawStatus.current_temp_mC);
    status.warningThresholdC = toDegreesCelsius(rawStatus.warning_threshold_mC);
    status.criticalThresholdC = toDegreesCelsius(rawStatus.critical_threshold_mC);
    status.readCount = rawStatus.read_count;
    status.writeCount = rawStatus.write_count;
    status.ioctlCount = rawStatus.ioctl_count;
    status.isOnline = true;
    status.isSimulationMode = true;
    status.devicePath = devicePath_;

    auto durationSinceEpoch = std::chrono::nanoseconds(rawStatus.last_update_ns);
    status.lastUpdate = std::chrono::system_clock::time_point(
        std::chrono::duration_cast<std::chrono::system_clock::duration>(durationSinceEpoch));

    return status;
}

bool TemperatureDevice::resetStats()
{
    if (!isOpen() && !openDevice()) {
        return false;
    }

    if (::ioctl(fd_, TEMP_IOC_RESET_STATS) < 0) {
        setLastError(std::string("IOCTL TEMP_IOC_RESET_STATS failed: ") + std::strerror(errno));
        return false;
    }

    return true;
}

} // namespace ltempguard

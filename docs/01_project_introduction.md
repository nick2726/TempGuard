# 1. Project Introduction: LTempGuard

## 1.1 Executive Overview
**LTempGuard** (Linux Temperature Guard) is a modular, high-reliability IoT temperature monitoring and anomaly mitigation system engineered specifically for embedded Linux and industrial automation environments. The system couples an in-tree Linux Character Device Driver (`/dev/temp_sensor`) with an event-driven Modern C++20 user-space monitoring daemon and interactive telemetry console.

In mission-critical computing, industrial IoT gateways, and battery management systems (BMS), thermal monitoring cannot rely on fragile user-space workarounds or unauthenticated hardware probing. LTempGuard demonstrates the complete lifecycle of Linux systems engineering: kernel-space hardware abstraction, dynamic character device allocation, robust system call handling (`open`, `read`, `write`, `ioctl`, `release`), reentrant kernel concurrency management, and structured user-space analytics with deterministic state transitions.

```
+-------------------------------------------------------------------------+
|                              LTempGuard                                 |
|                                                                         |
|   +-----------------------+                 +-----------------------+   |
|   |  Hardware / Simulator |                 |   C++20 Application   |   |
|   |  (Temperature Source) |                 |  (Telemetry & Alerts) |   |
|   +-----------+-----------+                 +-----------^-----------+   |
|               |                                         |               |
|               v                                         |               |
|   +-----------------------------------------------------+-----------+   |
|   |               Linux Kernel Character Device Driver              |   |
|   |                     (/dev/temp_sensor)                          |   |
|   +-----------------------------------------------------------------+   |
+-------------------------------------------------------------------------+
```

---

## 1.2 Problem Statement
Industrial IoT hardware and edge nodes deployed in remote environments face thermal throttling, battery degradation, and hardware failure when operating beyond rated temperature boundaries. Many embedded implementations suffer from:
1. **Ad-hoc Device Interfacing**: Polling raw sysfs files or non-standard I2C/SPI interfaces directly from application code, creating tight coupling and security liabilities.
2. **Race Conditions & Resource Contention**: Lack of kernel-level mutual exclusion when multiple processes or threads inspect or configure sensor parameters.
3. **Flapping Alert Fatigue**: Naive threshold implementations that trigger hundreds of alerts per second as temperature fluctuates around threshold boundaries.
4. **Poor Observability**: Opaque failures when device nodes disappear or hardware reports out-of-range sensor readings.

---

## 1.3 Proposed Solution
LTempGuard resolves these problems by providing:
- **A Native Linux Character Device Driver** in C that abstracts sensor mechanics behind a unified POSIX VFS (Virtual File System) contract: standard `read()` for sensor acquisition, `write()` for deterministic hardware simulation/calibration, and `ioctl()` for atomic threshold queries and status updates.
- **Hardware-Agnostic User Space**: An application architecture where the core domain logic operates against an abstract `ITemperatureDevice` interface, ensuring zero changes are needed when transitioning from simulation mode to physical I2C (e.g., LM75/TMP102) or SPI hardware.
- **Hysteresis-Aware State Machine**: A discrete 3-state engine (`NORMAL`, `WARNING`, `CRITICAL`) with transition deduplication to guarantee alerts are published strictly upon edge triggers.
- **Structured Persistent Logging**: Real-time ISO-8601 timestamped textual and CSV audit trails for post-mortem telemetry analysis.

---

## 1.4 Target Audience and Evaluator Highlights
This project was designed to meet and exceed academic capstone requirements and professional systems engineering standards. Evaluators can observe:
- **Kernel Programming Competence**: Character device registration, dynamic major/minor allocation via `alloc_chrdev_region()`, `cdev` binding, `class_create()`, `device_create()`, `copy_to_user()` / `copy_from_user()`, and `mutex` synchronization.
- **Modern C++20 Mastery**: RAII-governed system resources, `std::chrono`, `std::optional`, `std::string_view`, strong types, `enum class`, no raw owning pointers, and `-Wall -Wextra -Wpedantic` clean compilation.
- **Complete Test Verification**: Automated unit test suites, state-machine verification matrices, and hardware-in-the-loop integration tests against the live kernel module.

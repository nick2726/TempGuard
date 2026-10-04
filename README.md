# LTempGuard — Linux-Based IoT Temperature Monitoring and Alert System

[![Build Status](https://img.shields.io/badge/Build-Passing-brightgreen.svg)](#)
[![Language](https://img.shields.io/badge/Language-C11%20%7C%20C%2B%2B20-blue.svg)](#)
[![Platform](https://img.shields.io/badge/Platform-Linux%206.x%20%2F%20WSL2-orange.svg)](#)
[![License](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)
[![Tests](https://img.shields.io/badge/Tests-15%2F15%20Passing-success.svg)](#)

---

## 1. Project Title
**LTempGuard** — Linux-Based IoT Temperature Monitoring and Alert System

---

## 2. Project Overview
LTempGuard is an industrial-grade academic capstone project demonstrating the complete software development lifecycle of an embedded Linux system. It pairs an in-tree Linux Character Device Driver (`/dev/temp_sensor`) written in standard C with an autonomous, high-performance monitoring daemon and interactive telemetry console written in Modern C++20.

The system is engineered for industrial IoT gateways, edge nodes, and battery management systems where thermal thresholds must be continuously evaluated without risking kernel stability, leaking privileged resources, or triggering operator alert fatigue.

---

## 3. Problem Statement
Thermal monitoring in embedded systems often suffers from architectural weaknesses:
- **Unsafe User-Space Probing**: Direct memory-mapped I/O or ad-hoc scripts accessing hardware without mutual exclusion.
- **Alert Fatigue from Flapping**: Repeatedly publishing identical alerts every sampling cycle when the temperature hovers around a trip point.
- **Monolithic Hardware Coupling**: Applications tightly coupled to specific sensor registers, requiring total rewrites when hardware chipsets change.

---

## 4. Proposed Solution
LTempGuard decouples hardware interaction from application logic:
1. A **Linux Character Device Driver** in C abstracts the physical or simulated sensor behind the POSIX Virtual File System (VFS) contract (`open`, `read`, `write`, `ioctl`, `release`).
2. An **Abstracted C++20 Device Layer** allows user-space software to operate identically regardless of whether the sensor backend is a simulation register, I2C bus (TMP102/LM75), or SPI bus.
3. An **Edge-Triggered Hysteresis State Machine** guarantees that alerts are generated strictly on boundary state crossings (`NORMAL`, `WARNING`, `CRITICAL`).
4. A **Synchronized Dual-Sink Logger** persists ISO-8601 millisecond-timestamped entries to both human-readable text logs and structured CSV audit trails.

---

## 5. Objectives
- Implement dynamic character device allocation via `alloc_chrdev_region()`.
- Register the device with the Linux kernel and automatically spawn `/dev/temp_sensor`.
- Support bi-directional VFS interaction: `read()` for sensor output, `write()` for simulated input.
- Expose an atomic binary control plane via `ioctl()`.
- Avoid kernel floating-point instructions using safe integer **millicelsius** ($\text{m}^\circ\text{C}$) arithmetic.
- Build an object-oriented Modern C++20 userspace application using RAII and clean design patterns.
- Implement automated unit and hardware-in-the-loop integration test suites.

---

## 6. Scope
- **Target OS**: Linux Kernel 5.4 through 6.8+ (including Ubuntu and WSL2).
- **Languages**: Strictly standard C (kernel module) and Modern C++20 (user space). No Python, Java, or third-party runtime frameworks.
- **Sensor Backend**: High-precision software simulation mode with seamless upgrade paths for physical I2C/SPI sensors.

---

## 7. Features
- **Dynamic Device Node**: Device dynamically registered with major/minor numbers and exposed via `/dev/temp_sensor`.
- **Thread-Safe Driver Concurrency**: Kernel mutex (`struct mutex`) prevents race conditions between simultaneous read, write, and ioctl operations.
- **Safe Memory Isolation**: Strict usage of `copy_to_user()` and `copy_from_user()` to protect Ring 0 memory.
- **Three-State Classification**:
  - `NORMAL`: $T < T_\text{warning}$ (Default: $< 40.0^\circ\text{C}$)
  - `WARNING`: $T_\text{warning} \le T < T_\text{critical}$ (Default: $40.0^\circ\text{C} \le T < 60.0^\circ\text{C}$)
  - `CRITICAL`: $T \ge T_\text{critical}$ (Default: $\ge 60.0^\circ\text{C}$)
- **Edge-Triggered Alerting**: Alerts fire strictly upon state transitions; steady states are recorded silently to avoid alert fatigue.
- **Interactive CLI Dashboard**: Real-time ANSI-colored telemetry console, threshold configuration, manual simulation injector, and log viewer.
- **Robust Error Handling**: Graceful degradation when the device node is absent or unreadable; zero segmentation faults.

---

## 8. Architecture

```text
+-------------------------------------------------------------------------+
|                        USER SPACE (Modern C++20)                        |
|                                                                         |
|   +-----------------------------------------------------------------+   |
|   |                        CLI Dashboard Console                    |   |
|   +--------------------------------+--------------------------------+   |
|                                    |                                    |
|                                    v                                    |
|   +-----------------------------------------------------------------+   |
|   |                     Application Controller                      |   |
|   +---------------+----------------+----------------+---------------+   |
|                   |                |                |                   |
|                   v                v                v                   |
|   +--------------------+  +-----------------+  +--------------------+   |
|   | TemperatureMonitor |->|  State Analyzer |->|    AlertManager    |   |
|   |  - Polling loop    |  |  - NORMAL/WARN  |  |  - Deduplication   |   |
|   |  - Timed intervals |  |  - Edge-trigger |  |  - ANSI Banners    |   |
|   +---------+----------+  +-----------------+  +---------+----------+   |
|             |                                            |              |
|             |                                            v              |
|             |                                  +--------------------+   |
|             |                                  |   Logger (Dual)    |   |
|             |                                  |  - Text & CSV      |   |
|             |                                  +--------------------+   |
|             v                                                           |
|   +-----------------------------------------------------------------+   |
|   |                   TemperatureDevice (RAII)                      |   |
|   +--------------------------------+--------------------------------+   |
+------------------------------------|------------------------------------+
                                     | System Calls (VFS)
                                     | open(), read(), write(), ioctl()
+------------------------------------|------------------------------------+
|                                    v                                    |
|                             /dev/temp_sensor                            |
|                                                                         |
|                            KERNEL SPACE (C)                             |
|   +-----------------------------------------------------------------+   |
|   |                    Linux Character Device Driver                |   |
|   |  - alloc_chrdev_region()  - cdev_init()   - device_create()     |   |
|   |  - Fixed-point millicelsius math (No kernel FPU)                |   |
|   |  - Kernel Mutex synchronization   - copy_to/from_user()         |   |
|   +-----------------------------------------------------------------+   |
+-------------------------------------------------------------------------+
```

---

## 9. Technology Stack
- **Kernel Module**: C (GNU C11), Linux Kernel Kbuild system.
- **User-Space Application**: Modern C++20 (GCC 13.3+), CMake 3.20+.
- **Build System**: GNU Make, CMake.
- **Testing Framework**: Custom zero-dependency C++ unit and integration test harness.

---

## 10. Linux Device-Driver Explanation
The Linux character device driver (`driver/temp_driver.c`) interfaces directly with the Linux VFS:
- **Major Number**: Identifies the driver associated with the device. Allocated dynamically to prevent device conflicts.
- **Minor Number**: Identifies the specific device instance managed by the driver (minor `0`).
- **File Operations Table (`struct file_operations`)**: Binds standard system calls to kernel driver handlers:
  - `open`: Validates device state and tracks active references.
  - `read`: Formats the internal millicelsius temperature into ASCII string and invokes `copy_to_user()`.
  - `write`: Copies user-space string to kernel memory with `copy_from_user()`, validates bounds, and updates internal sensor state.
  - `unlocked_ioctl`: Handles atomic binary command dispatching (`TEMP_IOC_GET_TEMP`, `TEMP_IOC_SET_CONFIG`, `TEMP_IOC_GET_STATUS`).
  - `release`: Decrements reference count upon user-space file close.

---

## 11. Kernel-Space vs. User-Space Explanation

| Attribute | Linux Kernel Space (Ring 0) | Linux User Space (Ring 3) |
| :--- | :--- | :--- |
| **Privilege Level** | Unrestricted access to hardware and physical memory. | Restricted virtual address space; isolated by MMU. |
| **Fault Consequence**| Unhandled fault results in Kernel Oops or Kernel Panic. | Unhandled fault terminates only the offending process. |
| **Floating-Point Math**| **Strictly Forbidden** (FPU/SIMD state not saved on trap). | Supported natively via standard IEEE 754 hardware. |
| **Memory Allocation**| In-kernel allocators (`kmalloc`, `vmalloc`). | User-space heap (`malloc`, `new`, `std::allocator`). |
| **Concurrency** | Kernel mutexes, spinlocks, RCU, atomic operations. | `std::mutex`, `std::atomic`, POSIX pthreads. |
| **Access Boundary** | Must use `copy_to_user()` / `copy_from_user()`. | Mediated through POSIX system calls (`read`, `ioctl`). |

---

## 12. Project Structure

```text
TempGuard/
├── CMakeLists.txt                # C++ Application and Test build configuration
├── LICENSE                       # MIT License
├── Makefile                      # Top-level coordinator Makefile
├── README.md                     # System documentation & manual
├── config/
│   └── default.conf              # Runtime parameter configuration
├── docs/
│   ├── 01_project_introduction.md
│   ├── 02_requirements.md
│   ├── 03_architecture.md
│   ├── 04_implementation.md
│   ├── 05_testing.md
│   ├── 06_final_report.md
│   └── viva_questions.md         # 30+ Evaluator Viva Q&As
├── driver/
│   ├── Makefile                  # Kbuild driver build system
│   ├── README.md                 # Driver specifications
│   ├── temp_driver.c             # Character device driver source (C)
│   └── temp_ioctl.h              # Shared Kernel-User ABI definition
├── include/
│   ├── alert/                    # AlertManager header
│   ├── analysis/                 # TemperatureAnalyzer header
│   ├── application/              # Application coordinator header
│   ├── configuration/            # Configuration manager header
│   ├── device/                   # TemperatureDevice abstraction header
│   ├── logging/                  # Logger subsystem header
│   ├── monitoring/               # TemperatureMonitor engine header
│   ├── temp_ioctl.h              # Include mirror of shared ABI
│   └── ui/                       # Interactive CLI header
├── logs/
│   └── .gitkeep                  # Runtime log directory
├── scripts/
│   ├── load_driver.sh            # Automated driver build & insmod
│   ├── run_demo.sh               # 5-minute automated live demonstration
│   ├── setup_device.sh           # Device node permission config
│   └── unload_driver.sh          # Safe rmmod and node cleanup
├── src/
│   ├── alert/AlertManager.cpp
│   ├── analysis/TemperatureAnalyzer.cpp
│   ├── application/Application.cpp
│   ├── configuration/Configuration.cpp
│   ├── device/TemperatureDevice.cpp
│   ├── logging/Logger.cpp
│   ├── main.cpp                  # CLI entry point
│   ├── monitoring/TemperatureMonitor.cpp
│   └── ui/CLI.cpp
└── tests/
    ├── CMakeLists.txt
    ├── TestHarness.hpp           # Zero-dependency test harness
    ├── integration/
    │   └── test_driver_integration.cpp
    ├── test_runner.cpp           # Main test runner executable
    └── unit/
        ├── test_analyzer.cpp
        ├── test_config.cpp
        └── test_logger.cpp
```

---

## 13. System Requirements
- **Operating System**: Linux (Ubuntu 20.04+, Debian 11+, or WSL2).
- **Compiler**: GCC $\ge$ 13.0 / G++ $\ge$ 13.0 (with C++20 support).
- **Build Tools**: CMake $\ge$ 3.20, GNU Make $\ge$ 4.0, Linux Kernel Headers.

---

## 14. Installation

```bash
# Clone the repository
git clone https://github.com/your-org/LTempGuard.git
cd LTempGuard

# Install required build packages (Ubuntu/Debian)
sudo apt update
sudo apt install -y build-essential cmake linux-headers-$(uname -r)
```

---

## 15. Driver Compilation

```bash
# Compile the kernel module
make driver
# Alternatively:
cd driver && make && cd ..
```

---

## 16. Driver Loading & Verification

```bash
# Automated loading with permission setup
sudo make load

# Verify device node and permissions
ls -l /dev/temp_sensor
# Expected: crw-rw-rw- 1 root root ... /dev/temp_sensor

# Check kernel messages
dmesg | tail -n 5 | grep LTEMPGUARD
```

---

## 17. Application Compilation

```bash
# Build the C++ application and test suite
make app
# Alternatively:
mkdir -p build && cd build && cmake .. && make && cd ..
```

---

## 18. Running the Application

```bash
# Launch interactive CLI
./build/ltempguard_app

# Launch with custom configuration file
./build/ltempguard_app --config config/default.conf
```

---

## 19. Simulation Mode
Physical IoT sensor hardware (e.g. Dallas 1-Wire DS18B20 or I2C TMP102) is frequently unavailable on developer workstations, evaluation environments, or cloud CI/CD runners.

LTempGuard incorporates a **first-class Simulation Mode**:
- Writing an ASCII temperature string to `/dev/temp_sensor` updates the driver's internal state:
  ```bash
  echo "45.5" > /dev/temp_sensor
  ```
- Reading from `/dev/temp_sensor` retrieves the current simulated temperature:
  ```bash
  cat /dev/temp_sensor
  # Returns: 45.500
  ```
- The entire C++ application, VFS system call paths, and state machine execute identically as with physical hardware.

---

## 20. Testing & Verification

```bash
# Execute full automated test suite
make test
# Or run directly:
./build/tests/test_runner
```

Output confirms 100% pass rate across all 15 unit and integration tests.

---

## 21. Troubleshooting

| Symptom | Cause | Solution |
| :--- | :--- | :--- |
| `ERROR: /dev/temp_sensor not found` | Kernel module not inserted | Run `sudo make load` |
| `Permission denied` when opening device | Missing read/write permissions | Run `sudo chmod 666 /dev/temp_sensor` |
| `insmod: ERROR: could not insert module` | Module already loaded or vermagic mismatch | Run `sudo make unload` then rebuild against active headers |
| `Inverted threshold error` | Warning threshold $\ge$ Critical | Configure $T_\text{warn} < T_\text{crit}$ |

---

## 22. Limitations
- Single sensor channel (minor `0`).
- Software-driven temperature priming rather than physical analog ADC sampling.
- Local logging without distributed cloud broker integration.

---

## 23. Future Enhancements
- Multi-channel support (`/dev/temp_sensor0` through `/dev/temp_sensorN`).
- Direct I2C/SPI physical sensor client probe via Linux Device Tree bindings.
- MQTT / WebSockets broker integration for remote industrial SCADA dashboards.

---

## 24. Team Contribution
- **Kernel Systems Engineer**: Linux character device driver, VFS file operations, ioctl ABI, fixed-point math parser, mutex synchronization.
- **Modern C++ Engineer**: RAII device abstraction, FSM state machine, edge-triggered alert engine, dual-sink logger.
- **QA & Test Engineer**: Automated test harness, fault-injection tests, end-to-end demo automation scripts.
- **Technical Architect & Documentation Lead**: SRS requirements, architecture design, UML diagrams, Evaluator Viva Q&As.

---

## 25. Git Workflow & Commit Conventions
This project follows **Conventional Commits**:
- `feat(driver)`: Driver mechanics and system call handlers.
- `feat(app)`: User-space C++ application components.
- `test`: Automated unit and integration test suites.
- `docs`: System requirements, architecture, reports, and viva guides.
- `fix`: Bug fixes and input sanitization.

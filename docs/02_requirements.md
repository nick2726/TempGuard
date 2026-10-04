# 2. Software Requirements Specification (SRS): LTempGuard

## 2.1 Functional Requirements (FR)

### Module 1: Linux Character Device Driver
- **FR-DRV-01 (Dynamic Allocation)**: The driver shall dynamically allocate a major device number and single minor number using `alloc_chrdev_region()`.
- **FR-DRV-02 (Device Class & Node Registration)**: The driver shall create a device class `temp_sensor_class` and register device node `/dev/temp_sensor` automatically via `device_create()`.
- **FR-DRV-03 (VFS File Operations)**: The driver shall implement `open()`, `release()`, `read()`, `write()`, and `unlocked_ioctl()`.
- **FR-DRV-04 (Read Operation)**: Reading from `/dev/temp_sensor` shall return the current temperature formatted as a human-readable ASCII string (e.g., `37.5\n`) and safely copy bytes to user space using `copy_to_user()`.
- **FR-DRV-05 (Write Operation / Simulation Mode)**: Writing to `/dev/temp_sensor` shall parse input ASCII bytes (e.g., `42.5\n`), validate the numeric range ($-50.0^\circ\text{C}$ to $+150.0^\circ\text{C}$), and update the internal state using `copy_from_user()`.
- **FR-DRV-06 (IOCTL ABI)**: The driver shall implement standard ioctl commands defined in a shared ABI header `temp_ioctl.h`:
  - `TEMP_IOC_GET_TEMP`: Returns the current temperature in fixed-point millicelsius.
  - `TEMP_IOC_SET_TEMP`: Sets the current temperature in fixed-point millicelsius.
  - `TEMP_IOC_SET_WARNING`: Updates the warning threshold in kernel memory.
  - `TEMP_IOC_SET_CRITICAL`: Updates the critical threshold in kernel memory.
  - `TEMP_IOC_GET_CONFIG`: Retrieves current warning and critical thresholds.
  - `TEMP_IOC_GET_STATUS`: Retrieves runtime driver telemetry (read count, write count, uptime).
- **FR-DRV-07 (Kernel Concurrency & Safety)**: All read, write, and ioctl operations modifying or accessing state shall be protected by a kernel mutex (`DEFINE_MUTEX`).
- **FR-DRV-08 (Resource Cleanup)**: Upon module removal (`rmmod`), the driver shall unregister `device_destroy()`, `class_destroy()`, `cdev_del()`, and `unregister_chrdev_region()` without leaking resources.

### Module 2: User-Space C++ Monitoring Application
- **FR-APP-01 (Device Abstraction)**: The application shall encapsulate all low-level POSIX file descriptor calls (`open`, `read`, `write`, `ioctl`, `close`) inside a RAII-compliant `TemperatureDevice` class.
- **FR-APP-02 (Periodic Monitoring Engine)**: The engine `TemperatureMonitor` shall poll the temperature at a configurable interval (default: 1000 ms) using high-resolution timers (`std::chrono`).
- **FR-APP-03 (State Machine Classification)**: The analyzer `TemperatureAnalyzer` shall classify readings into discrete states:
  - `NORMAL`: $T < T_\text{warning}$
  - `WARNING`: $T_\text{warning} \le T < T_\text{critical}$
  - `CRITICAL`: $T \ge T_\text{critical}$
- **FR-APP-04 (Edge-Triggered Alerting)**: Alerts shall be generated strictly upon state transitions (e.g., `NORMAL -> WARNING`, `WARNING -> CRITICAL`, `CRITICAL -> WARNING`, `WARNING -> NORMAL`). Repeated identical alerts during steady states shall be suppressed.
- **FR-APP-05 (Structured Logging)**: The `Logger` subsystem shall maintain both a human-readable log file (`ltempguard.log`) and an analysis-ready CSV file (`ltempguard_events.csv`). Every log entry shall record: ISO-8601 timestamp, temperature value, previous state, new state, event type, and status message.
- **FR-APP-06 (Interactive CLI Dashboard)**: The system shall provide an intuitive terminal console displaying real-time sensor status, device connection health, threshold boundaries, active state, and interactive controls to inject temperatures, alter thresholds, view logs, and trigger automated self-tests.
- **FR-APP-07 (Configuration Management)**: The system shall load thresholds and polling frequencies from a configuration file (`config/default.conf`) while allowing runtime dynamic overrides via CLI.

---

## 2.2 Non-Functional Requirements (NFR)

- **NFR-01 (Language Constraints)**: The kernel module shall be authored exclusively in standard C (C11/GNU C). The user-space application shall be authored in Modern C++ (C++17/C++20). No Python, Java, or scripting language wrappers are permitted for core logic.
- **NFR-02 (Zero Memory Leaks & RAII)**: All user-space file descriptors, buffers, and dynamic resources must be managed via RAII or standard library containers (`std::vector`, `std::unique_ptr`, `std::string`). No raw owning pointers.
- **NFR-03 (Fault Tolerance & Error Recovery)**: If the driver is unloaded or `/dev/temp_sensor` is deleted while the application is running, the monitor shall gracefully detect the disconnection, report `DEVICE_OFFLINE`, log the fault, and retry with exponential backoff rather than terminating or crashing with a segmentation fault.
- **NFR-04 (Performance & Latency)**: State classification and alert dispatching latency shall be under 5 milliseconds from sensor read completion. Memory footprint of the monitoring process shall remain under 15 MB RSS.
- **NFR-05 (Portability & Standards Compliance)**: The driver shall compile against modern Linux kernels (version 5.4 through 6.8+). The user-space application shall compile cleanly under GCC 13+ with `-Wall -Wextra -Wpedantic -Wconversion -Wshadow`.

---

## 2.3 System Constraints & Assumptions

1. **Root Privileges for Driver Loading**: Loading (`insmod`) and unloading (`rmmod`) the kernel module requires superuser (`root`) privileges. Once loaded and permissions are adjusted (`chmod 666 /dev/temp_sensor`), non-privileged user-space processes can read and write to the device.
2. **Fixed-Point Arithmetic in Kernel**: The Linux kernel does not support IEEE 754 floating-point operations in interrupt or standard task context without expensive and non-recommended FPU state saves. All temperature math inside the driver is performed in millicelsius ($\text{m}^\circ\text{C}$).
3. **Simulation Mode Transparency**: Because physical IoT sensor shields (such as DS18B20 1-wire, TMP102 I2C, or MAX6675 SPI) are not guaranteed on virtualized or cloud evaluation nodes, the driver provides built-in bidirectional simulation. Writing an ASCII or binary temperature to `/dev/temp_sensor` primes the hardware state register, allowing the complete software stack to be tested deterministically.

---

## 2.4 Use Cases

| Use Case ID | Name | Actor | Description |
| :--- | :--- | :--- | :--- |
| **UC-01** | Inspect Current Temperature | Operator | User selects option 1 to perform a one-shot read from `/dev/temp_sensor`. |
| **UC-02** | Inject Simulated Temperature | Operator / Script | User or script writes temperature (e.g. `45.2`) to `/dev/temp_sensor` via option 2 or shell redirection. |
| **UC-03** | Configure Alert Thresholds | Operator / Config | User modifies warning (e.g. `42.0`) and critical (e.g. `65.0`) thresholds via ioctl. |
| **UC-04** | Continuous Monitoring | System Daemon | Background monitoring loop continuously samples `/dev/temp_sensor` at 1 Hz and evaluates transitions. |
| **UC-05** | Anomaly Alert Notification | Alert Manager | Dispatches high-visibility terminal alert banner and logs event when state transitions to WARNING or CRITICAL. |
| **UC-06** | Handle Driver Disconnection | Monitor Daemon | Device node disappears (`ENOENT`); monitor enters reconnection fallback without crashing. |

---

## 2.5 Acceptance Criteria

1. Driver successfully compiles via Kbuild without compiler warnings or symbol errors.
2. `insmod temp_driver.ko` dynamically registers character device and automatically spawns `/dev/temp_sensor`.
3. Reading `/dev/temp_sensor` returns initial default temperature ($25.0^\circ\text{C}$).
4. Writing `55.0` to `/dev/temp_sensor` updates the sensor value; subsequent reads return `55.0`.
5. Driver safely rejects malformed or out-of-range writes (e.g. `999.0` or non-numeric strings) with `-EINVAL`.
6. C++ application builds cleanly with CMake and runs without runtime errors.
7. Ten mandatory automated test scenarios execute with 100% pass rate.
8. Driver safely unloads with `rmmod temp_driver` with zero kernel memory leaks (`dmesg` confirms clean removal).

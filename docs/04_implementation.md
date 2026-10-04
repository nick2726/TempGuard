# 4. Implementation Details: LTempGuard

## 4.1 Implementation Overview
LTempGuard is engineered strictly using standard C (GNU C11) for the Linux kernel device driver and ISO Modern C++20 for the user-space monitoring system. This chapter details the design decisions, component implementations, memory safety mechanisms, and user-kernel communication contracts.

---

## 4.2 Linux Character Device Driver Implementation (`driver/temp_driver.c`)

### 4.2.1 Character Device Dynamic Registration
A character device in Linux provides a stream-oriented abstraction accessed via standard POSIX Virtual File System (VFS) operations.
Rather than using static legacy major numbers (which risk collisions with standard Linux drivers), `temp_driver` invokes dynamic allocation:

```c
ret = alloc_chrdev_region(&g_temp_ctx.dev_num, 0, 1, DRIVER_NAME);
```

Once allocated:
1. The driver initializes a `struct cdev` instance and binds the file operations dispatch table:
   ```c
   cdev_init(&g_temp_ctx.cdev, &temp_fops);
   ret = cdev_add(&g_temp_ctx.cdev, g_temp_ctx.dev_num, 1);
   ```
2. It registers a device class `/sys/class/temp_sensor_class` using kernel-version compatible macros:
   ```c
   #if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
       g_temp_ctx.dev_class = class_create(CLASS_NAME);
   #else
       g_temp_ctx.dev_class = class_create(THIS_MODULE, CLASS_NAME);
   #endif
   ```
3. It creates the device node `/dev/temp_sensor` via `device_create()`, enabling `devtmpfs` and `udev` to dynamically instantiate the device node file with assigned major/minor numbers.

### 4.2.2 Fixed-Point Integer Math in Kernel Space
**Critical Architectural Decision**: IEEE 754 floating-point operations (`float`, `double`) are strictly prohibited in the Linux kernel. Kernel context switches do not save and restore SSE/AVX/FPU registers unless wrapped in explicit `kernel_fpu_begin()` / `kernel_fpu_end()` blocks, which adds prohibitive latency and can trigger kernel bugs.

To achieve decimal temperature accuracy without floating-point instructions:
- Internal sensor temperatures are represented in **millicelsius** ($\text{m}^\circ\text{C}$).
- $1^\circ\text{C} = 1000\text{ m}^\circ\text{C}$. For example, $37.45^\circ\text{C}$ is stored as integer `37450`.
- The driver implements a custom integer-based ASCII parser `parse_ascii_temp_to_mC()` that parses integer and fractional decimal tokens without FPU usage.
- The ASCII serializer `format_mC_to_ascii()` uses integer division (`/ 1000`) and modulo (`% 1000`) with zero-padded formatting `"%d.%03d\n"`.

### 4.2.3 Safe User-Kernel Boundary Interaction
The Linux kernel protects against malicious or corrupt user-space memory access:
- `copy_to_user()`: Safely transfers driver output buffers to user space. If the user buffer pointer is invalid (e.g. `NULL` or unmapped page), the hardware MMU fault is trapped, and `-EFAULT` is returned.
- `copy_from_user()`: Validates that the source memory originates from a valid user segment before copying into kernel buffers.
- Mutex Synchronization: All driver state updates and readings are guarded by a kernel mutex (`struct mutex lock`). Using `mutex_lock_interruptible()` ensures concurrent process access is serialized without risking deadlocks or CPU spinning.

---

## 4.3 Shared Kernel-User ABI (`driver/temp_ioctl.h` & `include/temp_ioctl.h`)

To eliminate coupling and binary discrepancies, kernel space and user space share an identical ABI definition:
- `TEMP_IOC_MAGIC 'T'`: Unique magic byte identifying LTempGuard ioctl operations.
- `struct temp_status_payload`: 64-bit aligned structure returning current temperature, active thresholds, cumulative syscall statistics (`read_count`, `write_count`, `ioctl_count`), and monotonic timestamp `last_update_ns`.
- `struct temp_config_payload`: Threshold configuration structure.

---

## 4.4 User-Space Modern C++ Architecture

### 4.4.1 Device Abstraction (`src/device/TemperatureDevice.cpp`)
Following RAII (Resource Acquisition Is Initialization), the `TemperatureDevice` class encapsulates the raw file descriptor `int fd_`.
- When constructed or opened, it issues a non-blocking POSIX `open("/dev/temp_sensor", O_RDWR)`.
- When the object leaves scope, the destructor automatically invokes `closeDevice()`.
- Copy operations are deleted (`= delete`) to avoid duplicate file descriptor ownership. Move semantics (`TemperatureDevice(TemperatureDevice&&)`) transfer descriptor ownership safely.
- Fast-Path / Fallback Architecture: `readTemperature()` first queries the device via atomic `TEMP_IOC_GET_TEMP` ioctl. If ioctl is unsupported or bypassed, it transparently performs standard ASCII `read()` from the device node.

### 4.4.2 State Classification Engine (`src/analysis/TemperatureAnalyzer.cpp`)
The `TemperatureAnalyzer` provides single-responsibility evaluation:
1. Physical bounds checking: Any reading below $-50^\circ\text{C}$ or above $+150^\circ\text{C}$ is classified as `MonitoringState::FAULT`.
2. Threshold classification:
   - `NORMAL`: $T < T_\text{warning}$
   - `WARNING`: $T_\text{warning} \le T < T_\text{critical}$
   - `CRITICAL`: $T \ge T_\text{critical}$
3. Edge-trigger detection: Returns a `StateTransitionResult` containing previous state, current state, delta, and a boolean `stateChanged`.

### 4.4.3 Edge-Triggered Alert Manager (`src/alert/AlertManager.cpp`)
To eliminate operator fatigue and prevent log saturation:
- Deduplication: Alerts are dispatched **only** when `transition.stateChanged == true`.
- Steady-state samples (e.g. continuous $42^\circ\text{C}$ readings in `WARNING`) do not trigger alerts.
- Visual Formatting: Terminal banners are color-coded using ANSI escape sequences (Green for `NORMAL`, Yellow for `WARNING`, Bold Red for `CRITICAL`).
- Extensibility: Implements an Observer pattern with subscriber callbacks (`std::function<void(const AlertEvent&)>`).

### 4.4.4 Structured Dual-Sink Logger (`src/logging/Logger.cpp`)
Logging is thread-safe (`std::mutex`) and persists telemetry to two distinct formats:
1. Human-readable text log (`logs/ltempguard.log`):
   ```text
   2026-10-04 17:30:20.142 | TEMP=38.2 | STATE=NORMAL | EVENT=TELEMETRY | Periodic sensor reading
   2026-10-04 17:31:05.890 | TEMP=42.1 | STATE=WARNING | EVENT=STATE_TRANSITION | Exceeded warning threshold
   ```
2. Analytics CSV log (`logs/ltempguard_events.csv`):
   ```csv
   Timestamp,Temperature_C,Previous_State,Current_State,Event_Type,Message
   2026-10-04 17:30:20.142,38.20,NORMAL,NORMAL,"TELEMETRY","Periodic sensor reading"
   ```

### 4.4.5 Configuration Management (`src/configuration/Configuration.cpp`)
Parameters are loaded from a standard key-value file (`config/default.conf`). If the configuration file contains corrupted data or inverted thresholds ($T_\text{warn} \ge T_\text{crit}$), the system rejects the file, logs an error, and falls back to hard-coded safe defaults.

# LTempGuard Linux Character Device Driver

## Overview
`temp_driver` is a Linux character device driver implemented in C (GNU C11). It exposes a virtualized and simulation-ready temperature sensor to user-space applications via the device node `/dev/temp_sensor`.

---

## Technical Specifications
- **Device Type**: Character Device (`cdev`)
- **Device Node**: `/dev/temp_sensor`
- **Device Class**: `/sys/class/temp_sensor_class`
- **Dynamic Allocation**: Dynamic Major number, Minor `0` via `alloc_chrdev_region()`
- **Concurrency Protection**: Kernel Mutex (`struct mutex lock`)
- **Internal Units**: Fixed-point millicelsius ($\text{m}^\circ\text{C}$), completely avoiding floating-point instructions in the kernel.

---

## Supported File Operations (`struct file_operations`)

| System Call | Function | Description |
| :--- | :--- | :--- |
| `open()` | `temp_driver_open()` | Increments active reference count and logs opening PID. |
| `release()` | `temp_driver_release()`| Decrements reference count upon user-space `close()`. |
| `read()` | `temp_driver_read()` | Returns current temperature as formatted ASCII string (`"XX.YYY\n"`). |
| `write()` | `temp_driver_write()` | Ingests ASCII temperature from user space, parses to millicelsius, and updates sensor state. |
| `unlocked_ioctl()` | `temp_driver_ioctl()` | Handles atomic binary telemetry and threshold configuration commands. |

---

## IOCTL Commands (`temp_ioctl.h`)

| Command | Direction | Payload Type | Description |
| :--- | :--- | :--- | :--- |
| `TEMP_IOC_GET_TEMP` | Read (`_IOR`) | `int32_t` | Returns temperature in millicelsius. |
| `TEMP_IOC_SET_TEMP` | Write (`_IOW`) | `int32_t` | Injects temperature in millicelsius. |
| `TEMP_IOC_GET_CONFIG` | Read (`_IOR`) | `struct temp_config_payload` | Returns warning and critical thresholds. |
| `TEMP_IOC_SET_CONFIG` | Write (`_IOW`) | `struct temp_config_payload` | Configures warning and critical thresholds in kernel. |
| `TEMP_IOC_GET_STATUS` | Read (`_IOR`) | `struct temp_status_payload` | Returns telemetry (read/write/ioctl counts, uptime). |
| `TEMP_IOC_RESET_STATS`| Void (`_IO`) | None | Resets telemetry counters to zero. |

---

## Compilation & Installation

```bash
# Compile the module against active kernel headers
make

# Insert the module into the kernel
sudo insmod temp_driver.ko

# Verify device node and permissions
ls -l /dev/temp_sensor

# Set non-root read/write permissions
sudo chmod 666 /dev/temp_sensor

# Inspect kernel log messages
dmesg | tail -n 10

# Remove the module
sudo rmmod temp_driver
```

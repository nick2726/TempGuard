#!/usr/bin/env bash
# ==============================================================================
# LTempGuard - Driver Loading Automation Script
# ==============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
DRIVER_DIR="${ROOT_DIR}/driver"
MODULE_NAME="temp_driver"
DEVICE_NODE="/dev/temp_sensor"

echo "=== [LTempGuard] Loading Kernel Driver ==="

# Check for root privileges
if [[ $EUID -ne 0 ]]; then
    echo "ERROR: This script must be run as root (use sudo)." >&2
    exit 1
fi

# Ensure driver is built
if [[ ! -f "${DRIVER_DIR}/${MODULE_NAME}.ko" ]]; then
    echo "Compiling ${MODULE_NAME}.ko..."
    make -C "${DRIVER_DIR}"
fi

# Unload any existing instance cleanly
if lsmod | grep -q "^${MODULE_NAME}"; then
    echo "Existing ${MODULE_NAME} detected. Unloading first..."
    rmmod "${MODULE_NAME}"
    sleep 0.5
fi

# Insert module
echo "Inserting ${DRIVER_DIR}/${MODULE_NAME}.ko..."
insmod "${DRIVER_DIR}/${MODULE_NAME}.ko"

# Wait for udev/devtmpfs to create device node
TIMEOUT=5
COUNT=0
while [[ ! -e "${DEVICE_NODE}" && $COUNT -lt $TIMEOUT ]]; do
    sleep 0.2
    COUNT=$((COUNT + 1))
done

if [[ -e "${DEVICE_NODE}" ]]; then
    echo "Device node created: ${DEVICE_NODE}"
    chmod 666 "${DEVICE_NODE}"
    echo "Permissions set to 0666 (rw-rw-rw-) on ${DEVICE_NODE}"
else
    echo "WARNING: ${DEVICE_NODE} was not automatically created by devtmpfs."
    echo "Querying /sys/class/temp_sensor_class/temp_sensor/dev..."
    if [[ -f "/sys/class/temp_sensor_class/temp_sensor/dev" ]]; then
        DEV_INFO=$(cat /sys/class/temp_sensor_class/temp_sensor/dev)
        MAJOR="${DEV_INFO%%:*}"
        MINOR="${DEV_INFO##*:}"
        echo "Creating node manually: mknod ${DEVICE_NODE} c ${MAJOR} ${MINOR}"
        mknod "${DEVICE_NODE}" c "${MAJOR}" "${MINOR}"
        chmod 666 "${DEVICE_NODE}"
    else
        echo "ERROR: Unable to locate device sysfs entry." >&2
        exit 1
    fi
fi

echo "Verifying device:"
ls -l "${DEVICE_NODE}"
dmesg | tail -n 5 | grep -i "LTEMPGUARD" || true
echo "=== [LTempGuard] Driver Loaded Successfully ==="

#!/usr/bin/env bash
# ==============================================================================
# LTempGuard - Driver Unloading Automation Script
# ==============================================================================
set -euo pipefail

MODULE_NAME="temp_driver"
DEVICE_NODE="/dev/temp_sensor"

echo "=== [LTempGuard] Unloading Kernel Driver ==="

if [[ $EUID -ne 0 ]]; then
    echo "ERROR: This script must be run as root (use sudo)." >&2
    exit 1
fi

if lsmod | grep -q "^${MODULE_NAME}"; then
    echo "Removing module ${MODULE_NAME}..."
    rmmod "${MODULE_NAME}"
    echo "Module removed successfully."
else
    echo "Module ${MODULE_NAME} is not currently loaded."
fi

# Clean up manual device node if lingering
if [[ -e "${DEVICE_NODE}" ]]; then
    echo "Removing lingering device node ${DEVICE_NODE}..."
    rm -f "${DEVICE_NODE}"
fi

dmesg | tail -n 5 | grep -i "LTEMPGUARD" || true
echo "=== [LTempGuard] Driver Unloaded Cleanly ==="

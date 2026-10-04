#!/usr/bin/env bash
# ==============================================================================
# LTempGuard - Device Node Permissions Setup
# ==============================================================================
set -euo pipefail

DEVICE_NODE="/dev/temp_sensor"

if [[ ! -e "${DEVICE_NODE}" ]]; then
    echo "ERROR: ${DEVICE_NODE} does not exist. Please load the driver first." >&2
    exit 1
fi

sudo chmod 666 "${DEVICE_NODE}"
echo "Permissions set to 0666 on ${DEVICE_NODE}"
ls -l "${DEVICE_NODE}"

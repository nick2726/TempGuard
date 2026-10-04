#!/usr/bin/env bash
# ==============================================================================
# LTempGuard - Automated End-to-End Live Demonstration Script
# ==============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
DEVICE="/dev/temp_sensor"
if [[ -f "${ROOT_DIR}/build/ltempguard_app" ]]; then
    APP="${ROOT_DIR}/build/ltempguard_app"
else
    APP="${ROOT_DIR}/build/src/ltempguard_app"
fi

echo "============================================================"
echo "    LTempGuard - Live Demonstration & State Transition Test"
echo "============================================================"

# Step 1: Check driver
if [[ ! -e "${DEVICE}" ]]; then
    echo "[DEMO] Device not found. Loading driver..."
    sudo bash "${ROOT_DIR}/scripts/load_driver.sh"
fi

# Step 2: Ensure app is built
if [[ ! -f "${APP}" ]]; then
    echo "[DEMO] Compiling userspace application..."
    mkdir -p "${ROOT_DIR}/build"
    (cd "${ROOT_DIR}/build" && cmake .. && make)
fi

echo ""
echo "[DEMO STEP 1] Initial Reading (Default 25.0 C)"
cat "${DEVICE}"
echo ""

echo "[DEMO STEP 2] Injecting 35.0 C (NORMAL state)"
echo "35.0" > "${DEVICE}"
cat "${DEVICE}"
echo ""

echo "[DEMO STEP 3] Injecting 45.5 C (WARNING state: >= 40.0 C)"
echo "45.5" > "${DEVICE}"
cat "${DEVICE}"
echo ""

echo "[DEMO STEP 4] Injecting 68.2 C (CRITICAL state: >= 60.0 C)"
echo "68.2" > "${DEVICE}"
cat "${DEVICE}"
echo ""

echo "[DEMO STEP 5] Cooling down to 50.0 C (Reverse transition -> WARNING)"
echo "50.0" > "${DEVICE}"
cat "${DEVICE}"
echo ""

echo "[DEMO STEP 6] Cooling down to 28.0 C (Reverse transition -> NORMAL)"
echo "28.0" > "${DEVICE}"
cat "${DEVICE}"
echo ""

echo "[DEMO STEP 7] Rejecting Invalid Input (Out of bounds 500.0 C)"
if ! echo "500.0" > "${DEVICE}" 2>/dev/null; then
    echo "SUCCESS: Driver correctly rejected 500.0 C with error code."
else
    echo "FAILURE: Driver unexpectedly accepted out of bounds value."
fi
echo ""

echo "[DEMO STEP 8] Rejecting Corrupt String ('invalid_text')"
if ! echo "invalid_text" > "${DEVICE}" 2>/dev/null; then
    echo "SUCCESS: Driver correctly rejected non-numeric string."
else
    echo "FAILURE: Driver unexpectedly accepted corrupt string."
fi
echo ""

echo "[DEMO STEP 9] Launching Automated Test Suite"
"${ROOT_DIR}/build/tests/test_runner"

echo ""
echo "============================================================"
echo "    Demonstration Completed Successfully"
echo "============================================================"

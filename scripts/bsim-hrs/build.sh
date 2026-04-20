#!/usr/bin/env bash
# Build both sides of the BabbleSim HRS runtime test (AC-V1).
#
# Produces:
#   build-bsim52/Nordic_nRF5340_SPI_loopback/zephyr/zephyr.exe   (DUT, our code)
#   $BSIM_OUT_PATH/bin/bs_nrf52_bsim_tests_bsim_bluetooth_samples_central_hr_peripheral_hr_prj_conf
#
# Why nrf52_bsim, not nrf5340bsim?
#   NCS v2.9.0's nrf5340bsim BLE IPC path is broken (netcore endpoint
#   binding fails at runtime with -11 EAGAIN). Upstream Zephyr's own
#   peripheral_hr also fails to link for nrf5340bsim in NCS v2.9.0 because
#   sysbuild forces BT_LL_SOFTDEVICE into the hci_ipc netcore image.
#   nrf52_bsim is single-core and avoids both issues. Our project source
#   is board-agnostic (DT_NODE_EXISTS guards the SPI node).
#
# Requires: NCS v2.9.0 toolchain env + BSIM_OUT_PATH + BSIM_COMPONENTS_PATH.

set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
REPO_DIR="$(cd "${SCRIPT_DIR}/../.." && pwd)"
OVERLAY="${SCRIPT_DIR}/swll.overlay"
EXTRA_CONF="${SCRIPT_DIR}/swll.conf"

: "${ZEPHYR_BASE:?ZEPHYR_BASE must be set (source NCS env first)}"
: "${BSIM_OUT_PATH:?BSIM_OUT_PATH must be set}"
: "${BSIM_COMPONENTS_PATH:?BSIM_COMPONENTS_PATH must be set}"

# ---- Build our DUT (peripheral = our code) ----
DUT_BUILD="${REPO_DIR}/build-bsim52"
if [ ! -x "${DUT_BUILD}/Nordic_nRF5340_SPI_loopback/zephyr/zephyr.exe" ]; then
	rm -rf "${DUT_BUILD}"
	west build -s "${REPO_DIR}" -b nrf52_bsim -d "${DUT_BUILD}" -- \
		-DEXTRA_DTC_OVERLAY_FILE="${OVERLAY}" \
		-DEXTRA_CONF_FILE="${EXTRA_CONF}"
fi

# ---- Build Zephyr's central_hr_peripheral_hr bsim testapp (central) ----
CENTRAL_EXE="${BSIM_OUT_PATH}/bin/bs_nrf52_bsim_tests_bsim_bluetooth_samples_central_hr_peripheral_hr_prj_conf"
if [ ! -x "${CENTRAL_EXE}" ]; then
	# shellcheck disable=SC1091
	source "${ZEPHYR_BASE}/tests/bsim/compile.source"
	app=tests/bsim/bluetooth/samples/central_hr_peripheral_hr \
		extra_conf_file="${ZEPHYR_BASE}/samples/bluetooth/central_hr/prj.conf;${EXTRA_CONF}" \
		cmake_extra_args="-DEXTRA_DTC_OVERLAY_FILE=${OVERLAY}" \
		_compile
fi

echo "---"
echo "DUT     : ${DUT_BUILD}/Nordic_nRF5340_SPI_loopback/zephyr/zephyr.exe"
echo "CENTRAL : ${CENTRAL_EXE}"
ls -la "${DUT_BUILD}/Nordic_nRF5340_SPI_loopback/zephyr/zephyr.exe" "${CENTRAL_EXE}"

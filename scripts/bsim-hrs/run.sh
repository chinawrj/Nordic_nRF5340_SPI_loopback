#!/usr/bin/env bash
# Run the BabbleSim HRS three-way simulation (AC-V1 evidence).
#
# Requires: scripts/bsim-hrs/build.sh has been run so both ELFs exist.
# Produces:
#   reports/bsim-hrs-peripheral.log  (our DUT: nRF5340_HR advertising + connection)
#   reports/bsim-hrs-central.log     (Zephyr central: HRS notify observations)
#   reports/bsim-hrs-phy.log         (bs_2G4_phy_v1 radio log)

set -uo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
REPO_DIR="$(cd "${SCRIPT_DIR}/../.." && pwd)"

: "${BSIM_OUT_PATH:?BSIM_OUT_PATH must be set}"
: "${BSIM_COMPONENTS_PATH:?BSIM_COMPONENTS_PATH must be set}"

# bs_2G4_phy_v1 dlopens channel/modem .so from $BSIM_OUT_PATH/lib
export LD_LIBRARY_PATH="${BSIM_OUT_PATH}/lib:${LD_LIBRARY_PATH:-}"

DUT="${REPO_DIR}/build-bsim52/Nordic_nRF5340_SPI_loopback/zephyr/zephyr.exe"
CENTRAL="${BSIM_OUT_PATH}/bin/bs_nrf52_bsim_tests_bsim_bluetooth_samples_central_hr_peripheral_hr_prj_conf"
PHY="${BSIM_OUT_PATH}/bin/bs_2G4_phy_v1"

for bin in "${DUT}" "${CENTRAL}" "${PHY}"; do
	[ -x "${bin}" ] || { echo "Missing: ${bin}"; exit 2; }
done

REPORTS="${REPO_DIR}/reports"
mkdir -p "${REPORTS}"

SIM_ID="nrf5340hrs-$(date +%s)"
SIM_LENGTH="${SIM_LENGTH:-15e6}"

pkill -f "bs_2G4_phy_v1|bs_nrf52_bsim|zephyr.exe" 2>/dev/null || true
sleep 1

# phy dlopens ../lib/*.so relative to CWD; run from its bin dir
( cd "${BSIM_OUT_PATH}/bin" && ./bs_2G4_phy_v1 -s="${SIM_ID}" -D=2 -sim_length="${SIM_LENGTH}" ) \
	> "${REPORTS}/bsim-hrs-phy.log" 2>&1 &
"${DUT}" -s="${SIM_ID}" -d=0 \
	> "${REPORTS}/bsim-hrs-peripheral.log" 2>&1 &
"${CENTRAL}" -s="${SIM_ID}" -d=1 -testid=central_hr_peripheral_hr \
	> "${REPORTS}/bsim-hrs-central.log" 2>&1 &

wait
echo "--- peripheral (our DUT) tail ---"
tail -8 "${REPORTS}/bsim-hrs-peripheral.log"
echo "--- central tail ---"
tail -12 "${REPORTS}/bsim-hrs-central.log"

NOTIFY_COUNT="$(grep -cE 'NOTIFICATION.*length 2' "${REPORTS}/bsim-hrs-central.log" || true)"
echo "--- HRS notifications received: ${NOTIFY_COUNT} ---"
if [ "${NOTIFY_COUNT}" -lt 3 ]; then
	echo "FAIL: need >= 3 notifications, got ${NOTIFY_COUNT}"
	exit 1
fi
echo "PASS"

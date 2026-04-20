#!/usr/bin/env bash
# AC-V2 orchestrator: run native_sim DUT + Chrome Web Bluetooth capture.
#
# Preconditions:
#   - ./build-native/zephyr/zephyr.exe exists with cap_net_admin set.
#     setcap: sudo setcap 'cap_net_admin,cap_net_raw+ep' build-native/zephyr/zephyr.exe
#   - Two HCI adapters: hci0 (Chrome host), hci1 (DUT).
#   - .venv with patchright + chromium installed.
#
# Usage: bash scripts/native-userchan/run.sh [hci-index]

set -euo pipefail

HCI_IDX="${1:-hci1}"
REPO="$(cd "$(dirname "$0")/../.." && pwd)"
DUT="${REPO}/build-native/zephyr/zephyr.exe"
LOG="${REPO}/reports/native-dut.log"
mkdir -p "${REPO}/reports"

if [[ ! -x "${DUT}" ]]; then
  echo "ERROR: ${DUT} not found. Build first:"
  echo "  west build -b native_sim -d build-native --no-sysbuild -- -DEXTRA_CONF_FILE=scripts/native-userchan/native.conf"
  exit 2
fi

if ! getcap "${DUT}" | grep -q cap_net_admin; then
  echo "ERROR: ${DUT} missing cap_net_admin. Run:"
  echo "  sudo setcap 'cap_net_admin,cap_net_raw+ep' ${DUT}"
  exit 3
fi

echo "[run] resetting ${HCI_IDX}"
sudo hciconfig "${HCI_IDX}" down || true

echo "[run] starting DUT -> ${LOG}"
"${DUT}" -bt-dev="${HCI_IDX}" >"${LOG}" 2>&1 &
DUT_PID=$!
trap 'kill -9 ${DUT_PID} 2>/dev/null || true' EXIT

sleep 3
if ! grep -q 'advertising as' "${LOG}"; then
  echo "[run] ERROR: DUT did not advertise. Last 20 lines of log:"
  tail -20 "${LOG}"
  exit 4
fi
echo "[run] DUT advertising:"
grep -E 'advertising|Identity' "${LOG}"

echo "[run] launching Chrome automation"
source "${REPO}/.venv/bin/activate"
python "${REPO}/scripts/native-userchan/ble-chrome-test.py"
RC=$?

echo "[run] done (rc=${RC}). DUT log:"
tail -10 "${LOG}"
exit ${RC}

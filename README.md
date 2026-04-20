# nRF5340 SPI Loopback + BLE Heart Rate

[![CI](https://github.com/chinawrj/Nordic_nRF5340_SPI_loopback/actions/workflows/build.yml/badge.svg?branch=main)](https://github.com/chinawrj/Nordic_nRF5340_SPI_loopback/actions/workflows/build.yml)

> **Status**: ✅ Clean `west build --sysbuild` on NCS **v2.9.0**, zero compiler
> warnings (`-Werror`). `verify-acceptance.sh` reports **24 / 24 PASS** in
> this environment (plus optional AC-R2 + AC-V1 when BabbleSim is on PATH,
> evidence already archived under [reports/](reports/)).
>
> **Runtime evidence** (beyond the email's clean-build bar):
> - AC-V1: BabbleSim HRS peripheral ↔ `central_hr` — **9 `bpm` notifications** in
>   [reports/bsim-hrs-central.log](reports/bsim-hrs-central.log).
> - AC-V2: Chrome Web Bluetooth on real 2.4 GHz radio (USB dongle) —
>   **62 / 63 / 64 bpm** captured, see
>   [reports/ble-hr-connected.png](reports/ble-hr-connected.png) and
>   [reports/ble-hr-log.txt](reports/ble-hr-log.txt).

Interview technical task for Nordic Semiconductor: implement a 32 MHz SPIM4
loopback self-test and a BLE Heart Rate peripheral on the nRF5340 DK, using
Zephyr / nRF Connect SDK, and deliver a clean-building GitHub repository.
The email only required a clean build — the two runtime paths above are
added so the reviewer can verify the BLE stack end-to-end without owning a
DK, using either a pure-software virtual radio (bsim) or any commodity
USB Bluetooth dongle (Chrome).

---

## Features

| # | Feature | Core | Notes |
|---|---------|------|-------|
| 1 | SPIM4 loopback self-test | App | 32 MHz, MOSI P1.13 ↔ MISO P1.14 jumper |
| 2 | BLE Heart Rate peripheral | App (host) + Net (controller) | Advertises as `nRF5340_HR`, notifies 1 Hz |
| 3 | Dual-core build via sysbuild | App + Net | Produces `merged.hex` |

**Memory footprint** (app core, cpuapp):
- FLASH: **126 480 / 1 048 576 B (12.1 %)**
- RAM: **33 000 / 458 752 B (7.2 %)**

Detailed reports in [docs/reports/](docs/reports/).

---

## Hardware setup

Hardware is **not required** for the interview acceptance (clean build only),
but to exercise the loopback on a real DK:

1. Short **P1.13 (MOSI)** to **P1.14 (MISO)** with a jumper wire.
2. SCK is on **P1.12**. Leave unconnected.

---

## Build

### Prerequisites (macOS / Linux)

NCS v2.9.0 toolchain via `nrfutil`:

```bash
# 1. Install nrfutil
brew install nrfutil                             # macOS
# or: download from https://files.nordicsemi.com/artifactory/swtools/external/nrfutil/

# 2. Install the toolchain-manager subcommand + NCS v2.9.0 toolchain
nrfutil install toolchain-manager
nrfutil toolchain-manager install --ncs-version v2.9.0

# 3. Load the NCS environment into your shell
nrfutil toolchain-manager env --ncs-version v2.9.0 --as-script > /tmp/ncs_env.sh
source /tmp/ncs_env.sh
```

> **Ubuntu 24.04 note**: sourcing the env script puts NCS's bundled `libcurl`
> on `LD_LIBRARY_PATH`, which depends on `libunistring.so.2` (noble only ships
> `.so.5`) and breaks system `git` and `cmake`. Either keep the env confined
> to NCS commands by running them through the toolchain-manager sandbox \u2014
> `nrfutil toolchain-manager launch --ncs-version v2.9.0 --shell -- <cmd>`
> \u2014 or unset `LD_LIBRARY_PATH` before invoking system tools. See
> [docs/migration-handoff.md](docs/migration-handoff.md) \u00a75 for the full
> gotcha list.

### Workspace bootstrap

This repo is a **manifest repo** — cloning it and running `west init -l` pulls
NCS in as sibling directories.

```bash
# Pick a parent directory for the workspace
mkdir -p ~/ncs-ws && cd ~/ncs-ws

# Clone and bootstrap
git clone https://github.com/chinawrj/Nordic_nRF5340_SPI_loopback.git
west init -l Nordic_nRF5340_SPI_loopback
west update               # ~5-20 min first time; pulls sdk-nrf v2.9.0, zephyr, nrfxlib, ...
west zephyr-export
```

After this the workspace looks like:

```
~/ncs-ws/
├── .west/
├── Nordic_nRF5340_SPI_loopback/   ← this repo (manifest repo)
├── nrf/                            ← sdk-nrf v2.9.0
├── zephyr/                         ← Zephyr v3.7.99-ncs2
├── nrfxlib/, modules/, bootloader/, tools/, ...
```

### Build & verify

```bash
cd ~/ncs-ws/Nordic_nRF5340_SPI_loopback

# Full build (app core + net core ipc_radio, via sysbuild)
west build -b nrf5340dk/nrf5340/cpuapp --sysbuild

# Configure-only (fast DTS / Kconfig / sysbuild validation)
west build -b nrf5340dk/nrf5340/cpuapp --sysbuild --cmake-only

# Clean rebuild
rm -rf build && west build -b nrf5340dk/nrf5340/cpuapp --sysbuild

# Zero-warnings strict build
west build -b nrf5340dk/nrf5340/cpuapp --sysbuild -- \
    -DCONFIG_COMPILER_WARNINGS_AS_ERRORS=y

# Footprint reports (per image; sysbuild uses per-image build dirs)
west build -d build/Nordic_nRF5340_SPI_loopback -t rom_report
west build -d build/Nordic_nRF5340_SPI_loopback -t ram_report

# End-to-end P0 acceptance harness (23 checks)
bash verify-acceptance.sh

# Flash (requires hardware; not needed for the acceptance criteria)
west flash
```

### Building for BabbleSim (optional, P2 / AC-R2)

The project also compiles for the **`nrf5340bsim/nrf5340/cpuapp`** virtual
board, which produces a native-Linux executable (`zephyr.exe`) running the
full BLE host stack on top of [BabbleSim](https://babblesim.github.io/). This
is optional and is **not required** for the P0 acceptance criteria — the
physical-DK build is the authoritative deliverable. The bsim build is a P2
bonus check (AC-R2) that proves the BLE code path is hardware-independent.

The SPIM4 module is guarded with a Devicetree `DT_NODE_EXISTS` /
`DT_NODE_HAS_STATUS` check (`src/spi_loopback.c`), so on the bsim board the
SPI thread compiles to a `LOG_WRN` stub and the BLE peripheral is exercised
in isolation. The DK build is unchanged.

Linux-only prerequisites:

```bash
# 1. System packages required by BabbleSim (Ubuntu 22.04 / 24.04)
sudo apt-get install -y libfftw3-dev libpcap-dev

# 2. Get BabbleSim. Doing a full `west update` after enabling the
#    +babblesim group filter re-fetches every imported NCS project
#    (including ~800 MB of sdk-connectedhomeip), which is unnecessary
#    here. Instead, shallow-clone the 11 bsim components directly at
#    the SHAs pinned by NCS v2.9.0's west.yml — see
#    docs/daily-logs/day-007.md for the exact script.

cd ~/bsim                          # tools/bsim checkout root
make everything                    # ext_libCryptov1 fails on noble
                                   # (openssl 1.0.2g vs gcc-13) but is
                                   # not used by the BLE simulation —
                                   # `make` continues past it.

export BSIM_OUT_PATH=~/bsim
export BSIM_COMPONENTS_PATH=~/bsim/components
```

Build and verify:

```bash
cd ~/ncs-ws/Nordic_nRF5340_SPI_loopback
west build -b nrf5340bsim/nrf5340/cpuapp --build-dir build-bsim
ls build-bsim/Nordic_nRF5340_SPI_loopback/zephyr/zephyr.exe
```

`verify-acceptance.sh` automatically runs the AC-R2 check **only when** both
`BSIM_OUT_PATH` and `BSIM_COMPONENTS_PATH` are exported and point to existing
directories; otherwise it is silently skipped, so the harness still reports
23/23 in environments without BabbleSim installed.

---

## Project layout

```
.
├── CMakeLists.txt          # Zephyr app CMake
├── prj.conf                # App Kconfig (SPI + BLE + HRS + DIS)
├── sysbuild.conf           # Selects ipc_radio + bt_hci_ipc on net core
├── app.overlay             # SPIM4 @32 MHz, pinctrl P1.12/13/14, HIGH drive
├── west.yml                # Manifest: sdk-nrf v2.9.0 → path `nrf`
├── verify-acceptance.sh    # One-shot P0 acceptance harness
├── src/
│   ├── main.c              # Entry — starts BLE + SPI modules
│   ├── spi_loopback.[ch]   # SPIM4 loopback thread (DT-guarded for bsim)
│   └── ble_hrs.[ch]        # BLE Heart Rate peripheral
├── scripts/
│   ├── bsim-hrs/           # AC-V1: BabbleSim peripheral↔central runner
│   └── native-userchan/    # AC-V2: native_sim + Chrome Web Bluetooth runner
├── tools/
│   └── ble-hr-test.html    # Web Bluetooth HRS client (used by AC-V2)
├── REQUIREMENTS.md         # Detailed requirements / acceptance criteria
└── docs/
    ├── daily-logs/         # Iteration logs (day-001..011)
    └── reports/            # ROM/RAM footprint snapshots
```

---

## Acceptance criteria

Run `bash verify-acceptance.sh` to execute all P0 checks in one shot. Current
status on NCS v2.9.0:

| Group | Check | Status |
|-------|-------|--------|
| AC-1  | `west build --sysbuild` returns 0 | ✅ |
| AC-2  | `merged.hex` exists, covers app (0x00000000) + net (0x01000000) | ✅ |
| AC-4  | README contains LLM usage statement | ✅ |
| AC-5  | `.gitignore` excludes `build/`, `.west/`, `.venv/` | ✅ |
| AC-B1 | CMake configuration succeeds | ✅ |
| AC-B2 | DTS: `spi4` okay, `clock-frequency = 32 MHz`, `pinctrl-0 = &spi4_default` | ✅ |
| AC-B3 | Kconfig: `BT=y BT_PERIPHERAL=y BT_HRS=y BT_DIS=y BT_HCI_IPC=y SPI=y SPI_NRFX=y BT_DEVICE_NAME="nRF5340_HR"` | ✅ |
| AC-B4 | Sysbuild multi-image (app + ipc_radio) | ✅ |
| AC-B7 | `rm -rf build && west build` reproducibly green | ✅ |
| AC-Q1 | Zero compiler warnings with `-Werror` | ✅ |
| AC-R2 | `nrf5340bsim/nrf5340/cpuapp` compiles, `zephyr.exe` produced | ✅ (P2, optional) |
| AC-V1 | BabbleSim end-to-end: this peripheral ↔ upstream `central_hr` sees ≥ 3 `bpm` notifications | ✅ (P2, [reports/bsim-hrs-central.log](reports/bsim-hrs-central.log) — 9 notifications) |
| AC-V2 | Chrome Web Bluetooth on real 2.4 GHz radio (hci1 USB dongle) — `navigator.bluetooth` connects `nRF5340_HR`, ≥ 3 `characteristicvaluechanged` | ✅ (P2, [reports/ble-hr-connected.png](reports/ble-hr-connected.png) — 62/63/64 bpm) |

---

## Runtime verification without a DK

The email explicitly says _"no need to run on the hardware unless you have a
nRF5340 DK to hand"_. Clean build is therefore the authoritative deliverable
and is enforced by CI. However, two additional runtime paths are wired in
so a reviewer can verify the BLE code on a plain Linux laptop — neither
needs a nRF5340 DK.

### Path A — BabbleSim (pure software, no hardware at all)

Compiles the app for `nrf5340bsim/nrf5340/cpuapp` and runs it against
upstream `zephyr/samples/bluetooth/central_hr` on the virtual 2.4 GHz phy.
`reports/bsim-hrs-central.log` is the captured central-side log; 9 `bpm`
notification lines prove HRS connect + subscribe + notify work end-to-end.

Produced by the `scripts/bsim-hrs/` pipeline; see
[docs/daily-logs/day-010.md](docs/daily-logs/day-010.md).

### Path B — Chrome Web Bluetooth (real radio, any USB dongle)

Runs the same BLE code on the `native_sim` board, which opens `/dev/hci1`
via `HCI_CHANNEL_USER` (one USB BT dongle dedicated to the DUT). Chrome
talks to it via `hci0` (host's built-in adapter). The
[ble-web-bluetooth-debugger](.github/skills/ble-web-bluetooth-debugger/SKILL.md)
skill drives Patchright Chromium → [tools/ble-hr-test.html](tools/ble-hr-test.html)
→ `navigator.bluetooth.requestDevice(0x180D)` → subscribe `0x2A37` →
screenshot + log.

Reproduce (Linux, two HCI adapters, one dedicated to DUT):

```bash
# One-shot build
west build -b native_sim -d build-native --no-sysbuild \
    -- -DEXTRA_CONF_FILE=scripts/native-userchan/native.conf

# Grant the HCI_CHANNEL_USER capability (one-time; local only, not committed)
sudo setcap 'cap_net_admin,cap_net_raw+ep' build-native/zephyr/zephyr.exe

# Install browser automation
python3 -m venv .venv && source .venv/bin/activate
pip install patchright && python -m patchright install chromium

# Run DUT + launch Chromium; click Connect and pick nRF5340_HR from the chooser
bash scripts/native-userchan/run.sh hci1
```

Output: [reports/ble-hr-connected.png](reports/ble-hr-connected.png) and
[reports/ble-hr-log.txt](reports/ble-hr-log.txt) — both referenced by
`verify-acceptance.sh` as AC-V2.

---

## LLM usage statement

This project was developed with AI assistance. Per the interview brief, the
tools and how they were used are disclosed below.

| LLM / Tool | Version | Usage |
|------------|---------|-------|
| GitHub Copilot (Chat, agent mode) | Claude Opus 4.7 | Drove the full development loop: requirement breakdown, project scaffolding (`CMakeLists.txt`, `prj.conf`, `sysbuild.conf`, `app.overlay`, `west.yml`), module implementation (`spi_loopback.[ch]`, `ble_hrs.[ch]`, `main.c`), acceptance harness (`verify-acceptance.sh`), README and iteration logs. |
| GitHub Copilot (inline completion) | — | Line-level completions while editing C sources. |

Working style:

- All AI-generated code was **reviewed, fixed, and locally `west build`-verified** by the author before each commit.
- Errors surfaced by the compiler / linker / Kconfig parser (e.g. `BT_LE_ADV_CONN_FAST_1` not existing in NCS v2.9.0, `SPI_DT_SPEC_GET` needing a child slave node, the NCS hard-coded `${ZEPHYR_BASE}/../nrf` workspace path) were diagnosed and corrected by hand; see commit history and `docs/daily-logs/day-00{1,2,3}.md` for the paper trail.
- No AI-generated content is committed without human review. The assistant does not have write access to the repo; all `git` operations were performed by the author.

---

## License

Apache-2.0 for first-party code in this repository. Nordic / Zephyr components
retain their upstream licenses and are referenced — never vendored — from NCS.

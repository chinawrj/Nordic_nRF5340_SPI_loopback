# nRF5340 SPI Loopback + BLE Heart Rate

[![CI](https://github.com/chinawrj/Nordic_nRF5340_SPI_loopback/actions/workflows/build.yml/badge.svg?branch=main)](https://github.com/chinawrj/Nordic_nRF5340_SPI_loopback/actions/workflows/build.yml)

> **Status**: ✅ Clean `west build --sysbuild` passes on NCS **v2.9.0**.
> 23/23 P0 acceptance checks green. Zero compiler warnings (`-Werror`).

Interview technical task for Nordic Semiconductor: implement a 32 MHz SPIM4
loopback self-test and a BLE Heart Rate peripheral on the nRF5340 DK, using
Zephyr / nRF Connect SDK, and deliver a clean-building GitHub repository.

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
│   ├── spi_loopback.[ch]   # SPIM4 loopback thread
│   └── ble_hrs.[ch]        # BLE Heart Rate peripheral
├── REQUIREMENTS.md         # Detailed requirements / acceptance criteria
└── docs/
    ├── daily-logs/         # Iteration logs (day-001..00N)
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

# nRF5340 SPI Loopback + BLE Heart Rate

> **Status**: 🚧 Day 1 — project skeleton in place. Build verification pending
> NCS v2.9.0 toolchain installation (Day 2).

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

---

## Hardware setup

Hardware is **not required** for the interview acceptance (clean build only),
but to exercise the loopback on a real DK:

1. Short **P1.13 (MOSI)** to **P1.14 (MISO)** with a jumper wire.
2. SCK is on **P1.12**. Leave unconnected.

---

## Build

### Prerequisites

- nRF Connect SDK **v2.9.0** (installed via `nrfutil toolchain-manager` or the
  nRF Connect for VS Code extension).
- This repo checked out **inside** an NCS workspace, or referenced via a
  `west.yml` manifest (a freestanding-app layout is used).

### Commands

```bash
# Full build (app core + net core, via sysbuild)
west build -b nrf5340dk/nrf5340/cpuapp --sysbuild

# Configure-only (fast validation of DTS / Kconfig / sysbuild)
west build -b nrf5340dk/nrf5340/cpuapp --sysbuild --cmake-only

# Clean rebuild
rm -rf build && west build -b nrf5340dk/nrf5340/cpuapp --sysbuild

# ROM / RAM footprint
west build -t rom_report
west build -t ram_report

# Flash (requires hardware)
west flash
```

---

## Project layout

```
.
├── CMakeLists.txt       # Zephyr app CMake
├── prj.conf             # App Kconfig (SPI + BLE + HRS + DIS)
├── sysbuild.conf        # Selects ipc_radio + bt_hci_ipc on net core
├── app.overlay          # SPIM4 @32 MHz, pinctrl P1.12/13/14, HIGH drive
├── src/
│   ├── main.c           # Entry — starts BLE + SPI modules
│   ├── spi_loopback.[ch]# SPIM4 loopback thread
│   └── ble_hrs.[ch]     # BLE Heart Rate peripheral
├── REQUIREMENTS.md      # Detailed requirements / acceptance criteria
└── docs/daily-logs/     # Iteration logs
```

---

## LLM usage statement

This project was developed with AI assistance. Per the interview brief, the
tools and how they were used are disclosed below.

| LLM / Tool | Version | Usage |
|------------|---------|-------|
| GitHub Copilot (Chat, agent mode) | Claude Opus 4.7 | Drafted Kconfig + `app.overlay` templates, generated module skeletons (`spi_loopback.[ch]`, `ble_hrs.[ch]`), produced README and iteration logs. |
| GitHub Copilot (inline completion) | — | Line-level completions while authoring C sources. |

All generated code was **reviewed by hand** against Zephyr / NCS v2.9.0 API
documentation. Nothing in this repository is committed without a local
`west build` passing (once toolchain is in place). This README will be
updated with additional tools if any are used in later iterations.

---

## License

Apache-2.0 for first-party code in this repository. Nordic / Zephyr components
retain their upstream licenses and are referenced — never vendored — from NCS.

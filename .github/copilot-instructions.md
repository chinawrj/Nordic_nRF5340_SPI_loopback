# nRF5340 SPI Loopback + BLE HR — Copilot Instructions

## Project Overview

This is an embedded project for the **Nordic nRF5340 DK**, developed using **Zephyr RTOS / nRF Connect SDK (NCS)**.
Core functionality: SPIM4 32MHz SPI loopback test + BLE Heart Rate Service Peripheral.
Delivery criteria: clean build + GitHub repository + LLM usage disclosure.

## Tech Stack

| Item | Value |
|------|-------|
| SoC | Nordic nRF5340 (dual-core: App Core M33@128MHz + Net Core M33@64MHz) |
| RTOS | Zephyr |
| SDK | nRF Connect SDK (NCS) **v2.9.0** |
| Build tools | west, CMake, ninja |
| IDE | VS Code + nRF Connect for VS Code |
| Board | `nrf5340dk/nrf5340/cpuapp` |
| Language | C (Zephyr API) |

## Build Commands

```bash
# Full build (includes net core)
west build -b nrf5340dk/nrf5340/cpuapp --sysbuild

# CMake configuration only (quick validation)
west build -b nrf5340dk/nrf5340/cpuapp --sysbuild --cmake-only

# Incremental build
west build

# Clean rebuild
rm -rf build && west build -b nrf5340dk/nrf5340/cpuapp --sysbuild

# Flash (requires hardware)
west flash

# ROM/RAM reports
west build -t rom_report
west build -t ram_report
```

## Project Structure

```
Nordic_nRF5340_SPI_loopback/
├── CMakeLists.txt          # Top-level CMake
├── prj.conf                # Zephyr Kconfig
├── sysbuild.conf           # Sysbuild configuration (net core)
├── app.overlay             # Devicetree overlay (SPIM4 pinctrl)
├── src/
│   ├── main.c              # Entry: init SPI thread + BLE advertising
│   ├── spi_loopback.c      # SPI loopback test module
│   ├── spi_loopback.h
│   ├── ble_hrs.c           # BLE Heart Rate Service module
│   └── ble_hrs.h
├── .gitignore
├── REQUIREMENTS.md         # Requirements document
├── README.md               # Delivery notes + LLM disclosure
└── .github/
    ├── copilot-instructions.md   # Project-level AI instructions
    ├── agents/
    │   └── dev-workflow.agent.md  # Agent definition
    └── skills/                    # Agent skills
        ├── daily-iteration/
        ├── automated-testing/
        ├── code-refactoring/
        ├── tmux-multi-shell/
        └── ble-web-bluetooth-debugger/
```

## Working Rules

1. **All builds must use `west build`** — do not invoke cmake/ninja directly
2. **After DTS overlay changes, always wipe the build dir and rebuild** (DTS cache can produce false greens)
3. **After prj.conf changes, a clean rebuild is recommended** (Kconfig changes sometimes fail to trigger incremental builds)
4. **Do not modify NCS SDK source code** — configure everything through prj.conf / app.overlay / sysbuild.conf
5. **The net core is managed automatically by sysbuild** — do not build ipc_radio by hand
6. **All Python operations use `.venv/`** — the system Python is forbidden

## Key nRF5340 Facts

### Dual-core architecture
- **App Core** (M33@128MHz, 1MB Flash, 512KB RAM): runs the application code + BLE Host
- **Net Core** (M33@64MHz, 256KB Flash, 64KB RAM): runs the BLE Controller (ipc_radio)
- The two cores communicate through IPC (Inter-Processor Communication)

### SPIM4
- The only SPI instance on the nRF5340 that supports **32 MHz**
- SPIM0–3 top out at 8 MHz
- Requires HIGH drive pin configuration to guarantee signal integrity

### Sysbuild
- NCS v2.7+ uses sysbuild by default to manage multi-image builds
- `sysbuild.conf` configures the net core to use `ipc_radio` + `bt_hci_ipc`
- The build produces `merged.hex` = app core hex + net core hex combined

### BLE configuration
- `CONFIG_BT_HRS=y` enables Zephyr's built-in Heart Rate Service
- `bt_hrs_notify()` sends heart rate data
- Peripheral role: advertise → wait for connection → notify → disconnect → resume advertising

## Code Quality Requirements

- Modular: spi_loopback / ble_hrs / main kept separate
- Each `.c` file ≤ 300 lines
- Each function ≤ 50 lines
- Zero compiler warnings
- Use the Zephyr LOG subsystem (`LOG_MODULE_REGISTER`)
- Always check error codes (return value of `spi_transceive`, etc.)
- Comment language: English

## Git Conventions

```
feat: add SPI loopback test module
feat: add BLE HRS peripheral
fix: correct SPIM4 pinctrl configuration
docs: add README with build instructions and LLM statement
chore: add .gitignore and project skeleton
```

## Prohibitions

- ❌ Do not commit `build/`, `.west/`, or `.venv/` to Git
- ❌ Do not hardcode debug pin numbers in the code (use DTS definitions)
- ❌ Do not commit without running the build validation
- ❌ Do not ignore compiler warnings
- ❌ Do not perform complex operations or logging inside ISRs
- ❌ Do not fork NCS source code into the repository

## Key Reference Documents

Before starting work, read the following to understand the full requirements and plan:
- **`REQUIREMENTS.md`** (project root) — original detailed requirements
- **`.github/requirements.md`** — distilled functional requirements, acceptance criteria (P0/P1/P2), milestone definitions
- **`.github/daily-plan.md`** — daily iteration plan template, milestone progress tracking

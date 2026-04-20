# nRF5340_SPI_loopback — Project Requirements

## Overview

**Project name**: Nordic nRF5340 DK — SPI Loopback + BLE Heart Rate
**Description**: Interview technical task — implement a SPIM4 32MHz SPI loopback test + BLE Heart Rate Service Peripheral on the nRF5340 DK, built with Zephyr/NCS, and deliver a clean-build GitHub repository + README (including LLM usage disclosure)
**Target hardware**: Nordic nRF5340 DK (nrf5340dk/nrf5340/cpuapp)

## Functional Requirements

### FR-1: SPI Loopback (SPIM4 @ 32MHz)

- Use the SPIM4 instance with clock-frequency = 32 MHz
- Use the Zephyr SPI API (`spi_transceive`)
- Enable SPIM4 via a DTS overlay and assign pinctrl (SCK/MOSI/MISO, HIGH drive)
- Periodically send a test pattern (a 32-byte increasing sequence) and compare received bytes one by one
- Report results to RTT / UART via `LOG_INF` / `LOG_ERR`
- Run in a dedicated thread so BLE is not blocked

### FR-2: BLE Heart Rate Peripheral

- Enable the BLE stack (app core: host; net core: controller via ipc_radio)
- Device name `nRF5340_HR`
- Advertise with the HRS service UUID (0x180D)
- Support one Central connection (Peripheral role)
- After connection, send simulated heart rate at 1 Hz (varying 60–100 bpm)
- Automatically resume advertising after disconnection

### FR-3: Build and Delivery

- Opens and builds directly in VS Code + nRF Connect extension
- `west build -b nrf5340dk/nrf5340/cpuapp --sysbuild` succeeds without errors
- Produces `build/merged.hex` (combined app + net core)
- GitHub repo public or with an invite link
- README documents build steps, hardware wiring, and LLM usage disclosure

## Acceptance Criteria

### P0 must-pass
- [ ] `west build --sysbuild` clean build (returns 0, no errors)
- [ ] Produces `merged.hex` (contains app + net core)
- [ ] GitHub repo accessible
- [ ] README contains LLM usage disclosure
- [ ] `.gitignore` excludes build artefacts
- [ ] CMake configuration check passes
- [ ] DTS expansion confirms correct `spi4` configuration
- [ ] Kconfig confirms SPI + BLE + HRS enabled
- [ ] Sysbuild multi-image (app + ipc_radio) complete

### P1 strongly recommended
- [ ] ROM/RAM reports have no overflow and contain key symbols
- [ ] A rebuild from an empty `build/` is still a clean build
- [ ] Zero compiler warnings
- [ ] Consistent code style, clean git history

### P2 bonus
- [ ] bsim simulation compiles successfully
- [ ] Twister integration (`sample.yaml`)

## Milestones

### M1: Project skeleton + environment validation
- NCS toolchain installation verified
- Project directory structure created (CMakeLists.txt, prj.conf, sysbuild.conf, app.overlay, src/)
- `west build --cmake-only` passes

### M2: SPI Loopback implementation
- SPIM4 DTS overlay configured
- `spi_loopback.c/h` implemented
- Runs in a dedicated thread with periodic tests
- Build passes

### M3: BLE Heart Rate implementation
- BLE stack configured (prj.conf + sysbuild.conf)
- `ble_hrs.c/h` implemented
- Simulated heart-rate data notified periodically
- Build passes (including the net core ipc_radio)

### M4: Integration + acceptance
- SPI + BLE integrated into `main.c`
- Full-build validation (all AC-B checkpoints)
- ROM/RAM reports
- README + LLM disclosure
- Clean up the git history
- Publish the GitHub repository

## Tech Stack

- **SoC**: Nordic nRF5340 (dual-core: App Core M33@128MHz + Net Core M33@64MHz)
- **RTOS**: Zephyr
- **SDK**: nRF Connect SDK (NCS), latest stable release
- **Build tools**: west, CMake, ninja
- **IDE**: VS Code + nRF Connect for VS Code extension
- **Board**: `nrf5340dk/nrf5340/cpuapp`

## AI Agent Skills

The following skills will be used during development:

1. **daily-iteration** — daily iteration workflow (plan → execute → verify → review)
2. **automated-testing** — automated build validation and testing strategy
3. **code-refactoring** — periodic code refactoring to maintain quality
4. **tmux-multi-shell** — multi-terminal management (build window / serial monitor / editor)
5. **ble-web-bluetooth-debugger** — debug BLE HRS via Patchright browser + Web Bluetooth API

## Non-Functional Requirements

- Modular code (spi / ble / main separated), no compiler warnings
- Key functions are documented; README is clearly structured
- NCS version pinned (in the west manifest or the README)
- `.gitignore` excludes `build/`, `.west/`, IDE temp files
- LLM usage disclosed truthfully
- Flash/RAM usage within the nRF5340 budget

## Constraints

- Only a clean build is required; running on hardware is not required
- Use the latest official stable NCS release
- Do not fork NCS into the repo; only reference it
- App Core: 1 MB flash / 512 KB RAM
- Net Core: 256 KB flash / 64 KB RAM

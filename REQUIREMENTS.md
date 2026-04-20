# Requirements Specification

**Project**: Nordic nRF5340 DK — SPI Loopback + BLE Heart Rate
**Nature**: Interview technical task (offer-determining)
**Deliverable**: GitHub repository with a clean build + README (including LLM usage disclosure)
**Date**: 2026-04-20

---

## 1. Original Requirements (Sentence-by-Sentence Mapping)

Key points extracted from the original email:

| # | Original | Interpretation / Confirmation |
|---|----------|------------------------------|
| R1 | "Load the latest Nordic SDK and toolchain onto your PC" | Use the **latest** nRF Connect SDK (NCS). The current stable version needs to be confirmed (as of 2026-04, expected to be v2.9.x or v3.x). Install the toolchain via the nRF Connect for VS Code extension's toolchain manager. |
| R2 | "using this IDE in VS code set up a project for the nRF5340 DK" | In **VS Code**, use the **nRF Connect for VS Code** extension to create the project. Target board: `nrf5340dk/nrf5340/cpuapp`. |
| R3 | "set up a SPI loopback test for the 32 MHz capable SPI interface" | **32 MHz SPI** = must use **SPIM4** (the only SPI instance on the nRF5340 that supports 32 MHz; SPIM0–3 top out at 8 MHz). Loopback test: short MOSI to MISO, send a buffer, verify the received buffer matches. |
| R4 | "a BLE stack for communicating heart rate data over BLE from the device" | Peripheral role, GATT **Heart Rate Service (HRS, UUID 0x180D)** including the Heart Rate Measurement characteristic (0x2A37) notify. Periodically push simulated heart-rate data. |
| R5 | "provide us with that project from a GitHub repository that you can invite us to" | GitHub repository, providing an invite-capable access path (public, or private + invite). |
| R6 | "only need to create the project to a clean build level, no need to run on the hardware" | **Acceptance criterion = clean build passes** (`west build` with no errors, produces `merged.hex`). Hardware verification is not mandatory. |
| R7 | "Tell us what LLMs (including version numbers) you used in the task and how" | The README must list the LLM models/versions used and their purpose. **Honest disclosure** is itself one of the evaluation points (engineering integrity). |

**Potential implicit evaluation points (not stated but important)**:
- Understanding of the nRF5340 **dual-core architecture** (app core + network core)
- Mastery of **sysbuild** / multi-image builds
- Use of **devicetree overlays** (pinctrl + SPIM4 configuration)
- Git/GitHub engineering practices (.gitignore, commit granularity, README quality)
- Code organisation (not piling everything into `main.c`)

---

## 2. Functional Requirements (FR)

### FR-1: SPI Loopback
- **FR-1.1** Use the SPIM4 instance, configured with clock-frequency = 32 MHz
- **FR-1.2** Use the Zephyr SPI API (`<zephyr/drivers/spi.h>`, `spi_transceive`)
- **FR-1.3** Enable SPIM4 via DTS overlay and allocate pinctrl (SCK/MOSI/MISO)
- **FR-1.4** Periodically send a test pattern (e.g. a 32-byte increasing sequence) and compare received bytes one by one
- **FR-1.5** Report results through `LOG_INF` / `LOG_ERR` to RTT / UART
- **FR-1.6** Run in a dedicated thread or work queue so it does not block BLE

### FR-2: BLE Heart Rate Peripheral
- **FR-2.1** Enable the BLE stack (app core: host; net core: controller via `ipc_radio` / `hci_ipc`)
- **FR-2.2** Device name `nRF5340_HR` (configurable)
- **FR-2.3** Advertise with the HRS service UUID (0x180D)
- **FR-2.4** Support one Central connection (Peripheral role)
- **FR-2.5** After connection, periodically (1 Hz) send simulated heart rate via `bt_hrs_notify()` (e.g. varying between 60–100 bpm)
- **FR-2.6** Automatically resume advertising after disconnection

### FR-3: Build and Delivery
- **FR-3.1** The project opens and builds directly in VS Code + nRF Connect extension
- **FR-3.2** `west build -b nrf5340dk/nrf5340/cpuapp --sysbuild` succeeds with no errors
- **FR-3.3** Produces `build/merged.hex` (combined app + net core image)
- **FR-3.4** GitHub repository public or invite link provided
- **FR-3.5** README documents build steps, hardware wiring, and LLM usage disclosure

---

## 3. Non-Functional Requirements (NFR)

| ID | Item | Requirement |
|----|------|-------------|
| NFR-1 | Code quality | Modular (spi / ble / main separated), no compile warnings |
| NFR-2 | Readability | Key functions documented; README clearly structured |
| NFR-3 | Reproducibility | Pin the NCS version (west manifest or declared in README) |
| NFR-4 | Repo hygiene | `.gitignore` excludes `build/`, `.west/`, IDE temp files |
| NFR-5 | Integrity | LLM usage disclosed truthfully |

---

## 4. Constraints and Assumptions

### Constraints
- **C-1** Only a clean build is required; running on real hardware is not needed (deliverable stands without the DK)
- **C-2** Use the latest official stable NCS release (avoid the unstable `main` branch)
- **C-3** All Nordic proprietary code must follow its licence (do not fork NCS into the repo; only reference it)

### Assumptions
- **A-1** The local machine has (or can install) nRF Connect for VS Code + Toolchain Manager
- **A-2** Target board variant: `nrf5340dk/nrf5340/cpuapp` (the current NCS board naming convention)
- **A-3** Use sysbuild (default from NCS v2.7+) to automatically manage the net-core image
- **A-4** The net core uses NCS's `ipc_radio` sample image as the BLE controller

---

## 5. Technical Approach / Architecture

### 5.1 Hardware Architecture
```
 ┌────────────────────── nRF5340 SoC ──────────────────────┐
 │                                                         │
 │  App Core (M33 @128MHz)          Net Core (M33 @64MHz)  │
 │  ┌───────────────────────┐       ┌────────────────────┐ │
 │  │ main.c                │       │ ipc_radio (prebuilt│ │
 │  │ ├─ spi_loopback thread│       │   by sysbuild)     │ │
 │  │ ├─ BLE host + HRS     │◄─IPC─►│ BLE controller     │ │
 │  │ └─ logging            │       │ (HCI over IPC)     │ │
 │  └───────────────────────┘       └────────────────────┘ │
 │           │                                             │
 │           ▼                                             │
 │     SPIM4 @ 32 MHz ── MOSI ──┐                          │
 │                              ├── (wire jumper loopback) │
 │                     MISO ────┘                          │
 └─────────────────────────────────────────────────────────┘
```

### 5.2 Software Stack
- **Application layer**: `main.c` initialises, then starts the SPI thread + BLE advertising
- **SPI module**: `spi_loopback.c` — independent thread, periodic test
- **BLE module**: `ble_hrs.c` — stack init, HRS callbacks registration, periodic notify
- **Zephyr subsystems**: kernel threads, log, BT host (NCS), SPI driver (nrfx)

### 5.3 Devicetree Overlay Strategy
```
app.overlay:
  &pinctrl {
    spi4_default: spi4_default { ... SCK/MOSI/MISO pins + HIGH drive ... };
    spi4_sleep:   spi4_sleep   { ... };
  };
  &spi4 {
    status = "okay";
    pinctrl-0 = <&spi4_default>;
    pinctrl-1 = <&spi4_sleep>;
    pinctrl-names = "default", "sleep";
    clock-frequency = <32000000>;
  };
```

### 5.4 Kconfig Strategy (`prj.conf`)
```
# Logging
CONFIG_LOG=y
CONFIG_PRINTK=y

# SPI
CONFIG_SPI=y
CONFIG_SPI_NRFX=y

# BLE
CONFIG_BT=y
CONFIG_BT_PERIPHERAL=y
CONFIG_BT_DEVICE_NAME="nRF5340_HR"
CONFIG_BT_HRS=y
CONFIG_BT_DIS=y

# Threading
CONFIG_MAIN_STACK_SIZE=2048
```

`sysbuild.conf`:
```
SB_CONFIG_NETCORE_IPC_RADIO=y
SB_CONFIG_NETCORE_IPC_RADIO_BT_HCI_IPC=y
```

### 5.5 Project Directory
```
Nordic_nRF5340_SPI_loopback/
├── CMakeLists.txt
├── prj.conf
├── sysbuild.conf
├── app.overlay
├── src/
│   ├── main.c
│   ├── spi_loopback.c
│   ├── spi_loopback.h
│   ├── ble_hrs.c
│   └── ble_hrs.h
├── .gitignore
├── REQUIREMENTS.md   (this file)
└── README.md         (delivery notes + LLM disclosure)
```

---

## 6. Acceptance Criteria

### 6.0 Delivery Status Snapshot

> Refresh this table at every daily wrap-up. The **interviewer invitation gate** (see §6.3.2 + `.github/agents/dev-workflow.agent.md`) requires §6.1 + AC-V1 + AC-V2 all green before release.

| ID | Scope | Title | Priority | Status | Evidence / Command |
|----|-------|-------|----------|--------|--------------------|
| AC-1  | §6.1 | `west build --sysbuild` exits 0 | P0 | ✅ | `verify-acceptance.sh`, CI run `24658312584` |
| AC-2  | §6.1 | `merged.hex` covers app+net core | P0 | ✅ | harness AC-B4.d |
| AC-3  | §6.1 | GitHub repo + invitable | P0 | ✅ (repo live, invite gated by §6.3.2) | `chinawrj/Nordic_nRF5340_SPI_loopback` |
| AC-4  | §6.1 | README LLM disclosure | P0 | ✅ | harness AC-4 |
| AC-5  | §6.1 | `.gitignore` excludes build/west/venv | P0 | ✅ | harness AC-5.a–c |
| AC-B1 | §6.2 | `--cmake-only` succeeds | P0 | ✅ | harness implicit (AC-1 subset) |
| AC-B2 | §6.2 | DTS spi4 okay/32MHz/pinctrl | P0 | ✅ | harness AC-B2.a–c |
| AC-B3 | §6.2 | Kconfig SPI+BT+HRS+DIS+HCI_IPC | P0 | ✅ | harness AC-B3:* |
| AC-B4 | §6.2 | Sysbuild multi-image (app+net) | P0 | ✅ | harness AC-B4.a–e |
| AC-B5 | §6.2 | `rom_report` contains bt_hrs_* / spi_nrfx_* | P1 | ✅ | `docs/reports/rom-report-cpuapp.txt` |
| AC-B6 | §6.2 | `ram_report` no overflow | P1 | ✅ | `docs/reports/ram-report-cpuapp.txt` |
| AC-B7 | §6.2 | Rebuild from empty build dir still green | P1 | ✅ | harness `rm -rf build` every run |
| AC-B8 | §6.2 | `-DCONFIG_COMPILER_WARNINGS_AS_ERRORS=y` | P1 | ✅ | CI workflow step |
| AC-Q1 | §6.4 | Zero compiler warnings | P1 | ✅ | guaranteed by AC-B8 |
| AC-Q2 | §6.4 | Consistent code style | P1 | ✅ | manual review |
| AC-Q3 | §6.4 | Clean git history | P1 | ✅ | semantic commits through Day 9 |
| AC-R2 | §6.3 | `nrf5340bsim/nrf5340/cpuapp` compiles | P2 | ✅ | harness "P2: nrf5340bsim compile" |
| **AC-V1** | §6.3.2 | **bsim run: HRS peripheral↔central, notify visible** | **P2 (gate)** | ✅ Day 10 (nrf52_bsim pivot) | `reports/bsim-hrs-central.log` (9 notifications) |
| **AC-V2** | §6.3.2 | **native_sim + Chrome Web Bluetooth end-to-end HR received** | **P2 (gate)** | ✅ Day 11 (hci1 Broadcom USB + Patchright Chromium) | `reports/ble-hr-connected.png` + `reports/ble-hr-log.txt` (3 notifications, 62-64 bpm) |
| AC-R1 | §6.3 | native_sim smoke (optional) | P2 | N/A | covered by AC-V2's native_sim build |
| AC-R3 | §6.3 | bsim end-to-end BLE HRS (superset of AC-V1) | P2 | ⏳ Superseded by AC-V1 | — |
| AC-R4 | §6.3 | Twister automation | P2 | ✅ | `sample.yaml` added |

Legend: ✅ achieved / ⏳ planned / ⛔ blocked / N/A not applicable.

### 6.1 Baseline Acceptance (Explicitly Required by the Email)
- [ ] AC-1: `west build -b nrf5340dk/nrf5340/cpuapp --sysbuild` returns 0, with no errors or critical warnings
- [ ] AC-2: the build output contains `merged.hex` (combined app + net core image)
- [ ] AC-3: GitHub repo is accessible and supports collaborator invitation
- [ ] AC-4: README contains a complete LLM usage disclosure (explicitly required by the email)
- [ ] AC-5: `.gitignore` correctly excludes build artefacts

### 6.2 Hardware-free Self Test: Build-time (Static) Verification

> Goal: with only a computer and no nRF5340 DK, verify the deliverable as deeply as possible. All checks below can be completed inside the NCS workspace.

- [ ] **AC-B1 (CMake configuration check)**: `west build -b nrf5340dk/nrf5340/cpuapp --sysbuild --cmake-only` succeeds with no CMake errors
- [ ] **AC-B2 (Devicetree expansion check)**: inspect `build/nRF5340_SPI_loopback/zephyr/zephyr.dts` and confirm:
  - `spi4` node has `status = "okay"`
  - `spi4` node has `clock-frequency = <0x01e84800>` (= 32,000,000 in hex)
  - `pinctrl-0` correctly references the custom `spi4_default` node
- [ ] **AC-B3 (Kconfig effectiveness check)**: inspect `build/nRF5340_SPI_loopback/zephyr/.config` and confirm:
  - `CONFIG_SPI=y`, `CONFIG_SPI_NRFX=y`
  - `CONFIG_BT=y`, `CONFIG_BT_PERIPHERAL=y`, `CONFIG_BT_HRS=y`
  - `CONFIG_BT_DEVICE_NAME="nRF5340_HR"`
- [ ] **AC-B4 (Sysbuild multi-image check)**: under `build/` both the following exist
  - `nRF5340_SPI_loopback/` (app core subbuild)
  - `ipc_radio/` (net core subbuild — BLE controller image)
  - root `merged.hex` is larger than the app-only hex (proves it contains the net core)
- [ ] **AC-B5 (Symbols / section check)**: `west build -t rom_report` generates a ROM report containing `bt_hrs_*` and `spi_nrfx_*` related symbols
- [ ] **AC-B6 (Memory footprint)**: `west build -t ram_report` has no overflow; flash/RAM usage is within the nRF5340 budget (app core: 1MB flash / 512KB RAM; net core: 256KB flash / 64KB RAM)
- [ ] **AC-B7 (Cross-board compile smoke)**: rebuild from an empty `build/` and it still produces a clean build (rules out stale-cache false greens)
- [ ] **AC-B8 (Zero-warnings option)**: optionally re-run with `-DCONFIG_COMPILER_WARNINGS_AS_ERRORS=y` — any warning fails the build

### 6.3 Hardware-free Self Test: Runtime Simulation Verification (Bonus)

> In Zephyr the nRF5340 has a **dedicated simulation board** `nrf5340bsim/nrf5340/cpuapp` and `nrf5340bsim/nrf5340/cpunet`, which build to Linux executables on the POSIX architecture and, combined with **BabbleSim** (a physical-layer radio channel simulator), allow running the full BLE stack end to end on a plain Linux host. This is the officially recognised way to verify BLE without hardware.

- [ ] **AC-R1 (native_sim smoke, optional)**: attempt `west build -b native_sim` for the pure application logic (parts not tied to nrfx hardware registers); skip this item if SPI / BLE dependencies are too heavy
- [ ] **AC-R2 (bsim board compile)**: `west build -b nrf5340bsim/nrf5340/cpuapp` successfully produces `zephyr.exe` (a Linux ELF)
- [ ] **AC-R3 (BabbleSim running BLE HRS — highest confidence)**: with BabbleSim installed, launch a simulation and run concurrently:
  1. this project's `zephyr.exe` (DUT acting as HRS Peripheral)
  2. BabbleSim's `bs_device_handbrake` + phy simulator
  3. a simple BLE Central test script (or Zephyr's `central_hr` sample built for bsim)

  Verify: the Central scans and finds the device name `nRF5340_HR`, connects, and receives HRS notifications (heart rate values).
- [ ] **AC-R4 (Twister automation)**: add `sample.yaml` and use `twister -p nrf5340dk/nrf5340/cpuapp --build-only` to automate build regression; if bsim is added, use `--platform nrf5340bsim/nrf5340/cpuapp` for runtime regression

### 6.3.2 Runtime BLE Verification Without Hardware (added Day 9, invitation gate)

> Background: a clean build + AC-R2 compile-only prove that the configuration and compilation are correct, but do not prove that **the BLE HRS application code is correct at runtime**. An interviewer without hardware must either flash the DK themselves or only look at log screenshots. To provide reproducible runtime evidence without relying on the interviewer's own hardware, the following two **P2 runtime ACs** are added. They cover two different phys: purely virtual (bsim) and a real host adapter (HCI user-channel).
>
> **Invitation gate**: only once both `AC-V1` and `AC-V2` are green may the interviewer invitation be sent (rule captured in `.github/agents/dev-workflow.agent.md`). This is the new constraint the user made explicit on Day 9.

#### AC-V1 — bsim end-to-end HRS (pure virtual phy)

**Prerequisite**: BabbleSim installed; `BSIM_OUT_PATH` / `BSIM_COMPONENTS_PATH` exported (met on Day 7).

- [x] **AC-V1.a** Compile this project (HRS peripheral) into a bsim ELF — **board pivoted to `nrf52_bsim`** (nRF5340bsim on NCS v2.9.0 reports `Endpoint binding failed -11` at BLE IPC runtime; nrf52_bsim is single-core and stable). Build command: see `scripts/bsim-hrs/build.sh`
- [x] **AC-V1.b** Central side uses Zephyr's `tests/bsim/bluetooth/samples/central_hr_peripheral_hr` (testid `central_hr_peripheral_hr`), also built for `nrf52_bsim`, using the same `scripts/bsim-hrs/swll.overlay` + `swll.conf` (SoftDevice Controller disabled, `zephyr,bt-hci-ll-sw-split` enabled)
- [x] **AC-V1.c** `scripts/bsim-hrs/run.sh` launches `bs_2G4_phy_v1 -D=2 -sim_length=15e6`; DUT joins with `-d=0` and central joins with `-d=1 -testid=central_hr_peripheral_hr`
- [x] **AC-V1.d** `reports/bsim-hrs-central.log` actually grep-matches 9 entries of `[NOTIFICATION] data 0x... length 2` + `INFO: 9 packets received, expected >= 5` + `INFO: PASSED` (`central_hr_peripheral_hr`'s output format; length 2 = HRS flags + 8-bit BPM value)
- [x] **AC-V1.e** Log archive: `reports/bsim-hrs-central.log`, `reports/bsim-hrs-peripheral.log` (DUT shows `advertising as "nRF5340_HR"` + `connected` + `HRS notifications enabled`), `reports/bsim-hrs-phy.log`

> **Trade-off note**: nrf52_bsim only runs the App Core's side of the BLE stack (single core), so it does not reproduce the nRF5340 App↔Net IPC inside bsim. The dual-core IPC is still proved by the §6.1 hardware-target clean sysbuild (`build/ipc_radio/` + `merged.hex`). The bsim layer specifically proves that the BLE stack behaves correctly over the air; the two layers of evidence combined are equivalent to an end-to-end proof.

Verification script (approximate):
```bash
sim_id=nrf5340hrs-$(date +%s)
(cd $BSIM_OUT_PATH/bin && ./bs_2G4_phy_v1 -s=$sim_id -D=2 -sim_length=10e6) &
./build-bsim/Nordic_nRF5340_SPI_loopback/zephyr/zephyr.exe -s=$sim_id -d=0 &
./build-bsim-central/central_hr/zephyr/zephyr.exe -s=$sim_id -d=1 | tee reports/bsim-hrs-central.log
wait
grep -q 'bpm' reports/bsim-hrs-central.log
```

#### AC-V2 — native_sim + Chrome Web Bluetooth (real 2.4 GHz phy)

**Prerequisite**: two BLE adapters (system `hci0` + USB dongle `hci1`); bring the dongle up with `sudo rfkill unblock bluetooth && sudo hciconfig hci1 up`.

- [x] **AC-V2.a** Add `scripts/native-userchan/native.conf` overlay: disable SPIM4/RTT (`CONFIG_SPI_NRFX=n`, etc.); keep HRS; `BT_USERCHAN` is automatically selected by the `native_sim.dts` chosen `zephyr,bt-hci = &bt_hci_userchan` (BT_USERCHAN has no prompt and cannot be set directly)
- [x] **AC-V2.b** `west build -b native_sim -d build-native --no-sysbuild -- -DEXTRA_CONF_FILE=scripts/native-userchan/native.conf` produces `zephyr.exe` (2.4 MB)
- [x] **AC-V2.c** After `sudo setcap 'cap_net_admin,cap_net_raw+ep' zephyr.exe`, running `./zephyr.exe -bt-dev=hci1` prints `BLE stack enabled` + `advertising as "nRF5340_HR"`
- [x] **AC-V2.d** `scripts/native-userchan/ble-chrome-test.py` (Patchright headed Chromium + `tools/ble-hr-test.html`) connects to `nRF5340_HR` via hci0 and subscribes to 0x2A37 notify
- [x] **AC-V2.e** The browser `#bpm` shows 62/63/64 bpm for 3+ consecutive updates
- [x] **AC-V2.f** `reports/ble-hr-connected.png` + `reports/ble-hr-log.txt` saved

Review evidence: `reports/ble-hr-connected.png` is dropped directly into the README's verification section so the interviewer can see the end-to-end behaviour without reproducing locally.

### 6.4 Static Code Quality (Free Guarantees)

- [ ] **AC-Q1**: no compiler warnings (`-Wall -Wextra` enabled by Zephyr by default)
- [ ] **AC-Q2**: `clang-format` / Zephyr code style consistency (optionally `west format`)
- [ ] **AC-Q3**: clean git history (reasonable commit granularity, not a single dump)

### 6.5 Acceptance Priorities (When Time Is Tight)

**Must pass (P0)**: all of §6.1 + §6.2 AC-B1, AC-B2, AC-B3, AC-B4 — the minimum set for a clean build + proof of correct configuration.

**Strongly recommended (P1)**: §6.2 AC-B5, AC-B6, AC-B7, AC-B8 + all of §6.4 — demonstrates engineering rigour.

**Bonus (P2)**: §6.3 bsim end-to-end BLE simulation — without hardware, demonstrates deep familiarity with the Nordic ecosystem and scores significantly; the only cost is the extra BabbleSim setup.

---

## 6A. Hardware-free Verification Toolchain Cheat Sheet

| Tool | Purpose | Command / Usage | Source |
|------|---------|-----------------|--------|
| `west build --cmake-only` | Validate CMake/DTS/Kconfig without compiling | `west build -b <board> --cmake-only --sysbuild` | Zephyr standard |
| `zephyr.dts` inspection | Confirm DTS overlay applied | view `build/*/zephyr/zephyr.dts` | auto-generated |
| `.config` inspection | Confirm final Kconfig values | view `build/*/zephyr/.config` | auto-generated |
| `rom_report` / `ram_report` | Static footprint analysis | `west build -t rom_report` | Zephyr standard |
| **Twister** | Test runner, supports `--build-only` for regression | `twister -T . -p <board> --build-only` | Zephyr standard |
| **native_sim** | Pure POSIX application simulation (no nRF HW model) | `west build -b native_sim` | Zephyr standard |
| **nrf5340bsim** | nRF5340 HW model + POSIX simulation | `west build -b nrf5340bsim/nrf5340/cpuapp` | Zephyr/Nordic |
| **BabbleSim** | BLE/15.4 physical-layer channel simulator | Install separately (`BSIM_OUT_PATH`) | [babblesim.github.io](https://babblesim.github.io) |
| `nrfutil` / `nrfjprog` | Flashing / debugging (**not needed** for this task) | — | Nordic |

---

## 7. Risks and Mitigations

| Risk | Impact | Mitigation |
|------|--------|------------|
| NCS version and API changes (board naming became hierarchical from v2.7) | Build failure | Use the current stable release and pin the version in the README |
| SPIM4 32 MHz signal integrity requires HIGH drive pins | May not work on real hardware (but does not affect clean build) | Set `nordic,drive-mode` in pinctrl or use the HS pins the DK exposes; note in README |
| Net-core image not built correctly | No BLE | Use sysbuild + official `ipc_radio` template |
| No NCS environment locally | Cannot verify the clean build | Verify the local install first, or use NCS docker / toolchain manager |
| Dishonest LLM disclosure | Deduction at interview | Honestly list Copilot / Claude etc. and their specific usage |

---

## 8. Items Pending Confirmation / Decisions (please review)

1. **NCS version**: plan to use the latest stable release (to be confirmed via `west` / the official page before implementation). Accept?
2. **Board variant**: `nrf5340dk/nrf5340/cpuapp` (hierarchical, NCS v2.7+). OK?
3. **SPI pin selection**: need to pick two physically adjacent pins on the DK, easy to jumper-short for MOSI/MISO (noted in the overlay). OK to let me decide?
4. **Heart-rate data**: use simulated data (60–100 bpm ramp / sine). OK, or should a specific sensor be integrated?
5. **GitHub repo**: use the name `Nordic_nRF5340_SPI_loopback` (matches the workspace), public repo. OK?
6. **LLM disclosure format**: plan to list "GitHub Copilot (Claude Opus 4.7) — used for generating Kconfig templates, DTS overlay drafts, README prose; all code was human-reviewed and validated by a local clean build". Accept?
7. **Local NCS install status**: need to confirm whether `~/ncs` already has the toolchain installed. Want me to check now?
8. **Enable bsim bonus (§6.3)?** Extra BabbleSim install, to end-to-end verify BLE HRS without hardware. Cost: ~30 minutes of one-off setup. Benefit: significant interview bonus (demonstrates Nordic-ecosystem depth). Recommendation: **YES**, agree?
9. **Add Twister integration (§6.2 AC-B automation)?** Add `sample.yaml` so `twister --build-only` can regress the build automatically. Cost: a dozen lines of YAML. Benefit: demonstrates engineering mindset. Recommendation: **YES**, agree?

---

## 9. Next Steps

Once you confirm the decisions in §8, proceed with implementation:
1. Check / install the NCS toolchain
2. Generate the project skeleton (CMakeLists / prj.conf / sysbuild.conf / app.overlay / src/*)
3. Validate a clean build locally with `west build`
4. `git init` + `.gitignore` + initial commit
5. Push to GitHub and prepare the invitation
6. Finalise the README (including LLM disclosure)

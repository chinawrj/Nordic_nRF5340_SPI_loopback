# Linux Migration Handoff Notes

> For **Day 5 and later**, where I continue this project on a Linux machine.
> Goal: reproduce all Day 1–4 P0+P1 acceptance results from scratch within 30 minutes, then focus on Day 5's P2 tasks.

---

## 1. Current Delivery Status (end of macOS Day 4)

| Dimension | Status |
|-----------|--------|
| NCS version | v2.9.0 (pinned) |
| Board | `nrf5340dk/nrf5340/cpuapp` |
| `west build --sysbuild` | ✅ returns 0 |
| `merged.hex` | ✅ 347 KB, covering app@0x0 + net@0x01000000 |
| `-Werror` strict build | ✅ 0 warnings / 0 errors |
| `verify-acceptance.sh` | ✅ 23/23 PASS |
| App core usage | FLASH 12.1%, RAM 7.2% |
| Net core (ipc_radio) | FLASH 64.4%, RAM 71.0% |
| GitHub repo | `chinawrj/Nordic_nRF5340_SPI_loopback` (pushed to `origin/main`) |
| Interviewer invitation | ⏳ pending — only after Linux verification + P2 bonuses are complete |

Latest commit: `feat: Linux cross-platform regression — 23/23 PASS on Ubuntu 24.04` (`2ed96fa`, Day 5)
— Day 5 completed the first cross-platform verification on Ubuntu 24.04; Day 6 updates this doc to consolidate Linux-specific gotchas.

---

## 2. Linux Bootstrap Checklist (30–45 min, verified on Ubuntu 22.04 / 24.04)

> ⚠️ **Ubuntu 24.04 (noble) caveats**: `libmagic1` has been renamed to `libmagic1t64` (apt resolves this automatically); `gcc-multilib` / `g++-multilib` frequently 404 on the security mirror (install failure is safe to ignore — only bsim/native_sim need `-m32`, the ARM target does not).

```bash
# [1] Install system prerequisites
sudo apt-get update && sudo apt-get install -y --no-install-recommends \
    git cmake ninja-build gperf ccache dfu-util device-tree-compiler \
    wget curl python3-pip python3-venv python3-dev xz-utils file make \
    libsdl2-dev
# Optional (only needed for bsim/native_sim):
#   sudo apt-get install -y gcc-multilib g++-multilib

# [2] Install nrfutil (URL path changed after 2025 — the old linux-amd64 path returns 404)
mkdir -p ~/.local/bin && curl -fsSL -o ~/.local/bin/nrfutil \
    https://files.nordicsemi.com/artifactory/swtools/external/nrfutil/executables/x86_64-unknown-linux-gnu/nrfutil
chmod +x ~/.local/bin/nrfutil
export PATH="$HOME/.local/bin:$PATH"
nrfutil --version    # expected: nrfutil 8.x

# [3] Install the NCS v2.9.0 toolchain (~5 GB, 5-10 min)
nrfutil install toolchain-manager
nrfutil toolchain-manager install --ncs-version v2.9.0
nrfutil toolchain-manager list    # should show * v2.9.0

# [4] Clone and initialise the workspace
#     Convention: the app repo inside the workspace topdir is called `Nordic_nRF5340_SPI_loopback`;
#     nrf/zephyr/nrfxlib/... land as its siblings
mkdir -p ~/ncs-ws && cd ~/ncs-ws
git clone git@github.com:chinawrj/Nordic_nRF5340_SPI_loopback.git

# [5] Run every subsequent west command inside the nrfutil launch sandbox (see §4 gotcha 2)
alias nw='nrfutil toolchain-manager launch --ncs-version v2.9.0 --shell --'
nw west init -l Nordic_nRF5340_SPI_loopback
nw west update --narrow -o=--depth=1    # 5-20 min; interrupts can simply be re-run to resume
nw west zephyr-export

# [6] Build + verify
cd Nordic_nRF5340_SPI_loopback
nw west build -b nrf5340dk/nrf5340/cpuapp --sysbuild
nw bash verify-acceptance.sh            # expected: 23/23 PASS
```

---

## 3. Recommended P2 Task Path for Day 5+

### 3.1 `nrf5340bsim` Simulation (AC-R2/R3 — high-value bonus)

Zephyr officially provides the `nrf5340bsim/nrf5340/cpuapp` target — built against the POSIX architecture as a Linux ELF, and combined with BabbleSim it can exercise the BLE HRS stack end to end **without hardware**.

```bash
# AC-R2 smoke: compile
cd ~/ncs-ws/Nordic_nRF5340_SPI_loopback
west build -b nrf5340bsim/nrf5340/cpuapp --sysbuild --build-dir build-bsim
ls build-bsim/Nordic_nRF5340_SPI_loopback/zephyr/zephyr.exe

# AC-R3 run: install BabbleSim first
git clone https://github.com/BabbleSim/babble_sim.git ~/bsim && cd ~/bsim
make everything
export BSIM_OUT_PATH=~/bsim
# Then compile a bsim version of Zephyr's `central_hr` sample as the peer
# For specific scripts see zephyr/tests/bsim/bluetooth/
```

**Caveats**:
- SPIM4 has no hardware model under bsim — the application's SPI loopback init will fail (`device_is_ready` returns false). It does not affect the BLE side. Guard it with conditional compilation (`#if !defined(CONFIG_SOC_POSIX)`), or let the SPI thread exit gracefully.
- The full BLE stack is available — Zephyr's bsim HCI driver + BabbleSim's physical layer are enough to drive real HRS notifications.

### 3.2 Twister Integration (AC-R4)

`sample.yaml` is already in place, so on Linux you can simply:
```bash
cd ~/ncs-ws
zephyr/scripts/twister -T Nordic_nRF5340_SPI_loopback \
    -p nrf5340dk/nrf5340/cpuapp --build-only
# Expected: PASSED
```

### 3.3 GitHub Actions

`.github/workflows/build.yml` is in place; the next push will automatically run sysbuild + `-Werror` + `verify-acceptance.sh` on the Ubuntu runner. **First thing on Day 5: look at the Actions tab in the GitHub web UI and check whether it is green**. If red → fix the error, since the local Linux reproduction and the CI reproduction follow the exact same path.

---

## 4. Cross-Platform Differences Quick Reference

| Item | macOS (Day 1–4) | Linux / Ubuntu 24.04 (Day 5+) |
|------|-----------------|-------------------------------|
| nrfutil install | `brew install nrfutil` | `curl ...x86_64-unknown-linux-gnu/nrfutil` (**the old `linux-amd64` path is 404**) |
| Toolchain install path | `/opt/nordic/ncs/toolchains/<hash>/` | `~/ncs/toolchains/<hash>/` (~5 GB) |
| `nrfutil env --as-script` + source | On zsh, must write to a file and source it (`eval` enters `dquote>`) | **Not recommended to source directly**: `LD_LIBRARY_PATH` pollutes the system git/cmake (see gotcha 2); prefer the `nrfutil toolchain-manager launch --shell --` sandbox |
| bsim support | ❌ none (BabbleSim is Linux-only) | ✅ (requires a separate BabbleSim install) |
| `native_sim` target | Limited (nrf5340 model incomplete) | Recommended path |
| apt package names (Ubuntu 24.04 vs 22.04) | — | `libmagic1` → `libmagic1t64`; `gcc-multilib` often 404 (can be skipped) |

---

## 5. Known Gotchas (Day 1–5 Cumulative Summary)

All are fixed and committed, but if anything regresses on Linux, check these first:

### Day 1–4 (application layer and Zephyr API)

1. **The sdk-nrf path must be named `nrf`** — `path: nrf` in `west.yml`; otherwise NCS's hardcoded `${ZEPHYR_BASE}/../nrf/...` breaks everything.
2. **`BT_LE_ADV_CONN_FAST_1` is a Zephyr 4.x macro** — NCS v2.9.0 = Zephyr v3.7.99-ncs2, use `BT_LE_ADV_CONN`.
3. **`SPI_DT_SPEC_GET` requires a child slave node** — for loopback scenarios without a slave, use `DEVICE_DT_GET` + a hand-crafted `spi_config`.
4. **`.cs = {0}` triggers `-Werror=missing-braces`** — simply omit it; C99 zero-initialises.
5. **`CONFIG_BT_HCI_IPC` is not user-configurable** — the board defconfig auto-selects it through `chosen zephyr,bt-hci`; do not hardcode in `prj.conf`.
6. **`rom_report` / `ram_report` must be per-image** — running from the sysbuild root dir reports an unknown target; use `-d build/Nordic_nRF5340_SPI_loopback -t rom_report`.

### Day 5 (Linux / Ubuntu 24.04 environment)

7. **The nrfutil download URL path changed** — the old `.../executables/linux-amd64/nrfutil` returns 404; the new path is `.../executables/x86_64-unknown-linux-gnu/nrfutil`. macOS equivalently moves to `aarch64-apple-darwin` / `x86_64-apple-darwin`.
8. **`LD_LIBRARY_PATH` pollutes the system git/cmake (Ubuntu 24.04)** — after `source /tmp/ncs_env.sh`, NCS's libcurl links against `libunistring.so.2` while noble only provides `libunistring.so.5` → `git-remote-https` fails to start; likewise the system cmake exits 127 under the NCS `LD_LIBRARY_PATH`. **Idiom**: use `nrfutil toolchain-manager launch --ncs-version v2.9.0 --shell -- <cmd>` sandbox (auto-isolates env + loads dependencies correctly). If you insist on doing it manually: `unset LD_LIBRARY_PATH GIT_EXEC_PATH GIT_TEMPLATE_DIR` before running git.
9. **Per-board overlay convention** — a top-level `app.overlay` applies to every board; under bsim/native_sim (no `&spi4` label) DTS parsing fails. Hardware-specific nodes (spi/uart/gpio) belong in `boards/<board>.overlay` (e.g. `boards/nrf5340dk_nrf5340_cpuapp.overlay`), which Zephyr applies only when the board matches.
10. **The bsim board requires the BabbleSim environment** — installing the NCS toolchain alone is **not enough** to run `west build -b nrf5340bsim/...`; you must also `git clone babble_sim.github.io && make everything` and `export BSIM_OUT_PATH` + `BSIM_COMPONENTS_PATH`. Without this, AC-R2 smoke CMake fails at `find_package(bsim)`.

---

## 6. Interviewer Invitation Checklist (end of Day 5 / Day 6)

Only do this after verifying CI + bsim on Linux:

- [ ] GitHub Actions is green (latest commit on main)
- [ ] Local `bash verify-acceptance.sh` reports 23/23
- [ ] bsim/native_sim reproducible on Linux (at least AC-R2 compiles)
- [ ] README top-of-file status line updated with the CI badge
- [ ] `gh repo edit --add-topic nrf5340 --add-topic zephyr --add-topic ble`
- [ ] Settings → Collaborators → Add interviewer email
- [ ] Send the interviewer a confirmation email with the repo URL, noting "clean build reproduces in a single `west build`, `bash verify-acceptance.sh` runs 23 assertions"

---

## 7. Quick Reference Links

- **REQUIREMENTS.md** — original requirements + full acceptance criteria table
- **docs/daily-logs/** — daily progress and gotchas
- **docs/skill-feedback.md** — workflow feedback (if any)
- **verify-acceptance.sh** — single source of truth for P0

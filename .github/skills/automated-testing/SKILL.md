---
name: "automated-testing"
description: "Automated build validation and acceptance testing for the nRF5340 project — based on west build + static analysis"
---

# Skill: Automated Testing (Zephyr/NCS edition)

## Purpose

Defines the automated build-validation and acceptance-testing strategy for the nRF5340 Zephyr/NCS project.
This project does not require hardware; the primary verification approach is **build success + static analysis**.

**When to use:**
- After every code change, to validate a clean build
- At milestone completion, to run the full acceptance check
- Before release, as the final validation
- As a regression safety net against configuration drift

**When not to use:**
- Pure documentation changes (no build validation needed)
- During SDK install / environment setup

## Prerequisites

- nRF Connect SDK (NCS) installed and available via `west`
- `west init` / `west update` completed
- The project directory contains `CMakeLists.txt`, `prj.conf`, `app.overlay`

## Procedure

### 1. Test pyramid (hardware-free)

```
        ┌─────────────┐
        │ Acceptance   │  ← verify-acceptance.sh: full P0 suite
        │ Tests        │
       ┌┴─────────────┴┐
       │ Static Analysis│  ← DTS/Kconfig/ROM/RAM verification
       │ Tests          │
      ┌┴────────────────┴┐
      │  Build Tests      │  ← west build --sysbuild must succeed
      └──────────────────┘
```

### 2. Build tests (after every change)

```bash
# Incremental build (fast validation)
west build

# Full build (after config changes)
rm -rf build && west build -b nrf5340dk/nrf5340/cpuapp --sysbuild

# CMake-only (config validation only)
west build -b nrf5340dk/nrf5340/cpuapp --sysbuild --cmake-only
```

### 3. Static-analysis tests

```bash
# DTS expansion check — confirm the SPIM4 configuration is correct
BUILD_APP_DIR=$(find build -name "zephyr.dts" -path "*/zephyr/*" ! -path "*/ipc_radio/*" | head -1 | xargs dirname)
cat "$BUILD_APP_DIR/zephyr.dts" | grep -A10 "spi4"
# Expect: status = "okay", clock-frequency = <32000000>

# Kconfig check — confirm key options are enabled
grep -E "CONFIG_(SPI|BT|BT_PERIPHERAL|BT_HRS)=" \
  "$BUILD_APP_DIR/.config"
# Expect: all =y

# Sysbuild multi-image check
ls -la build/ipc_radio/zephyr/zephyr.hex
ls -la build/merged.hex
# Expect: both files exist

# ROM/RAM reports
west build -t rom_report 2>&1 | tail -20
west build -t ram_report 2>&1 | tail -20
# Expect: no overflow

# Symbol check
west build -t rom_report 2>&1 | grep -E "bt_hrs|spi_nrfx"
# Expect: contains bt_hrs_* and spi_nrfx_* symbols
```

### 4. Strict compile test

```bash
# Warnings as errors
west build -b nrf5340dk/nrf5340/cpuapp --sysbuild \
  -- -DCONFIG_COMPILER_WARNINGS_AS_ERRORS=y

# Clean-rebuild validation (rules out cache-driven false greens)
rm -rf build && west build -b nrf5340dk/nrf5340/cpuapp --sysbuild
```

### 5. One-shot acceptance script (verify-acceptance.sh)

Save the following script as `verify-acceptance.sh` at the project root:

```bash
#!/bin/bash
# verify-acceptance.sh — automated P0 acceptance for the nRF5340 SPI+BLE project
set -euo pipefail

PASS=0; FAIL=0; TOTAL=0
BOARD="nrf5340dk/nrf5340/cpuapp"
BUILD_DIR="build"
APP_BUILD="$BUILD_DIR/nRF5340_SPI_loopback"

check() {
    local id="$1" desc="$2"
    shift 2
    TOTAL=$((TOTAL + 1))
    if "$@" >/dev/null 2>&1; then
        echo "  ✅ PASS: $id — $desc"
        PASS=$((PASS + 1))
    else
        echo "  ❌ FAIL: $id — $desc"
        FAIL=$((FAIL + 1))
    fi
}

echo "============================================"
echo " nRF5340 SPI+BLE P0 acceptance"
echo "============================================"
echo ""

# --- Build tests ---
echo "[1/4] clean rebuild..."
rm -rf "$BUILD_DIR"
west build -b "$BOARD" --sysbuild 2>&1 | tail -5
BUILD_RC=${PIPESTATUS[0]:-$?}

echo ""
echo "[2/4] build result checks..."
check "AC-1" "west build --sysbuild returns 0" test "$BUILD_RC" -eq 0
check "AC-2" "merged.hex exists" test -f "$BUILD_DIR/merged.hex"
check "AC-B1" "CMake config succeeded" test -f "$APP_BUILD/zephyr/.config"
check "AC-B4" "ipc_radio net core present" test -d "$BUILD_DIR/ipc_radio"

echo ""
echo "[3/4] static analysis..."

# DTS check
check "AC-B2" "spi4 node status=okay" \
    grep -q 'status = "okay"' <(grep -A5 "spi4 {" "$APP_BUILD/zephyr/zephyr.dts" 2>/dev/null || true)

# Kconfig checks
for cfg in CONFIG_SPI CONFIG_BT CONFIG_BT_PERIPHERAL CONFIG_BT_HRS; do
    check "AC-B3:$cfg" "$cfg=y" grep -q "^${cfg}=y" "$APP_BUILD/zephyr/.config"
done

echo ""
echo "[4/4] repo checks..."
check "AC-5" ".gitignore excludes build/" grep -q "^build/" .gitignore
check "AC-4" "README contains LLM disclosure" grep -qi "llm\|language model\|copilot\|chatgpt\|claude" README.md

echo ""
echo "============================================"
echo " Result: $PASS/$TOTAL PASS, $FAIL FAIL"
echo "============================================"

if [ "$FAIL" -gt 0 ]; then
    echo "❌ Acceptance failed — please fix the FAIL items above"
    exit 1
else
    echo "✅ All P0 acceptance criteria passed!"
    exit 0
fi
```

### 6. Post-release verification (Fresh Clone Test)

After publishing to GitHub, verify on another machine or in a clean directory:

```bash
# 1. Clone
git clone https://github.com/<user>/Nordic_nRF5340_SPI_loopback.git /tmp/nrf-verify
cd /tmp/nrf-verify

# 2. File integrity
test -f CMakeLists.txt && test -f prj.conf && test -f app.overlay && \
test -d src && test -f README.md && echo "✅ files present" || echo "❌ files missing"

# 3. Files that should not exist
test ! -d build && test ! -d .west && test ! -d .venv && \
echo "✅ no build artefacts" || echo "❌ build artefacts were committed"

# 4. Build validation (requires the NCS env)
west build -b nrf5340dk/nrf5340/cpuapp --sysbuild && \
echo "✅ fresh clone build succeeded" || echo "❌ fresh clone build failed"

# 5. Run the acceptance script
bash verify-acceptance.sh

# 6. Cleanup
rm -rf /tmp/nrf-verify
```

## Self-Test

> Validate the core logic of the testing framework (no NCS environment required).

### Self-test steps

```bash
# Test 1: check() function logic
bash -c '
PASS=0; FAIL=0; TOTAL=0
check() {
    local id="$1" desc="$2"; shift 2; TOTAL=$((TOTAL+1))
    if "$@" >/dev/null 2>&1; then PASS=$((PASS+1)); else FAIL=$((FAIL+1)); fi
}
check "T1" "true passes" true
check "T2" "false fails" false
check "T3" "test works" test 1 -eq 1

if [ "$PASS" -eq 2 ] && [ "$FAIL" -eq 1 ] && [ "$TOTAL" -eq 3 ]; then
    echo "SELF_TEST_PASS: check_function"
else
    echo "SELF_TEST_FAIL: check_function (P=$PASS F=$FAIL T=$TOTAL)"
fi
'

# Test 2: grep pattern validation
bash -c '
echo "CONFIG_SPI=y" > /tmp/test_config
echo "CONFIG_BT=y" >> /tmp/test_config
echo "CONFIG_BT_PERIPHERAL=y" >> /tmp/test_config
echo "CONFIG_I2C=n" >> /tmp/test_config

OK=0
for cfg in CONFIG_SPI CONFIG_BT CONFIG_BT_PERIPHERAL; do
    grep -q "^${cfg}=y" /tmp/test_config && OK=$((OK+1))
done
! grep -q "^CONFIG_BT_HRS=y" /tmp/test_config && OK=$((OK+1))

rm /tmp/test_config
if [ "$OK" -eq 4 ]; then
    echo "SELF_TEST_PASS: kconfig_grep"
else
    echo "SELF_TEST_FAIL: kconfig_grep ($OK/4)"
fi
'

# Test 3: .gitignore check logic
bash -c '
echo -e "build/\n.west/\n.venv/" > /tmp/test_gitignore
grep -q "^build/" /tmp/test_gitignore && \
grep -q "^\.west/" /tmp/test_gitignore && \
echo "SELF_TEST_PASS: gitignore_check" || echo "SELF_TEST_FAIL: gitignore_check"
rm /tmp/test_gitignore
'
```

### Blind Test

**Test prompt:**
```
You are an AI development assistant. Read this skill, then:
1. Write a simplified verify-acceptance.sh that only checks 3 things: build succeeded, merged.hex exists, .gitignore is correct
2. Make the script print a PASS/FAIL summary and return the correct exit code
3. Explain why a "clean rebuild" is more reliable than an "incremental build"
4. Describe the purpose of the fresh clone test
```

**Acceptance criteria:**
- [ ] The agent uses a `check()` function structure
- [ ] The script exits 1 on FAIL
- [ ] The agent explains the false-green problem caused by DTS/Kconfig caching
- [ ] The agent understands that a fresh clone test validates `.gitignore` and repo integrity

## Success Criteria

- [ ] `west build --sysbuild` passes after every change
- [ ] All P0 acceptance criteria pass (`verify-acceptance.sh` exits 0)
- [ ] A clean rebuild still succeeds
- [ ] Post-release fresh clone build succeeds

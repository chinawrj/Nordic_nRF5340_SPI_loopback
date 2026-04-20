---
name: "automated-testing"
description: "nRF5340 项目自动化构建验证与验收测试 — 基于 west build + 静态分析"
---

# Skill: 自动化测试（Zephyr/NCS 版）

## 用途

定义 nRF5340 Zephyr/NCS 项目的自动化构建验证与验收测试策略。
本项目不要求硬件，核心验证手段是 **构建成功 + 静态分析**。

**何时使用：**
- 每次代码修改后验证 clean build
- 里程碑完成时运行完整验收检查
- 发布前进行最终验证
- 回归测试防止配置退化

**何时不使用：**
- 单纯的文档修改（无需构建验证）
- SDK 安装/环境配置阶段

## 前置条件

- nRF Connect SDK (NCS) 已安装并通过 `west` 可用
- `west init` / `west update` 已完成
- 项目目录包含 `CMakeLists.txt`, `prj.conf`, `app.overlay`

## 操作步骤

### 1. 测试金字塔（无硬件版）

```
        ┌─────────────┐
        │ Acceptance   │  ← verify-acceptance.sh: 全量 P0 检查
        │ Tests        │
       ┌┴─────────────┴┐
       │ Static Analysis│  ← DTS/Kconfig/ROM/RAM 验证
       │ Tests          │
      ┌┴────────────────┴┐
      │  Build Tests      │  ← west build --sysbuild 编译通过
      └──────────────────┘
```

### 2. 构建测试（每次修改后）

```bash
# 增量构建（快速验证）
west build

# 完整构建（配置变更后）
rm -rf build && west build -b nrf5340dk/nrf5340/cpuapp --sysbuild

# CMake-only（仅验证配置）
west build -b nrf5340dk/nrf5340/cpuapp --sysbuild --cmake-only
```

### 3. 静态分析测试

```bash
# DTS 展开验证 — 确认 SPIM4 配置正确
BUILD_APP_DIR=$(find build -name "zephyr.dts" -path "*/zephyr/*" ! -path "*/ipc_radio/*" | head -1 | xargs dirname)
cat "$BUILD_APP_DIR/zephyr.dts" | grep -A10 "spi4"
# 预期: status = "okay", clock-frequency = <32000000>

# Kconfig 验证 — 确认关键配置已启用
grep -E "CONFIG_(SPI|BT|BT_PERIPHERAL|BT_HRS)=" \
  "$BUILD_APP_DIR/.config"
# 预期: 全部 =y

# Sysbuild 多镜像验证
ls -la build/ipc_radio/zephyr/zephyr.hex
ls -la build/merged.hex
# 预期: 两个文件均存在

# ROM/RAM 报告
west build -t rom_report 2>&1 | tail -20
west build -t ram_report 2>&1 | tail -20
# 预期: 无 overflow

# 符号检查
west build -t rom_report 2>&1 | grep -E "bt_hrs|spi_nrfx"
# 预期: 包含 bt_hrs_* 和 spi_nrfx_* 符号
```

### 4. 严格编译测试

```bash
# 警告作为错误
west build -b nrf5340dk/nrf5340/cpuapp --sysbuild \
  -- -DCONFIG_COMPILER_WARNINGS_AS_ERRORS=y

# 清空重建验证（排除缓存假成功）
rm -rf build && west build -b nrf5340dk/nrf5340/cpuapp --sysbuild
```

### 5. 一键验收脚本（verify-acceptance.sh）

将以下脚本保存为项目根目录的 `verify-acceptance.sh`：

```bash
#!/bin/bash
# verify-acceptance.sh — nRF5340 SPI+BLE 项目 P0 验收自动化
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
echo " nRF5340 SPI+BLE P0 验收测试"
echo "============================================"
echo ""

# --- 构建测试 ---
echo "[1/4] 清空重建..."
rm -rf "$BUILD_DIR"
west build -b "$BOARD" --sysbuild 2>&1 | tail -5
BUILD_RC=${PIPESTATUS[0]:-$?}

echo ""
echo "[2/4] 构建结果检查..."
check "AC-1" "west build --sysbuild 返回 0" test "$BUILD_RC" -eq 0
check "AC-2" "merged.hex 存在" test -f "$BUILD_DIR/merged.hex"
check "AC-B1" "CMake 配置成功" test -f "$APP_BUILD/zephyr/.config"
check "AC-B4" "ipc_radio net core 存在" test -d "$BUILD_DIR/ipc_radio"

echo ""
echo "[3/4] 静态分析..."

# DTS 检查
check "AC-B2" "spi4 节点 status=okay" \
    grep -q 'status = "okay"' <(grep -A5 "spi4 {" "$APP_BUILD/zephyr/zephyr.dts" 2>/dev/null || true)

# Kconfig 检查
for cfg in CONFIG_SPI CONFIG_BT CONFIG_BT_PERIPHERAL CONFIG_BT_HRS; do
    check "AC-B3:$cfg" "$cfg=y" grep -q "^${cfg}=y" "$APP_BUILD/zephyr/.config"
done

echo ""
echo "[4/4] 仓库检查..."
check "AC-5" ".gitignore 排除 build/" grep -q "^build/" .gitignore
check "AC-4" "README 含 LLM 声明" grep -qi "llm\|language model\|copilot\|chatgpt\|claude" README.md

echo ""
echo "============================================"
echo " 结果: $PASS/$TOTAL PASS, $FAIL FAIL"
echo "============================================"

if [ "$FAIL" -gt 0 ]; then
    echo "❌ 验收未通过 — 请修复上述 FAIL 项"
    exit 1
else
    echo "✅ 所有 P0 验收标准通过！"
    exit 0
fi
```

### 6. 发布后验证（Fresh Clone Test）

发布到 GitHub 后，用另一台机器或干净目录验证：

```bash
# 1. 克隆
git clone https://github.com/<user>/Nordic_nRF5340_SPI_loopback.git /tmp/nrf-verify
cd /tmp/nrf-verify

# 2. 检查文件完整性
test -f CMakeLists.txt && test -f prj.conf && test -f app.overlay && \
test -d src && test -f README.md && echo "✅ 文件完整" || echo "❌ 文件缺失"

# 3. 不应存在的文件
test ! -d build && test ! -d .west && test ! -d .venv && \
echo "✅ 无构建产物" || echo "❌ 构建产物被提交"

# 4. 构建验证（需 NCS 环境）
west build -b nrf5340dk/nrf5340/cpuapp --sysbuild && \
echo "✅ Fresh clone build 成功" || echo "❌ Fresh clone build 失败"

# 5. 运行验收脚本
bash verify-acceptance.sh

# 6. 清理
rm -rf /tmp/nrf-verify
```

## Self-Test（自检）

> 验证测试框架的核心逻辑（不需要 NCS 环境）。

### 自检步骤

```bash
# Test 1: check() 函数逻辑验证
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

# Test 2: grep 模式验证
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

# Test 3: .gitignore 检查逻辑
bash -c '
echo -e "build/\n.west/\n.venv/" > /tmp/test_gitignore
grep -q "^build/" /tmp/test_gitignore && \
grep -q "^\.west/" /tmp/test_gitignore && \
echo "SELF_TEST_PASS: gitignore_check" || echo "SELF_TEST_FAIL: gitignore_check"
rm /tmp/test_gitignore
'
```

### Blind Test（盲测）

**测试 Prompt:**
```
你是一个 AI 开发助手。请阅读此 Skill，然后：
1. 编写一个简化版的 verify-acceptance.sh，只检查 3 项：build 成功、merged.hex 存在、.gitignore 正确
2. 使脚本输出 PASS/FAIL 汇总并返回正确的 exit code
3. 解释为什么"清空重建"比"增量构建"更可靠
4. 描述 fresh clone test 的目的
```

**验收标准:**
- [ ] Agent 使用了 check() 函数结构
- [ ] 脚本在 FAIL 时 exit 1
- [ ] Agent 解释了 DTS/Kconfig 缓存导致假成功的问题
- [ ] Agent 理解 fresh clone test 验证 .gitignore 和仓库完整性

## 成功标准

- [ ] `west build --sysbuild` 每次修改后通过
- [ ] 所有 P0 验收标准通过（verify-acceptance.sh exit 0）
- [ ] 清空重建仍然成功
- [ ] 发布后 fresh clone build 成功
- [ ] 测试结果有清晰的 PASS/FAIL 输出

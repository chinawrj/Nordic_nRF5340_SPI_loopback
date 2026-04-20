---
description: "nRF5340_SPI_loopback 开发工作流 Agent - 驱动迭代开发，确保 clean build 交付"
name: "dev-workflow"
---

# nRF5340_SPI_loopback 开发工作流 Agent

你是 **nRF5340_SPI_loopback** 项目的开发工作流 Agent。你的职责是驱动项目的每日迭代开发，确保项目按计划推进并最终完成。

## 项目信息

- **项目名称**: Nordic nRF5340 DK — SPI Loopback + BLE Heart Rate
- **项目描述**: 在 nRF5340 DK 上实现 SPIM4 32MHz SPI Loopback 测试 + BLE Heart Rate Service Peripheral，使用 Zephyr/NCS 构建，交付 clean build 的 GitHub 仓库
- **目标硬件**: Nordic nRF5340 DK (nrf5340dk/nrf5340/cpuapp)
- **构建系统**: Zephyr RTOS / nRF Connect SDK (NCS) / west
- **NCS 版本**: v2.9.0（固定版本，确保可复现构建）

## 关键参考文档

以下文档位于 `.github/` 目录下，包含完整的需求和计划信息：
- **`.github/requirements.md`** — 功能需求 (FR)、非功能需求 (NFR)、验收标准 (P0/P1/P2)、里程碑 (M1-M4)
- **`.github/daily-plan.md`** — 每日迭代计划模板、里程碑跟踪、验收标准进度

## 可用 Skills

以下 skills 已安装在 `.github/skills/` 目录中：

1. **daily-iteration** — 每日迭代工作流（计划→执行→回顾）
2. **automated-testing** — 自动化测试策略（构建验证、串口测试）
3. **code-refactoring** — 周期性代码重构策略
4. **tmux-multi-shell** — 多终端管理（编译/烧录/串口监控）
5. **ble-web-bluetooth-debugger** — 通过 Patchright 浏览器 + Web Bluetooth API 调试 BLE 设备（连接 HRS、读取心率 notify）

## 验收标准

### P0 必过（邮件明确要求 + 配置正确性最小集）

- [ ] AC-1: `west build -b nrf5340dk/nrf5340/cpuapp --sysbuild` 返回 0，无 error
- [ ] AC-2: build 输出包含 `merged.hex`（app + net core 合并镜像）
- [ ] AC-3: GitHub 仓库可访问并能邀请协作者
- [ ] AC-4: README 中包含完整 LLM 使用声明
- [ ] AC-5: `.gitignore` 正确排除 build 产物
- [ ] AC-B1: `west build --cmake-only` 成功无 CMake error
- [ ] AC-B2: `zephyr.dts` 确认 spi4 节点 status="okay", clock-frequency=32MHz
- [ ] AC-B3: `.config` 确认 CONFIG_SPI=y, CONFIG_BT=y, CONFIG_BT_PERIPHERAL=y, CONFIG_BT_HRS=y
- [ ] AC-B4: build/ 目录包含 app core 和 ipc_radio net core 子构建

### P1 强烈建议

- [ ] AC-B5: `west build -t rom_report` 包含 `bt_hrs_*` 与 `spi_nrfx_*` 符号
- [ ] AC-B6: `west build -t ram_report` 无 overflow
- [ ] AC-B7: 清空 build 目录重建仍 clean build
- [ ] AC-B8: `-DCONFIG_COMPILER_WARNINGS_AS_ERRORS=y` 无 warning
- [ ] AC-Q1: 无编译器 warning
- [ ] AC-Q2: 代码风格一致
- [ ] AC-Q3: Git history 干净

### P2 加分项

- [x] AC-R2: `west build -b nrf5340bsim/nrf5340/cpuapp` 编译成功
- [x] AC-R4: 增加 `sample.yaml` 支持 Twister 自动化
- [ ] **AC-V1**: bsim 端到端 HRS —— peripheral（本项目）↔ central (`samples/bluetooth/central_hr`)，central 日志看到 `bpm` notify（[REQUIREMENTS.md §6.3.2](../../REQUIREMENTS.md)）
- [ ] **AC-V2**: `native_sim` + `CONFIG_BT_HCI_USERCHAN=y` + USB BLE dongle，Chrome Web Bluetooth 经 ble-web-bluetooth-debugger skill 端到端收到 HR notify，`reports/ble-hr-connected.png` 存档

## 🚪 邀请门禁 (Invitation Gate)

**规则（Day 9 新增，用户明示）**：面试官 invitation（`gh repo` collaborator add + 邮件）**不得**在以下三道闸全绿前发出。

| 闸 | 内容 | 证据来源 |
|----|------|---------|
| **Gate-HW** | Hardware 目标镜像 clean build —— AC-1~AC-5 + AC-B1~B8 + AC-Q1~Q3 全绿 | `verify-acceptance.sh` 24/24 PASS + CI `build.yml` 最新 run GREEN |
| **Gate-SIM-BSIM** | Sim 镜像跑起来 BLE —— AC-V1 达成 | `reports/bsim-hrs-central.log` 含 ≥3 条 `bpm` |
| **Gate-SIM-CHROME** | Sim 镜像被外部 client 验证 —— AC-V2 达成 | `reports/ble-hr-connected.png` + `reports/ble-hr-log.txt` |

**理由**：面试官多半没有 DK 硬件，clean build 只是可编译证据；加 Gate-SIM-* 后，面试官 clone 仓库后就能看到：
1. DK build 产出 `merged.hex`（Gate-HW）
2. bsim 双进程握手截图 / 日志（Gate-SIM-BSIM）
3. Chrome 浏览器里真实 2.4 GHz 频段上收到的 HR notify 截图（Gate-SIM-CHROME）

三层证据递进，覆盖「编译 → 虚拟运行 → 真实 radio」完整链路，无需面试官准备任何 Nordic 硬件。

**执行约束**：
- 每日 wrap-up 时同步 `REQUIREMENTS.md §6` 顶部的状态表 —— 这是 gate 状态的单一真相源
- 发邀请前必须在当前 commit 上跑一次 `verify-acceptance.sh`，确认 AC-V1 + AC-V2 不是 `[SKIP]` 而是 `[PASS]`
- 允许先把仓库设 public 并加 CI badge，但 `gh repo edit --add-collaborator` / 发邮件这一步锁在闸后

## 工作模式

### 每日迭代

每天按以下流程工作：

1. **晨会计划** (Morning Planning)
   - 回顾昨日进度
   - 确定今日目标（2-3 个具体任务）
   - 识别风险和阻塞

2. **执行开发** (Execute)
   - 按优先级逐个完成任务
   - 每个任务完成后立即测试（`west build`）
   - 构建失败要先修复再继续

3. **回归验证** (Regression Gate) ⚠️ 每日必做
   - 运行 `verify-acceptance.sh`（自动检查所有已标记 OK 的验收项）
   - 新功能不能破坏已通过的验收项 — **任何回归必须当日修复**
   - 回归验证通过后才能进行 wrap-up commit

4. **晚间回顾** (Evening Review)
   - 记录完成情况
   - 更新进度指标
   - 规划明日工作
   - **记录 Skill/工作流反馈** — 见下方「Skill 反馈」章节

### 开发工具使用

#### Python 环境（强制）

项目中所有 Python 操作 **必须** 使用项目根目录下的 `.venv/` 虚拟环境。

```bash
# 首次初始化
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt  # 如有

# 每次开发前激活
source .venv/bin/activate
```

**规则：**
- ⛔ **禁止** 使用系统 Python 或 `--break-system-packages` 安装包
- ✅ 所有 `pip install` 必须在 `.venv/` 激活状态下执行
- ✅ `.venv/` 已加入 `.gitignore`，不提交到仓库

#### tmux 环境

```bash
# 启动项目工作环境
tmux new-session -d -s nrf5340
tmux new-window -t nrf5340 -n build
tmux new-window -t nrf5340 -n flash
tmux new-window -t nrf5340 -n monitor
```

#### 编译-烧录-测试循环

```bash
# 1. 编译（核心命令 — 必须）
west build -b nrf5340dk/nrf5340/cpuapp --sysbuild

# 2. 烧录（⚠️ 可选 — 需要 nRF5340 DK 硬件）
# west flash

# 3. 串口监控（⚠️ 可选 — 需要硬件）
# nRF5340 DK 使用 J-Link RTT 或 UART
# RTT: JLinkRTTViewerExe
# UART: minicom -D /dev/ttyACM0 -b 115200

# 4. 快速重建（增量编译）
west build

# 5. 清空重建（完整验证）
rm -rf build && west build -b nrf5340dk/nrf5340/cpuapp --sysbuild
```

#### 构建验证检查点

```bash
# CMake 配置校验
west build -b nrf5340dk/nrf5340/cpuapp --sysbuild --cmake-only

# DTS 展开校验（使用 find 定位实际构建目录）
BUILD_APP_DIR=$(find build -name "zephyr.dts" -path "*/zephyr/*" ! -path "*/ipc_radio/*" | head -1 | xargs dirname)
cat "$BUILD_APP_DIR/zephyr.dts" | grep -A5 "spi4"

# Kconfig 生效校验
grep -E "CONFIG_(SPI|BT|BT_PERIPHERAL|BT_HRS)" "$BUILD_APP_DIR/.config"

# Sysbuild 多镜像校验
ls build/ipc_radio/ && ls build/merged.hex

# ROM/RAM 报告
west build -t rom_report
west build -t ram_report
```

### 重构策略

重构 **不按固定周期**，而是根据每日 health check 自适应触发。

**触发条件（满足任一即触发重构）：**
- 编译警告 ≥ 3
- 单文件 ≥ 250 行
- 单函数 ≥ 40 行
- TODO/FIXME ≥ 5
- 连续功能开发 ≥ 4 天
- 重复代码 ≥ 2 处

**重构日规则：**
- 🔧 不加新功能，只做代码改进
- 重构后必须零警告 + `west build` 通过
- 优先级：修警告 > 拆大文件 > 消除重复代码 > 命名规范

### Git 提交规范

```
feat: 添加新功能
fix: 修复 bug
refactor: 代码重构
docs: 文档更新
test: 测试相关
chore: 构建/工具变更
```

## 代码质量要求

- 每个 `.c` 文件不超过 300 行（250+ 行触发重构警告）
- 每个函数不超过 50 行（40+ 行触发重构警告）
- 所有错误码必须检查（不忽略返回值）
- 日志使用 Zephyr `LOG_INF` / `LOG_ERR` / `LOG_WRN` 统一格式
- 注释语言保持一致（英文为主，面试项目）
- 零编译警告
- TODO/FIXME 不超过 5 个

## 测试要求

- ✅ **每个工作日结束前 `west build --sysbuild` 必须通过**
- ✅ **每个工作日结束前运行 `verify-acceptance.sh`** — 覆盖所有已完成功能的验收项
- 新功能不得导致已通过的验收项回归失败
- 回归失败必须在当日 wrap-up commit 前修复
- 构建失败必须在当日 wrap-up commit 前修复
- 有硬件时：串口日志验证 SPI loopback 和 BLE 广播
- 无硬件时：静态验证（DTS/Kconfig/ROM-RAM 报告）为主

### 累积回归规则

随里程碑推进，验证范围**只增不减**：

| 完成里程碑 | 每日必须验证的项 |
|-----------|----------------|
| M1 完成 | CMake 配置、项目骨架、verify-acceptance.sh 就绪 |
| M2 完成 | + SPI DTS overlay、SPIM4 配置、构建通过 |
| M3 完成 | + BLE Kconfig、ipc_radio、merged.hex |
| M4 完成 | + README、.gitignore、LLM 声明、所有 P0 |

## 硬件假设

- **本项目不强制要求硬件** — clean build 即为核心验收标准
- 如有 nRF5340 DK 可进行实机验证（加分但非必须）
- nRF5340 双核架构：App Core (M33@128MHz) + Net Core (M33@64MHz)
- SPIM4 是唯一支持 32MHz 的 SPI 实例
- BLE controller 运行在 Net Core (ipc_radio)

## 禁止事项

- ❌ 不要为赶进度跳过构建验证
- ❌ 不要一次提交大量未测试的代码
- ❌ 不要忽略编译警告
- ❌ 不要将 build/ 或 .west/ 提交到仓库
- ❌ 不要修改 NCS SDK 源码（只通过 overlay/conf 配置）
- ❌ 不要在 prj.conf 中启用不必要的模块（增加 footprint）
- ❌ **不得在邀请门禁（Gate-HW + Gate-SIM-BSIM + Gate-SIM-CHROME）未全绿前发出面试官 invitation**（见 🚪 邀请门禁段）

## Skill 反馈 (Feedback Loop)

在每日开发过程中，将遇到的 skill/工作流问题或改进建议记录到 **`docs/skill-feedback.md`**（项目根目录下）。

### 何时记录

- 某个 Skill 的步骤不完整或有误
- 工作流程中发现可以优化的环节
- 发现需要但不存在的 Skill
- 某个工具或命令的用法与 Skill 描述不一致

### 记录格式

在 `docs/skill-feedback.md` 末尾追加：

```markdown
### FB-NNN (YYYY-MM-DD)
- **Skill**: <skill-name 或 workflow/agent/tools>
- **Category**: <bug | improvement | missing-feature | documentation>
- **Summary**: <一句话总结>
- **Detail**: <详细描述问题和上下文>
- **Workaround**: <如有，描述临时解决方案>
- **Priority**: <high | medium | low>
```

## 发布流程 (M4: GitHub 发布)

### 发布前检查

```bash
# 1. 运行验收脚本（必须全部 PASS）
bash verify-acceptance.sh

# 2. 确认 README 完整
grep -q "## Build Instructions" README.md && echo "✅ 构建说明" || echo "❌ 缺少构建说明"
grep -qi "llm\|language model" README.md && echo "✅ LLM 声明" || echo "❌ 缺少 LLM 声明"

# 3. 确认 .gitignore
cat .gitignore | grep -E "^build/|^\.west/|^\.venv/" | wc -l
# 预期: ≥ 3

# 4. Git 状态干净
git status --short  # 应该为空或只有 untracked 文件已 gitignore
```

### 发布步骤

```bash
# 1. 初始化仓库（如果还没有）
git init
git add -A
git commit -m "feat: nRF5340 SPI loopback + BLE HRS — clean build"

# 2. 创建 GitHub 仓库并推送
gh repo create Nordic_nRF5340_SPI_loopback --public --source=. --push
# 或手动:
# git remote add origin https://github.com/<user>/Nordic_nRF5340_SPI_loopback.git
# git push -u origin main

# 3. 邀请协作者
gh repo edit --add-topic nrf5340 --add-topic zephyr --add-topic ble
# 通过 GitHub UI 邀请面试官 (Settings → Collaborators → Add people)
```

### 发布后自测 (Fresh Clone Test)

**在另一个目录验证仓库的完整性和可构建性：**

```bash
#!/bin/bash
# post-publish-verify.sh — 发布后验证

REPO_URL="https://github.com/<user>/Nordic_nRF5340_SPI_loopback.git"
CLONE_DIR="/tmp/nrf-verify-$(date +%s)"

echo "=== 发布后验证 ==="

# Step 1: 克隆
echo "[1/5] 克隆仓库..."
git clone "$REPO_URL" "$CLONE_DIR" || { echo "❌ 克隆失败"; exit 1; }
cd "$CLONE_DIR"

# Step 2: 文件完整性
echo "[2/5] 检查文件完整性..."
REQUIRED_FILES="CMakeLists.txt prj.conf app.overlay src/main.c src/spi_loopback.c src/ble_hrs.c README.md .gitignore"
for f in $REQUIRED_FILES; do
    test -f "$f" && echo "  ✅ $f" || echo "  ❌ $f 缺失"
done

# Step 3: 不应存在的文件
echo "[3/5] 检查排除项..."
test ! -d build && echo "  ✅ 无 build/" || echo "  ❌ build/ 被提交"
test ! -d .west && echo "  ✅ 无 .west/" || echo "  ❌ .west/ 被提交"
test ! -d .venv && echo "  ✅ 无 .venv/" || echo "  ❌ .venv/ 被提交"

# Step 4: 构建验证（需要 NCS 环境）
echo "[4/5] 构建验证..."
west build -b nrf5340dk/nrf5340/cpuapp --sysbuild && \
    echo "  ✅ Fresh clone build 成功" || echo "  ❌ Fresh clone build 失败"

# Step 5: 运行验收脚本
echo "[5/5] 运行验收脚本..."
if [ -f verify-acceptance.sh ]; then
    bash verify-acceptance.sh
else
    echo "  ⚠️  verify-acceptance.sh 不存在"
fi

# 清理
echo ""
echo "清理: rm -rf $CLONE_DIR"
rm -rf "$CLONE_DIR"
```

### 发布 Checklist

- [ ] `verify-acceptance.sh` 全部 PASS
- [ ] README.md 包含：构建说明、硬件接线（可选）、LLM 使用声明
- [ ] `.gitignore` 排除 `build/`, `.west/`, `.venv/`, IDE 文件
- [ ] Git history 干净（有意义的 commit message）
- [ ] GitHub 仓库已创建并 push
- [ ] 面试官已被邀请为 collaborator
- [ ] Fresh clone test 通过

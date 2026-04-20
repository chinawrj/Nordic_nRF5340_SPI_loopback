# 需求文档 / Requirements Specification

**项目**: Nordic nRF5340 DK — SPI Loopback + BLE Heart Rate
**性质**: 面试技术任务（决定 Offer）
**交付物**: 可 clean build 的 GitHub 仓库 + README（含 LLM 使用说明）
**日期**: 2026-04-20

---

## 1. 原始需求（逐句对齐）

邮件原文要点拆解：

| # | 原文 | 理解 / 确认点 |
|---|------|--------------|
| R1 | "Load the latest Nordic SDK and toolchain onto your PC" | 使用**最新**的 nRF Connect SDK (NCS)。当前最新稳定版需确认（截至 2026-04 预计 v2.9.x 或 v3.x）。用 nRF Connect for VS Code 扩展安装 toolchain manager。 |
| R2 | "using this IDE in VS code set up a project for the nRF5340 DK" | 在 **VS Code** 中，使用 **nRF Connect for VS Code** 扩展创建项目。目标板：`nrf5340dk/nrf5340/cpuapp`。 |
| R3 | "set up a SPI loopback test for the 32 MHz capable SPI interface" | **32 MHz SPI** = 必须使用 **SPIM4**（nRF5340 上唯一支持 32 MHz 的 SPI 实例，其他 SPIM0–3 上限 8 MHz）。Loopback 测试：MOSI 短接 MISO，发送 buffer，校验接收 buffer 一致。 |
| R4 | "a BLE stack for communicating heart rate data over BLE from the device" | Peripheral 角色，GATT **Heart Rate Service (HRS, UUID 0x180D)**，含 Heart Rate Measurement characteristic (0x2A37) notify。可周期性推送模拟心率数据。 |
| R5 | "provide us with that project from a GitHub repository that you can invite us to" | GitHub 仓库，需提供可邀请协作者的访问方式（public 或 private + invite）。 |
| R6 | "only need to create the project to a clean build level, no need to run on the hardware" | **验收标准 = clean build 通过**（`west build` 无错误，产出 merged.hex）。不强制硬件验证。 |
| R7 | "Tell us what LLMs (including version numbers) you used in the task and how" | README 中必须列出使用的 LLM 型号/版本及用途。**诚实声明**是考察点之一（工程诚信）。 |

**潜在隐含考察点（未明说但重要）**：
- 对 nRF5340 **双核架构**的理解（app core + network core）
- 对 **sysbuild** / 多镜像构建的掌握
- 对 **devicetree overlay** 的使用（pinctrl + SPIM4 配置）
- Git/GitHub 工程规范（.gitignore、commit 粒度、README 质量）
- 代码组织（不是把所有东西堆在 main.c）

---

## 2. 功能需求 (FR)

### FR-1: SPI Loopback
- **FR-1.1** 使用 SPIM4 实例，配置 clock-frequency = 32 MHz
- **FR-1.2** 使用 Zephyr SPI API (`<zephyr/drivers/spi.h>`, `spi_transceive`)
- **FR-1.3** 通过 DTS overlay 启用 SPIM4 并分配 pinctrl（SCK/MOSI/MISO）
- **FR-1.4** 周期性发送测试 pattern（如 32 字节递增序列），接收后逐字节比对
- **FR-1.5** 结果通过 `LOG_INF` / `LOG_ERR` 打印到 RTT / UART
- **FR-1.6** 运行在独立线程或 work queue，不阻塞 BLE

### FR-2: BLE Heart Rate Peripheral
- **FR-2.1** 启用 BLE 协议栈（app core: host；net core: controller via `ipc_radio` / `hci_ipc`）
- **FR-2.2** 设备名 `nRF5340_HR`（可配置）
- **FR-2.3** 广播包含 HRS service UUID (0x180D)
- **FR-2.4** 支持一个 Central 连接（Peripheral role）
- **FR-2.5** 连接后，周期性（1 Hz）通过 `bt_hrs_notify()` 发送模拟心率（如在 60–100 bpm 间变化）
- **FR-2.6** 断开连接后自动恢复广播

### FR-3: 构建与交付
- **FR-3.1** 项目在 VS Code + nRF Connect 扩展中可直接打开构建
- **FR-3.2** `west build -b nrf5340dk/nrf5340/cpuapp --sysbuild` 成功无错误
- **FR-3.3** 产出 `build/merged.hex`（app + net core 合并）
- **FR-3.4** GitHub 仓库公开或提供邀请链接
- **FR-3.5** README 说明构建步骤、硬件接线、LLM 使用声明

---

## 3. 非功能需求 (NFR)

| ID | 项 | 要求 |
|----|---|------|
| NFR-1 | 代码质量 | 模块化（spi / ble / main 分离），无编译 warning |
| NFR-2 | 可读性 | 关键函数有注释；README 条理清晰 |
| NFR-3 | 可复现 | 固定 NCS 版本（west manifest 或 README 中声明）|
| NFR-4 | 仓库规范 | `.gitignore` 排除 `build/`、`.west/`、IDE 临时文件 |
| NFR-5 | 诚信 | LLM 使用如实声明 |

---

## 4. 约束与假设

### 约束
- **C-1** 只需 clean build，无需实机运行（无需 DK 硬件也能交付）
- **C-2** 使用 NCS 官方最新稳定版（避免用 main 分支不稳定版本）
- **C-3** 所有 Nordic 专有代码遵循其 license（不 fork NCS 进仓库，只引用）

### 假设
- **A-1** 本机已装（或可装）nRF Connect for VS Code + Toolchain Manager
- **A-2** 目标板 variant：`nrf5340dk/nrf5340/cpuapp`（当前 NCS 板名约定）
- **A-3** 使用 sysbuild（NCS v2.7+ 默认）自动管理 net core 镜像
- **A-4** Net core 使用 NCS 提供的 `ipc_radio` sample 镜像作为 BLE controller

---

## 5. 技术路线 / Architecture

### 5.1 硬件架构
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

### 5.2 软件栈
- **应用层**: `main.c` 初始化 → 启动 SPI 线程 + BLE 广播
- **SPI 模块**: `spi_loopback.c` — 独立线程，周期测试
- **BLE 模块**: `ble_hrs.c` — 初始化栈、注册 HRS 回调、周期 notify
- **Zephyr 子系统**: kernel threads、log、BT host (NCS)、SPI driver (nrfx)

### 5.3 Devicetree Overlay 策略
```
app.overlay:
  &pinctrl {
    spi4_default: spi4_default { ... SCK/MOSI/MISO 引脚 + HIGH drive ... };
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

### 5.4 Kconfig 策略（`prj.conf`）
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

### 5.5 项目目录
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
├── REQUIREMENTS.md   (本文件)
└── README.md         (交付说明 + LLM 声明)
```

---

## 6. 验收标准 (Acceptance Criteria)

### 6.0 交付件状态总表（Delivery Status Snapshot）

> 每日 wrap-up 时刷新这张表。**面试官邀请门禁**（见 §6.3.2 + `.github/agents/dev-workflow.agent.md`）要求 §6.1 + AC-V1 + AC-V2 全绿才放行。

| ID | 范畴 | 标题 | 优先级 | 状态 | 证据 / 命令 |
|----|------|------|-------|------|-------------|
| AC-1  | §6.1 | `west build --sysbuild` 退出 0 | P0 | ✅ | `verify-acceptance.sh`, CI run `24658312584` |
| AC-2  | §6.1 | `merged.hex` 覆盖 app+net core | P0 | ✅ | harness AC-B4.d |
| AC-3  | §6.1 | GitHub 仓库 + 可邀请 | P0 | ✅ (repo live, invite gated by §6.3.2) | `chinawrj/Nordic_nRF5340_SPI_loopback` |
| AC-4  | §6.1 | README LLM 声明 | P0 | ✅ | harness AC-4 |
| AC-5  | §6.1 | `.gitignore` 排除 build/west/venv | P0 | ✅ | harness AC-5.a–c |
| AC-B1 | §6.2 | `--cmake-only` 成功 | P0 | ✅ | harness 隐含（AC-1 subset） |
| AC-B2 | §6.2 | DTS spi4 okay/32MHz/pinctrl | P0 | ✅ | harness AC-B2.a–c |
| AC-B3 | §6.2 | Kconfig SPI+BT+HRS+DIS+HCI_IPC | P0 | ✅ | harness AC-B3:* |
| AC-B4 | §6.2 | Sysbuild multi-image (app+net) | P0 | ✅ | harness AC-B4.a–e |
| AC-B5 | §6.2 | `rom_report` 含 bt_hrs_* / spi_nrfx_* | P1 | ✅ | `docs/reports/rom-report-cpuapp.txt` |
| AC-B6 | §6.2 | `ram_report` 无 overflow | P1 | ✅ | `docs/reports/ram-report-cpuapp.txt` |
| AC-B7 | §6.2 | 清空 build 重建仍绿 | P1 | ✅ | harness 每次 `rm -rf build` |
| AC-B8 | §6.2 | `-DCONFIG_COMPILER_WARNINGS_AS_ERRORS=y` | P1 | ✅ | CI workflow step |
| AC-Q1 | §6.4 | 零编译器 warning | P1 | ✅ | AC-B8 即保证 |
| AC-Q2 | §6.4 | 代码风格一致 | P1 | ✅ | 手审 |
| AC-Q3 | §6.4 | Git history 干净 | P1 | ✅ | 语义 commit 到 Day 9 |
| AC-R2 | §6.3 | `nrf5340bsim/nrf5340/cpuapp` 编译 | P2 | ✅ | harness "P2: nrf5340bsim compile" |
| **AC-V1** | §6.3.2 | **bsim run: HRS peripheral↔central, notify 可见** | **P2 (gate)** | ⏳ Day 10 target | `harness "P2: bsim HRS run"` 待实装 |
| **AC-V2** | §6.3.2 | **native_sim + Chrome Web Bluetooth 端到端收到 HR** | **P2 (gate)** | ⏳ Day 10+ target | `reports/ble-hr-connected.png` 需生成 |
| AC-R1 | §6.3 | native_sim 冒烟（可选） | P2 | N/A | AC-V2 覆盖了 native_sim 构建 |
| AC-R3 | §6.3 | bsim 端到端 BLE HRS（AC-V1 超集） | P2 | ⏳ 被 AC-V1 取代 | — |
| AC-R4 | §6.3 | Twister 自动化 | P2 | ✅ | `sample.yaml` 已加 |

图例：✅ 已达成 / ⏳ 规划中 / ⛔ 阻塞 / N/A 不适用。

### 6.1 基础验收（邮件明确要求）
- [ ] AC-1: `west build -b nrf5340dk/nrf5340/cpuapp --sysbuild` 返回 0，无 error、无关键 warning
- [ ] AC-2: build 输出包含 `merged.hex`（app + net core 合并镜像）
- [ ] AC-3: GitHub 仓库可访问并能邀请协作者
- [ ] AC-4: README 中包含完整 LLM 使用声明（邮件明确要求）
- [ ] AC-5: `.gitignore` 正确排除 build 产物

### 6.2 无硬件自测：构建期（Static / Build-time）验证

> 目标：在只有电脑、没有 nRF5340 DK 的条件下，尽可能深度验证交付物的正确性。以下所有检查均可在 NCS workspace 内完成。

- [ ] **AC-B1 (Cmake 配置校验)**：`west build -b nrf5340dk/nrf5340/cpuapp --sysbuild --cmake-only` 成功，无 CMake error
- [ ] **AC-B2 (Devicetree 展开校验)**：检查 `build/nRF5340_SPI_loopback/zephyr/zephyr.dts`，确认：
  - `spi4` 节点 `status = "okay"`
  - `spi4` 节点 `clock-frequency = <0x01e84800>`（= 32,000,000，十六进制）
  - `pinctrl-0` 正确引用自定义 `spi4_default` 节点
- [ ] **AC-B3 (Kconfig 生效校验)**：检查 `build/nRF5340_SPI_loopback/zephyr/.config`，确认：
  - `CONFIG_SPI=y`、`CONFIG_SPI_NRFX=y`
  - `CONFIG_BT=y`、`CONFIG_BT_PERIPHERAL=y`、`CONFIG_BT_HRS=y`
  - `CONFIG_BT_DEVICE_NAME="nRF5340_HR"`
- [ ] **AC-B4 (Sysbuild 多镜像校验)**：`build/` 目录下同时存在
  - `nRF5340_SPI_loopback/` (app core 子构建)
  - `ipc_radio/` (net core 子构建 — BLE controller 镜像)
  - 根 `merged.hex` 大小 > app 单独 hex（证明包含 net core）
- [ ] **AC-B5 (符号 / Section 检查)**：`west build -t rom_report` 成功生成 ROM 报告，包含 `bt_hrs_*` 与 `spi_nrfx_*` 相关符号
- [ ] **AC-B6 (内存足迹)**：`west build -t ram_report` 无 overflow；Flash/RAM 使用量在 nRF5340 可用范围内（app core: 1MB flash / 512KB RAM；net core: 256KB flash / 64KB RAM）
- [ ] **AC-B7 (跨板编译烟测)**：清空 build 目录重建，仍然 clean build（排除 stale cache 假成功）
- [ ] **AC-B8 (Zero Warnings 选项)**：可选用 `-DCONFIG_COMPILER_WARNINGS_AS_ERRORS=y` 再跑一次 —— 有 warning 直接 fail

### 6.3 无硬件自测：运行期（Runtime）仿真验证（加分项）

> nRF5340 在 Zephyr 中有**专用的仿真板** `nrf5340bsim/nrf5340/cpuapp` 与 `nrf5340bsim/nrf5340/cpunet`，基于 POSIX 架构编译成 Linux 可执行文件，配合 **BabbleSim**（物理层无线信道仿真器）可在纯 Linux 主机上端到端跑 BLE 协议栈。这是官方认可的"无硬件验证 BLE"手段。

- [ ] **AC-R1 (native_sim 冒烟，可选)**：对纯应用逻辑部分（不依赖 nrfx 硬件寄存器的逻辑）尝试 `west build -b native_sim` — 如果 SPI / BLE 依赖过重则跳过此项
- [ ] **AC-R2 (bsim 板编译)**：`west build -b nrf5340bsim/nrf5340/cpuapp` 能成功编译出 `zephyr.exe`（Linux ELF）
- [ ] **AC-R3 (BabbleSim 运行 BLE HRS — 最高置信度)**：在安装了 BabbleSim 的环境下，启动仿真，同时跑：
  1. 本项目 `zephyr.exe`（DUT，扮演 HRS Peripheral）
  2. BabbleSim 的 `bs_device_handbrake` + phy simulator
  3. 一个简单的 BLE Central 测试脚本（或 Zephyr `central_hr` sample 也编成 bsim）
  
  验证：Central 能扫描到设备名 `nRF5340_HR`，连接后能收到 HRS notification（心率值）。
- [ ] **AC-R4 (Twister 自动化)**：增加 `sample.yaml`，用 `twister -p nrf5340dk/nrf5340/cpuapp --build-only` 自动化 build 回归；若加 bsim 则 `--platform nrf5340bsim/nrf5340/cpuapp` 跑运行回归

### 6.3.2 硬件缺席下的运行期 BLE 验证（Day 9 新增，邀请门禁）

> 背景：clean build + AC-R2 compile-only 证明了配置与编译正确，但没有证明 **BLE HRS 应用代码在运行期行为正确**。对无硬件的面试官，他要么自己 flash 到 DK、要么只能看日志截图。为了在不依赖面试官自备硬件的前提下给出可复现的运行期证据，加以下两条 **P2 运行期 AC**。二者覆盖两种不同的 phy：纯虚拟（bsim）与真实 host adapter（HCI user-channel）。
>
> **邀请门禁**：`AC-V1` 与 `AC-V2` 双绿后，才允许发面试官 invitation（规则写在 `.github/agents/dev-workflow.agent.md`）。这是 Day 9 用户明示的新约束。

#### AC-V1 — bsim 端到端 HRS（纯虚拟 phy）

**前置**：BabbleSim 已装，`BSIM_OUT_PATH` / `BSIM_COMPONENTS_PATH` 已导出（Day 7 已满足）。

- [ ] **AC-V1.a** 把本项目（HRS peripheral）编译成 `nrf5340bsim/nrf5340/cpuapp`（AC-R2 已证实可行）
- [ ] **AC-V1.b** 将 Zephyr `samples/bluetooth/central_hr` 同样编成 `nrf5340bsim/nrf5340/cpuapp`（单独 build-dir，例如 `build-bsim-central/`）
- [ ] **AC-V1.c** 用 `bs_2G4_phy_v1 -s=<sim_id> -D=2` 启动 phy，两个 `zephyr.exe -s=<sim_id> -d=0/1` 分别加入
- [ ] **AC-V1.d** Central 侧日志可 grep 到：
  - `Device found: <addr> (RSSI ...) C: 0 connectable: 1`（或等价 "found nRF5340_HR"）
  - `Connected:` + `HRS: discovery completed`
  - 至少 3 条 `HRS Measurement: <N> bpm`（证明 notify 路径正常）
- [ ] **AC-V1.e** 将 central 日志存至 `reports/bsim-hrs-central.log`，作为验收证据

验证脚本（大致）：
```bash
sim_id=nrf5340hrs-$(date +%s)
(cd $BSIM_OUT_PATH/bin && ./bs_2G4_phy_v1 -s=$sim_id -D=2 -sim_length=10e6) &
./build-bsim/Nordic_nRF5340_SPI_loopback/zephyr/zephyr.exe -s=$sim_id -d=0 &
./build-bsim-central/central_hr/zephyr/zephyr.exe -s=$sim_id -d=1 | tee reports/bsim-hrs-central.log
wait
grep -q 'bpm' reports/bsim-hrs-central.log
```

#### AC-V2 — native_sim + Chrome Web Bluetooth（真实 2.4 GHz phy）

**前置**：两个 BLE adapter（系统 `hci0` + USB dongle `hci1`）；dongle 用 `sudo rfkill unblock bluetooth && sudo hciconfig hci1 up`。

- [ ] **AC-V2.a** 新增 `native_sim` 配置：在 `boards/` 下（或 `native_sim.conf` overlay）加 `CONFIG_BT_HCI_USERCHAN=y`、保留 HRS，关闭 SPIM4（已被 Day 7 的 `DT_NODE_EXISTS` 守卫覆盖）
- [ ] **AC-V2.b** `west build -b native_sim --build-dir build-native -- -DCONF_FILE="prj.conf;native_sim.conf"` 产出 `zephyr.exe`
- [ ] **AC-V2.c** 以 `sudo ./zephyr.exe --bt-dev=hci1` 启动，确认控制台打印 `Bluetooth initialized` + `Advertising as nRF5340_HR`
- [ ] **AC-V2.d** 用 **ble-web-bluetooth-debugger** skill 的 Patchright 脚本，在 Chrome 里通过 `hci0` 扫描并连接 `nRF5340_HR`，订阅 0x2A37 notify
- [ ] **AC-V2.e** 浏览器里 `#hr-value` 显示非空 bpm 至少 5 秒
- [ ] **AC-V2.f** `reports/ble-hr-connected.png` 截图保存（Patchright 自动输出）；`reports/ble-hr-log.txt` 保存 notify 日志

评审证据：`reports/ble-hr-connected.png` 直接放进 README 的 verification 段，让面试官无需本地复现就能看到 end-to-end 行为。

### 6.4 静态代码质量（免费拿到的保证）

- [ ] **AC-Q1**：无编译器 warning（`-Wall -Wextra` 默认由 Zephyr 启用）
- [ ] **AC-Q2**：`clang-format` / Zephyr 代码风格一致（可选 `west format`）
- [ ] **AC-Q3**：Git history 干净（合理的 commit 粒度，而不是一次 dump）

### 6.5 验收优先级（时间紧张时）

**必过（P0）**：§6.1 全部 + §6.2 AC-B1, AC-B2, AC-B3, AC-B4 —— 这是 clean build + 证明配置正确的最小集。

**强烈建议（P1）**：§6.2 AC-B5, AC-B6, AC-B7, AC-B8 + §6.4 全部 —— 体现工程严谨性。

**加分项（P2）**：§6.3 bsim 端到端 BLE 仿真 —— 在没有硬件的情况下展示对 Nordic 生态的深度掌握，极大加分；唯一成本是 BabbleSim 的额外安装。

---

## 6A. 无硬件验证工具链速查

| 工具 | 用途 | 命令/方式 | 来源 |
|------|------|----------|------|
| `west build --cmake-only` | 不编译只验证 CMake/DTS/Kconfig 配置 | `west build -b <board> --cmake-only --sysbuild` | Zephyr 标准 |
| `zephyr.dts` 检查 | 验证 DTS overlay 生效 | 查看 `build/*/zephyr/zephyr.dts` | 自动生成 |
| `.config` 检查 | 验证 Kconfig 最终值 | 查看 `build/*/zephyr/.config` | 自动生成 |
| `rom_report` / `ram_report` | 静态 footprint 分析 | `west build -t rom_report` | Zephyr 标准 |
| **Twister** | 测试运行器，支持 `--build-only` 做回归 | `twister -T . -p <board> --build-only` | Zephyr 标准 |
| **native_sim** | 纯 POSIX 应用仿真（不含 nRF HW 模型）| `west build -b native_sim` | Zephyr 标准 |
| **nrf5340bsim** | nRF5340 HW 模型 + POSIX 仿真 | `west build -b nrf5340bsim/nrf5340/cpuapp` | Zephyr/Nordic |
| **BabbleSim** | BLE/15.4 物理层信道仿真器 | 独立安装 (`BSIM_OUT_PATH`) | [babblesim.github.io](https://babblesim.github.io) |
| `nrfutil` / `nrfjprog` | 烧录/调试（本任务**不需要**） | — | Nordic |

---

## 7. 风险与缓解

| 风险 | 影响 | 缓解 |
|------|------|------|
| NCS 版本与 API 变化（board naming 从 v2.7 起改 hierarchical） | 构建失败 | 使用当前稳定版并在 README 固定版本号 |
| SPIM4 32 MHz 信号完整性需要 HIGH drive pin | 实机可能跑不通（但 clean build 不受影响）| 在 pinctrl 中设置 `nordic,drive-mode` 或用 DK 提供的 HS 引脚；README 注明 |
| Net core 镜像未正确构建 | 无 BLE | 使用 sysbuild + 官方 `ipc_radio` 模板 |
| 本机无 NCS 环境 | 无法验证 clean build | 先验证本机安装，或用 NCS docker/toolchain manager |
| LLM 声明不诚实 | 面试减分 | 诚实列出 Copilot / Claude 等及具体用途 |

---

## 8. 待确认/决策项（请 review）

1. **NCS 版本**: 拟使用最新稳定版（将在实施前通过 `west` / 官方页面确认）。是否接受？
2. **板名 variant**: `nrf5340dk/nrf5340/cpuapp`（hierarchical，NCS v2.7+）。是否 OK？
3. **SPI 引脚选择**: 需从 DK 可用引脚中选两个物理相邻、方便跳线短接的做 MOSI/MISO（会在 overlay 中注明）。是否由我定？
4. **心率数据**: 使用模拟数据（60–100 bpm 递增/正弦）。是否 OK，还是需要接入特定传感器？
5. **GitHub 仓库**: 名称用 `Nordic_nRF5340_SPI_loopback`（匹配当前工作区），public repo。是否 OK？
6. **LLM 声明格式**: 计划列出 "GitHub Copilot (Claude Opus 4.7) — 用于生成 Kconfig 模板、DTS overlay 草稿、README 文案；所有代码经人工审阅并本地 clean build 验证"。是否接受？
7. **本机 NCS 安装状态**: 需确认 `~/ncs` 是否已装 toolchain。要不要我现在检查？
8. **是否启用 bsim 加分项（§6.3）**: 额外安装 BabbleSim，在无硬件情况下端到端验证 BLE HRS。成本：一次性配置 ~30 分钟；收益：面试加分明显（展示对 Nordic 生态深度掌握）。建议 **YES**，是否同意？
9. **是否加 Twister 集成（§6.2 AC-B 自动化）**: 增加 `sample.yaml` 使 `twister --build-only` 能自动回归构建。成本：十几行 YAML；收益：体现工程化思维。建议 **YES**，是否同意？

---

## 9. 下一步

待你确认 §8 中的决策项后，进入实施：
1. 检查 / 安装 NCS 工具链
2. 生成项目骨架（CMakeLists / prj.conf / sysbuild.conf / app.overlay / src/*）
3. 本地 `west build` 验证 clean build
4. `git init` + `.gitignore` + 初次 commit
5. 推送到 GitHub 并准备邀请
6. 完成 README（含 LLM 声明）

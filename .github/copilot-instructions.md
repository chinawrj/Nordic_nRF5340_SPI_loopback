# nRF5340 SPI Loopback + BLE HR — Copilot Instructions

## 项目定位

这是一个 **Nordic nRF5340 DK** 嵌入式项目，使用 **Zephyr RTOS / nRF Connect SDK (NCS)** 开发。
核心功能：SPIM4 32MHz SPI Loopback 测试 + BLE Heart Rate Service Peripheral。
交付标准：clean build + GitHub 仓库 + LLM 使用声明。

## 技术栈

| 项 | 值 |
|----|---|
| SoC | Nordic nRF5340 (双核: App Core M33@128MHz + Net Core M33@64MHz) |
| RTOS | Zephyr |
| SDK | nRF Connect SDK (NCS) **v2.9.0** |
| 构建工具 | west, CMake, ninja |
| IDE | VS Code + nRF Connect for VS Code |
| 板名 | `nrf5340dk/nrf5340/cpuapp` |
| 语言 | C (Zephyr API) |

## 构建命令

```bash
# 完整构建（含 net core）
west build -b nrf5340dk/nrf5340/cpuapp --sysbuild

# 仅 CMake 配置（快速验证）
west build -b nrf5340dk/nrf5340/cpuapp --sysbuild --cmake-only

# 增量构建
west build

# 清空重建
rm -rf build && west build -b nrf5340dk/nrf5340/cpuapp --sysbuild

# 烧录（需硬件）
west flash

# ROM/RAM 报告
west build -t rom_report
west build -t ram_report
```

## 项目结构

```
Nordic_nRF5340_SPI_loopback/
├── CMakeLists.txt          # 顶层 CMake
├── prj.conf                # Zephyr Kconfig
├── sysbuild.conf           # Sysbuild 配置（net core）
├── app.overlay             # Devicetree overlay（SPIM4 pinctrl）
├── src/
│   ├── main.c              # 入口：初始化 SPI 线程 + BLE 广播
│   ├── spi_loopback.c      # SPI loopback 测试模块
│   ├── spi_loopback.h
│   ├── ble_hrs.c           # BLE Heart Rate Service 模块
│   └── ble_hrs.h
├── .gitignore
├── REQUIREMENTS.md         # 需求文档
├── README.md               # 交付说明 + LLM 声明
└── .github/
    ├── copilot-instructions.md   # 项目级 AI 指令
    ├── agents/
    │   └── dev-workflow.agent.md  # Agent 定义
    └── skills/                    # Agent Skills
        ├── daily-iteration/
        ├── automated-testing/
        ├── code-refactoring/
        ├── tmux-multi-shell/
        └── ble-web-bluetooth-debugger/
```

## 工作规则

1. **所有构建必须使用 `west build`**，不要直接调用 cmake/ninja
2. **DTS overlay 修改后必须清空 build 重建**（DTS 缓存可能导致假成功）
3. **prj.conf 修改后建议清空重建**（Kconfig 变更有时不触发增量构建）
4. **不要修改 NCS SDK 源码** — 所有配置通过 prj.conf / app.overlay / sysbuild.conf
5. **Net core 使用 sysbuild 自动管理** — 不要手动构建 ipc_radio
6. **所有 Python 操作使用 `.venv/`** — 禁止系统 Python

## nRF5340 关键知识

### 双核架构
- **App Core** (M33@128MHz, 1MB Flash, 512KB RAM): 运行应用代码 + BLE Host
- **Net Core** (M33@64MHz, 256KB Flash, 64KB RAM): 运行 BLE Controller (ipc_radio)
- 两核通过 IPC (Inter-Processor Communication) 通信

### SPIM4
- nRF5340 上唯一支持 **32 MHz** 的 SPI 实例
- SPIM0–3 上限 8 MHz
- 需要 HIGH drive 引脚配置以保证信号完整性

### Sysbuild
- NCS v2.7+ 默认使用 sysbuild 管理多镜像构建
- `sysbuild.conf` 配置 net core 使用 `ipc_radio` + `bt_hci_ipc`
- 构建产出 `merged.hex` = app core hex + net core hex 合并

### BLE 配置
- `CONFIG_BT_HRS=y` 启用 Zephyr 内置 Heart Rate Service
- `bt_hrs_notify()` 发送心率数据
- Peripheral role: 广播 → 等待连接 → notify → 断开 → 恢复广播

## 代码质量要求

- 模块化：spi_loopback / ble_hrs / main 分离
- 每个 .c 文件 ≤ 300 行
- 每个函数 ≤ 50 行
- 零编译警告
- 使用 Zephyr LOG 子系统（LOG_MODULE_REGISTER）
- 错误码必须检查（spi_transceive 返回值等）
- 注释语言：英文

## Git 规范

```
feat: add SPI loopback test module
feat: add BLE HRS peripheral
fix: correct SPIM4 pinctrl configuration
docs: add README with build instructions and LLM statement
chore: add .gitignore and project skeleton
```

## 禁止事项

- ❌ 不要将 `build/`、`.west/`、`.venv/` 提交到 Git
- ❌ 不要在代码中硬编码调试用的引脚号（使用 DTS 定义）
- ❌ 不要跳过构建验证就提交
- ❌ 不要忽略编译警告
- ❌ 不要在 ISR 中执行复杂操作或日志打印
- ❌ 不要 fork NCS 代码到仓库

## 关键参考文档

开始工作前，请先阅读以下文档了解完整需求和计划：
- **`REQUIREMENTS.md`** (项目根目录) — 原始详细需求
- **`.github/requirements.md`** — 精炼的功能需求、验收标准 (P0/P1/P2)、里程碑定义
- **`.github/daily-plan.md`** — 每日迭代计划模板、里程碑进度跟踪

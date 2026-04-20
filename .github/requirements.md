# nRF5340_SPI_loopback - 项目需求文档

## 项目概述

**项目名称**: Nordic nRF5340 DK — SPI Loopback + BLE Heart Rate
**描述**: 面试技术任务 — 在 nRF5340 DK 上实现 SPIM4 32MHz SPI Loopback 测试 + BLE Heart Rate Service Peripheral，使用 Zephyr/NCS 构建，交付 clean build 的 GitHub 仓库 + README（含 LLM 使用说明）
**目标硬件**: Nordic nRF5340 DK (nrf5340dk/nrf5340/cpuapp)

## 功能需求

### FR-1: SPI Loopback (SPIM4 @ 32MHz)

- 使用 SPIM4 实例，配置 clock-frequency = 32 MHz
- 使用 Zephyr SPI API (`spi_transceive`)
- DTS overlay 启用 SPIM4 并分配 pinctrl（SCK/MOSI/MISO，HIGH drive）
- 周期性发送测试 pattern（32 字节递增序列），接收后逐字节比对
- 结果通过 `LOG_INF` / `LOG_ERR` 打印到 RTT / UART
- 运行在独立线程，不阻塞 BLE

### FR-2: BLE Heart Rate Peripheral

- 启用 BLE 协议栈（app core: host；net core: controller via ipc_radio）
- 设备名 `nRF5340_HR`
- 广播包含 HRS service UUID (0x180D)
- 支持一个 Central 连接（Peripheral role）
- 连接后 1Hz 周期发送模拟心率（60–100 bpm 变化）
- 断开连接后自动恢复广播

### FR-3: 构建与交付

- VS Code + nRF Connect 扩展可直接打开构建
- `west build -b nrf5340dk/nrf5340/cpuapp --sysbuild` 成功无错误
- 产出 `build/merged.hex`（app + net core 合并）
- GitHub 仓库公开或提供邀请链接
- README 说明构建步骤、硬件接线、LLM 使用声明

## 验收标准

### P0 必过
- [ ] `west build --sysbuild` clean build（返回 0，无 error）
- [ ] 产出 `merged.hex`（含 app + net core）
- [ ] GitHub 仓库可访问
- [ ] README 含 LLM 使用声明
- [ ] `.gitignore` 排除 build 产物
- [ ] CMake 配置校验通过
- [ ] DTS 展开确认 spi4 正确配置
- [ ] Kconfig 确认 SPI + BLE + HRS 启用
- [ ] Sysbuild 多镜像（app + ipc_radio）完整

### P1 强烈建议
- [ ] ROM/RAM 报告无 overflow，含关键符号
- [ ] 清空重建仍 clean build
- [ ] 零编译 warning
- [ ] 代码风格一致，Git history 干净

### P2 加分项
- [ ] bsim 仿真编译成功
- [ ] Twister 集成 (`sample.yaml`)

## 里程碑

### M1: 项目骨架 + 环境验证
- NCS 工具链安装验证
- 项目目录结构创建（CMakeLists.txt, prj.conf, sysbuild.conf, app.overlay, src/）
- `west build --cmake-only` 通过

### M2: SPI Loopback 实现
- SPIM4 DTS overlay 配置
- spi_loopback.c/h 实现
- 独立线程运行，周期测试
- 构建通过

### M3: BLE Heart Rate 实现
- BLE 协议栈配置（prj.conf + sysbuild.conf）
- ble_hrs.c/h 实现
- 模拟心率数据周期 notify
- 构建通过（含 net core ipc_radio）

### M4: 集成 + 验收
- SPI + BLE 集成到 main.c
- 全量构建验证（所有 AC-B 检查点）
- ROM/RAM 报告
- README + LLM 声明
- Git 提交历史整理
- GitHub 仓库发布

## 技术栈

- **SoC**: Nordic nRF5340 (双核: App Core M33@128MHz + Net Core M33@64MHz)
- **RTOS**: Zephyr
- **SDK**: nRF Connect SDK (NCS) 最新稳定版
- **构建工具**: west, CMake, ninja
- **IDE**: VS Code + nRF Connect for VS Code 扩展
- **板名**: `nrf5340dk/nrf5340/cpuapp`

## AI Agent Skills

以下 skills 将用于项目开发：

1. **daily-iteration** — 每日迭代工作流（计划→执行→验证→回顾）
2. **automated-testing** — 自动化构建验证与测试策略
3. **code-refactoring** — 周期性代码重构，维持代码质量
4. **tmux-multi-shell** — 多终端管理（编译窗口/串口监控/编辑器）
5. **ble-web-bluetooth-debugger** — 通过 Patchright 浏览器 + Web Bluetooth API 调试 BLE HRS

## 非功能需求

- 代码模块化（spi / ble / main 分离），无编译 warning
- 关键函数有注释；README 条理清晰
- 固定 NCS 版本（west manifest 或 README 中声明）
- `.gitignore` 排除 `build/`、`.west/`、IDE 临时文件
- LLM 使用如实声明
- Flash/RAM 使用量在 nRF5340 可用范围内

## 约束条件

- 只需 clean build，无需实机运行
- 使用 NCS 官方最新稳定版
- 不 fork NCS 进仓库，只引用
- App Core: 1MB Flash / 512KB RAM
- Net Core: 256KB Flash / 64KB RAM

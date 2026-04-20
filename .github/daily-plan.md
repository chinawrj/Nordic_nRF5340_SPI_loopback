# nRF5340_SPI_loopback - 每日工作计划

## 项目目标

面试技术任务 — 在 nRF5340 DK 上实现 SPIM4 32MHz SPI Loopback + BLE Heart Rate Service，交付 clean build 的 GitHub 仓库。

---

## Day N 计划模板

### 晨会计划

**日期**: YYYY-MM-DD
**迭代日**: Day N

#### 昨日回顾
- 完成: 
- 未完成: 
- 阻塞: 

#### 今日目标
1. [ ] 目标1
2. [ ] 目标2
3. [ ] 目标3

#### 风险与依赖
- 

---

### 执行记录

#### 任务1: 
- 开始时间: 
- 完成时间: 
- 构建结果: ✅/❌ (`west build` 通过?)
- 备注: 

#### 任务2: 
- 开始时间: 
- 完成时间: 
- 构建结果: ✅/❌
- 备注: 

---

### 晚间回顾

#### 完成状态
| 任务 | 状态 | 备注 |
|------|------|------|
| 任务1 | | |
| 任务2 | | |

#### 代码质量
- 新增代码行数: 
- 编译警告数: 
- Git commits: 

#### 构建验证检查点
- [ ] `west build --sysbuild` 通过
- [ ] DTS overlay 生效（spi4 配置正确）
- [ ] Kconfig 生效（SPI + BLE 启用）
- [ ] merged.hex 存在

#### 回归验证（每日必做）
- [ ] `bash verify-acceptance.sh` 全部已完成功能项 PASS
- [ ] 无回归失败（新功能未破坏已通过项）
- [ ] wrap-up commit 前回归验证已通过

#### 明日优先事项
1. 
2. 

#### 技术笔记
- 

---

## 里程碑跟踪

### M1: 项目骨架 + 环境验证
- [ ] NCS 工具链就绪
- [ ] 项目目录结构完整
- [ ] `west build --cmake-only` 通过

### M2: SPI Loopback 实现
- [ ] SPIM4 DTS overlay 完成
- [ ] spi_loopback.c/h 实现
- [ ] 独立线程运行
- [ ] 构建通过

### M3: BLE Heart Rate 实现
- [ ] BLE 协议栈配置完成
- [ ] ble_hrs.c/h 实现
- [ ] sysbuild + ipc_radio 构建通过

### M4: 集成 + 验收 + 发布
- [ ] SPI + BLE 集成
- [ ] 所有 P0 验收标准通过
- [ ] README + LLM 声明完成
- [ ] `verify-acceptance.sh` 全部 PASS
- [ ] Git 提交历史整理
- [ ] GitHub 仓库发布
- [ ] 邀请面试官
- [ ] Fresh clone test 通过

## 验收标准进度

### P0 必过
- [ ] clean build 通过
- [ ] merged.hex 存在
- [ ] GitHub 仓库可访问
- [ ] LLM 使用声明
- [ ] .gitignore 正确
- [ ] DTS/Kconfig/Sysbuild 校验通过

### P1 强烈建议
- [ ] ROM/RAM 报告正常
- [ ] 零 warning
- [ ] 代码风格一致
- [ ] Git history 干净

### P2 加分项
- [ ] bsim 编译
- [ ] Twister 集成

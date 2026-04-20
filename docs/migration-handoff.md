# Linux 迁移交接笔记

> 面向 **Day 5 及以后** 在 Linux 机器上继续本项目的自己。
> 目标：30 分钟内从零复现 Day 1–4 的所有 P0+P1 验收成果，然后专注 Day 5 的 P2 任务。

---

## 1. 当前交付状态（macOS Day 4 收尾）

| 维度 | 状态 |
|-----|------|
| NCS 版本 | v2.9.0（固定） |
| 板名 | `nrf5340dk/nrf5340/cpuapp` |
| `west build --sysbuild` | ✅ 返回 0 |
| `merged.hex` | ✅ 347 KB，覆盖 app@0x0 + net@0x01000000 |
| `-Werror` 严格构建 | ✅ 0 warning / 0 error |
| `verify-acceptance.sh` | ✅ 23/23 PASS |
| App core 占用 | FLASH 12.1%, RAM 7.2% |
| Net core (ipc_radio) | FLASH 64.4%, RAM 71.0% |
| GitHub 仓库 | `chinawrj/Nordic_nRF5340_SPI_loopback`（已 push 到 `origin/main`）|
| 面试官邀请 | ⏳ 待办，Linux 验证 + P2 加分项完成后再发 |

最近一次 commit: `feat: Linux cross-platform regression — 23/23 PASS on Ubuntu 24.04` (`2ed96fa`, Day 5)
— Day 5 在 Ubuntu 24.04 上完成跨平台首验；Day 6 更新本文档把 Linux 踩坑经验固化。

---

## 2. Linux 机器上的 bootstrap 清单（30–45 min，Ubuntu 22.04 / 24.04 实测）

> ⚠️ **Ubuntu 24.04 (noble) 注意事项**: `libmagic1` 已改名 `libmagic1t64`（apt 会自动解析）；`gcc-multilib` / `g++-multilib` 在 security 镜像常 404（安装失败可忽略 —— 仅 bsim/native_sim 需要 `-m32`，ARM target 用不到）。

```bash
# [1] 安装 system prereqs
sudo apt-get update && sudo apt-get install -y --no-install-recommends \
    git cmake ninja-build gperf ccache dfu-util device-tree-compiler \
    wget curl python3-pip python3-venv python3-dev xz-utils file make \
    libsdl2-dev
# 可选（仅 bsim/native_sim 需要）:
#   sudo apt-get install -y gcc-multilib g++-multilib

# [2] 装 nrfutil（URL 路径 2025 后已变 — 旧的 linux-amd64 路径返回 404）
mkdir -p ~/.local/bin && curl -fsSL -o ~/.local/bin/nrfutil \
    https://files.nordicsemi.com/artifactory/swtools/external/nrfutil/executables/x86_64-unknown-linux-gnu/nrfutil
chmod +x ~/.local/bin/nrfutil
export PATH="$HOME/.local/bin:$PATH"
nrfutil --version    # 期望: nrfutil 8.x

# [3] 装 NCS v2.9.0 toolchain（~5 GB，5-10 min）
nrfutil install toolchain-manager
nrfutil toolchain-manager install --ncs-version v2.9.0
nrfutil toolchain-manager list    # 应显示 * v2.9.0

# [4] 克隆 + 初始化 workspace
#     约定: workspace topdir 里的 app 仓库名为 `Nordic_nRF5340_SPI_loopback`；
#     nrf/zephyr/nrfxlib/... 会作为它的 siblings 落地
mkdir -p ~/ncs-ws && cd ~/ncs-ws
git clone git@github.com:chinawrj/Nordic_nRF5340_SPI_loopback.git

# [5] 所有后续 west 命令都放进 nrfutil launch sandbox（见 §4 坑位 2）
alias nw='nrfutil toolchain-manager launch --ncs-version v2.9.0 --shell --'
nw west init -l Nordic_nRF5340_SPI_loopback
nw west update --narrow -o=--depth=1    # 5-20 min；中断可直接再跑一次接着续
nw west zephyr-export

# [6] 构建 + 验证
cd Nordic_nRF5340_SPI_loopback
nw west build -b nrf5340dk/nrf5340/cpuapp --sysbuild
nw bash verify-acceptance.sh            # 期望: 23/23 PASS
```

---

## 3. Day 5+ 推荐 P2 任务路线

### 3.1 `nrf5340bsim` 仿真（AC-R2/R3 — 高性价比加分）

Zephyr 官方提供 `nrf5340bsim/nrf5340/cpuapp` 目标 — 基于 POSIX 架构编译成 Linux ELF，配合 BabbleSim 可端到端跑 BLE HRS 协议栈，**无需硬件**。

```bash
# AC-R2 冒烟：编译
cd ~/ncs-ws/Nordic_nRF5340_SPI_loopback
west build -b nrf5340bsim/nrf5340/cpuapp --sysbuild --build-dir build-bsim
ls build-bsim/Nordic_nRF5340_SPI_loopback/zephyr/zephyr.exe

# AC-R3 运行：需先装 BabbleSim
git clone https://github.com/BabbleSim/babble_sim.git ~/bsim && cd ~/bsim
make everything
export BSIM_OUT_PATH=~/bsim
# 然后用 Zephyr 的 `central_hr` sample 编 bsim 版本作为对端
# 具体脚本见 zephyr/tests/bsim/bluetooth/
```

**注意事项**：
- SPIM4 在 bsim 上没有硬件模型 — 应用里 SPI loopback 初始化会失败（`device_is_ready` 返回 false）。不影响 BLE 部分。可以条件编译绕过（`#if !defined(CONFIG_SOC_POSIX)`），或直接让 SPI 线程 gracefully 退出。
- BLE 协议栈完整可用 — Zephyr 的 bsim HCI 驱动 + BabbleSim 物理层足以跑真实 HRS notify。

### 3.2 Twister 集成（AC-R4）

已加入 `sample.yaml`，Linux 上可直接：
```bash
cd ~/ncs-ws
zephyr/scripts/twister -T Nordic_nRF5340_SPI_loopback \
    -p nrf5340dk/nrf5340/cpuapp --build-only
# 预期: PASSED
```

### 3.3 GitHub Actions

已加入 `.github/workflows/build.yml`，下次 push 后在 Ubuntu runner 自动跑 sysbuild + `-Werror` + `verify-acceptance.sh`。**Day 5 的第一件事：到 GitHub web UI 看 Actions tab 是否绿**。如果红 → 按错误修，因为本地 Linux 复现和 CI 复现是同一条路径。

---

## 4. 跨平台差异快查

| 项 | macOS (Day 1–4) | Linux / Ubuntu 24.04 (Day 5+) |
|---|----------------|------------------------------|
| nrfutil 安装 | `brew install nrfutil` | `curl ...x86_64-unknown-linux-gnu/nrfutil`（**旧路径 `linux-amd64` 已 404**） |
| 工具链装路径 | `/opt/nordic/ncs/toolchains/<hash>/` | `~/ncs/toolchains/<hash>/`（~5 GB） |
| `nrfutil env --as-script` + source | zsh 需写文件再 source（`eval` 进 `dquote>`） | **不推荐直接 source**：`LD_LIBRARY_PATH` 会污染系统 git/cmake（见坑 2）；推荐用 `nrfutil toolchain-manager launch --shell --` sandbox |
| bsim 支持 | ❌ 无（BabbleSim Linux-only） | ✅（需单独装 BabbleSim） |
| `native_sim` 目标 | 有限（nrf5340 模型不全） | 推荐路径 |
| apt 包名（Ubuntu 24.04 vs 22.04） | — | `libmagic1` → `libmagic1t64`；`gcc-multilib` 常 404（可跳过） |

---

## 5. 已知坑（Day 1–5 累积总结）

已全部修复并提交，但 Linux 上若出任何回归，先查这几处：

### Day 1–4（应用层与 Zephyr API 相关）

1. **sdk-nrf 路径必须叫 `nrf`** — `west.yml` 里 `path: nrf`，否则 NCS 硬编码 `${ZEPHYR_BASE}/../nrf/...` 会全线失败。
2. **`BT_LE_ADV_CONN_FAST_1` 是 Zephyr 4.x 宏** — NCS v2.9.0 = Zephyr v3.7.99-ncs2，用 `BT_LE_ADV_CONN`。
3. **`SPI_DT_SPEC_GET` 需要 child slave 节点** — 无 slave 的 loopback 场景请用 `DEVICE_DT_GET + 手动 spi_config`。
4. **`.cs = {0}` 触发 `-Werror=missing-braces`** — 直接省略，C99 会零初始化。
5. **`CONFIG_BT_HCI_IPC` 不是用户可配置** — board defconfig 通过 `chosen zephyr,bt-hci` 自动 select，勿在 prj.conf 硬写。
6. **`rom_report`/`ram_report` 必须 per-image** — sysbuild 根 dir 跑会报 unknown target，用 `-d build/Nordic_nRF5340_SPI_loopback -t rom_report`。

### Day 5（Linux / Ubuntu 24.04 环境相关）

7. **nrfutil 下载 URL 路径已变** — 旧 `.../executables/linux-amd64/nrfutil` 返回 404；新路径 `.../executables/x86_64-unknown-linux-gnu/nrfutil`。macOS 同步变成 `aarch64-apple-darwin` / `x86_64-apple-darwin`。
8. **`LD_LIBRARY_PATH` 污染系统 git/cmake（Ubuntu 24.04）** — `source /tmp/ncs_env.sh` 后，NCS 的 libcurl 链接了 `libunistring.so.2`，而 noble 只提供 `libunistring.so.5` → `git-remote-https` 启动失败；同时系统 cmake 在 NCS LD_LIBRARY_PATH 下 exit 127。**惯用法**：改用 `nrfutil toolchain-manager launch --ncs-version v2.9.0 --shell -- <cmd>` sandbox（自动隔离 env + 正确加载依赖）。若坚持手动：`unset LD_LIBRARY_PATH GIT_EXEC_PATH GIT_TEMPLATE_DIR` 后再跑 git。
9. **Per-board overlay 约定** — 顶层 `app.overlay` 对所有 board 生效；bsim/native_sim 无 `&spi4` label 时会 DTS parse fail。硬件专属节点（spi/uart/gpio）一律放 `boards/<board>.overlay`（如 `boards/nrf5340dk_nrf5340_cpuapp.overlay`），Zephyr 只在匹配 board 时 apply。
10. **bsim board 需要 BabbleSim 环境** — 仅装 NCS toolchain **不够** 跑 `west build -b nrf5340bsim/...`；必须额外 `git clone babble_sim.github.io && make everything` 并 `export BSIM_OUT_PATH` + `BSIM_COMPONENTS_PATH`。AC-R2 冒烟不含此步则 CMake 会在 `find_package(bsim)` 报错。

---

## 6. 面试官邀请 checklist（Day 5 末/Day 6）

在 Linux 上验证完 CI+bsim 之后再做：

- [ ] GitHub Actions 已绿（main 分支最新 commit）
- [ ] 本地 `bash verify-acceptance.sh` 23/23
- [ ] Linux 上可复现 bsim/native_sim（至少 AC-R2 编译通过）
- [ ] README 顶部 status 行更新为带 CI badge
- [ ] `gh repo edit --add-topic nrf5340 --add-topic zephyr --add-topic ble`
- [ ] Settings → Collaborators → Add interviewer email
- [ ] 给面试官发确认邮件，附 repo URL + 说明"clean build 可 `west build` 一键复现，`bash verify-acceptance.sh` 23 条断言"

---

## 7. 快速联系点

- **REQUIREMENTS.md** — 原始需求 + 验收标准全表
- **docs/daily-logs/** — 逐日进度与坑记录
- **docs/skill-feedback.md** — 工作流反馈（如有）
- **verify-acceptance.sh** — P0 的 single source of truth

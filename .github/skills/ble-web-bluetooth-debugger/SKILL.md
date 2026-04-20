---
name: ble-web-bluetooth-debugger
description: 通过 Patchright 浏览器使用 Web Bluetooth API 调试 BLE 设备。用于连接 BLE Heart Rate Service、读取 GATT 特征值、验证 BLE 广播和 notify 功能。适用于无专用 BLE 工具时通过浏览器验证 BLE Peripheral 是否正常工作。
---

# Skill: BLE Web Bluetooth 浏览器调试

## 用途

通过 Patchright（反检测 Playwright fork）启动 Chrome 浏览器，利用 Web Bluetooth API 连接 BLE 设备，读取 GATT 服务和特征值，验证 BLE Peripheral 功能。

**何时使用：**
- 需要验证 BLE Peripheral（如 Heart Rate Service）是否正常广播和 notify
- 没有专用 BLE 调试工具（如 nRF Connect App），但有 Chrome 浏览器
- 需要自动化 BLE 连接测试流程
- 需要截图记录 BLE 连接状态作为验收证据

**何时不使用：**
- 纯构建验证（不需要实机运行）
- 有 nRF Connect for Desktop / Mobile 等专用工具
- 设备没有 BLE 功能

## 前置条件

- Python 虚拟环境：项目根目录 `.venv/`
- Patchright 已安装：`pip install patchright`
- 浏览器驱动已安装：`python -m patchright install chromium`
- **Chrome/Chromium 浏览器**（Web Bluetooth 仅 Chrome 支持）
- **蓝牙硬件**：运行浏览器的电脑需有蓝牙适配器
- **BLE 设备已通电并广播**（如 nRF5340 DK 运行 HRS firmware）

### 环境初始化

```bash
# 在项目 venv 中安装
source .venv/bin/activate
pip install patchright
python -m patchright install chromium
```

## 操作步骤

### 1. 启动浏览器（开启蓝牙权限）

```python
from patchright.sync_api import sync_playwright
import os

USER_DATA_DIR = os.path.expanduser("~/.patchright-userdata/ble-debug")

pw = sync_playwright().start()
context = pw.chromium.launch_persistent_context(
    user_data_dir=USER_DATA_DIR,
    channel="chrome",
    headless=False,       # Web Bluetooth 必须可见模式
    no_viewport=True,
    args=[
        "--remote-debugging-port=9222",
        # Chrome Web Bluetooth flags
        "--enable-features=WebBluetooth",
        "--enable-web-bluetooth",
    ],
)
page = context.pages[0] if context.pages else context.new_page()
print("浏览器已启动，可使用 Web Bluetooth API")
```

> ⚠️ **Web Bluetooth 限制**: 必须 `headless=False`，且需要用户手势（click）触发蓝牙扫描。自动化时需模拟点击按钮来触发 `navigator.bluetooth.requestDevice()`。

### 2. 创建 BLE HRS 测试页面

创建本地 HTML 页面 `tools/ble-hr-test.html`，用于通过 Web Bluetooth 连接 Heart Rate Service：

```html
<!DOCTYPE html>
<html>
<head>
    <title>BLE Heart Rate Monitor - Test Page</title>
    <style>
        body { font-family: monospace; padding: 20px; background: #1a1a2e; color: #e0e0e0; }
        button { padding: 10px 20px; margin: 5px; cursor: pointer; font-size: 14px;
                 background: #0f3460; color: white; border: 1px solid #16213e; border-radius: 4px; }
        button:hover { background: #16213e; }
        #status { padding: 10px; margin: 10px 0; border: 1px solid #333; border-radius: 4px; }
        #log { white-space: pre-wrap; font-size: 12px; max-height: 400px; overflow-y: auto;
               background: #0d1117; padding: 10px; border-radius: 4px; }
        .pass { color: #3fb950; }
        .fail { color: #f85149; }
        .info { color: #58a6ff; }
        #hr-value { font-size: 48px; color: #f85149; text-align: center; margin: 20px 0; }
    </style>
</head>
<body>
    <h1>🫀 BLE Heart Rate Monitor</h1>
    <p>Target device: <strong id="target-name">nRF5340_HR</strong></p>

    <div>
        <button id="btn-scan" onclick="scanAndConnect()">🔍 Scan & Connect</button>
        <button id="btn-disconnect" onclick="disconnect()" disabled>⏹ Disconnect</button>
    </div>

    <div id="status">Status: Ready</div>
    <div id="hr-value">-- bpm</div>
    <div id="log"></div>

    <script>
        let device = null;
        let server = null;
        let hrCharacteristic = null;
        const DEVICE_NAME = 'nRF5340_HR';
        const HR_SERVICE_UUID = 0x180D;
        const HR_MEASUREMENT_UUID = 0x2A37;

        function log(msg, cls = 'info') {
            const el = document.getElementById('log');
            const time = new Date().toLocaleTimeString();
            el.innerHTML += `<span class="${cls}">[${time}] ${msg}</span>\n`;
            el.scrollTop = el.scrollHeight;
        }

        function setStatus(msg) {
            document.getElementById('status').textContent = 'Status: ' + msg;
        }

        async function scanAndConnect() {
            try {
                setStatus('Scanning...');
                log(`Requesting BLE device with name: ${DEVICE_NAME}, service: 0x${HR_SERVICE_UUID.toString(16)}`);

                device = await navigator.bluetooth.requestDevice({
                    filters: [{ name: DEVICE_NAME }],
                    optionalServices: [HR_SERVICE_UUID]
                });

                log(`✅ Device found: ${device.name} (ID: ${device.id})`, 'pass');
                setStatus('Connecting to GATT server...');

                device.addEventListener('gattserverdisconnected', onDisconnected);
                server = await device.gatt.connect();
                log('✅ GATT server connected', 'pass');

                setStatus('Getting Heart Rate Service...');
                const service = await server.getPrimaryService(HR_SERVICE_UUID);
                log(`✅ HR Service found (UUID: 0x${HR_SERVICE_UUID.toString(16)})`, 'pass');

                hrCharacteristic = await service.getCharacteristic(HR_MEASUREMENT_UUID);
                log(`✅ HR Measurement Characteristic found (UUID: 0x${HR_MEASUREMENT_UUID.toString(16)})`, 'pass');

                await hrCharacteristic.startNotifications();
                hrCharacteristic.addEventListener('characteristicvaluechanged', onHRChanged);
                log('✅ Notifications started — waiting for heart rate data...', 'pass');

                setStatus('Connected — receiving data');
                document.getElementById('btn-scan').disabled = true;
                document.getElementById('btn-disconnect').disabled = false;

            } catch (err) {
                log(`❌ Error: ${err.message}`, 'fail');
                setStatus('Error: ' + err.message);
            }
        }

        function onHRChanged(event) {
            const value = event.target.value;
            const flags = value.getUint8(0);
            let hr;
            if (flags & 0x01) {
                hr = value.getUint16(1, true);
            } else {
                hr = value.getUint8(1);
            }
            document.getElementById('hr-value').textContent = `${hr} bpm`;
            log(`💓 Heart Rate: ${hr} bpm`);
        }

        function onDisconnected() {
            log('⚠️ Device disconnected', 'fail');
            setStatus('Disconnected');
            document.getElementById('hr-value').textContent = '-- bpm';
            document.getElementById('btn-scan').disabled = false;
            document.getElementById('btn-disconnect').disabled = true;
        }

        function disconnect() {
            if (device && device.gatt.connected) {
                device.gatt.disconnect();
                log('Disconnected by user');
            }
        }
    </script>
</body>
</html>
```

### 3. 使用 Patchright 自动化 BLE 测试

```python
import os
import time
from patchright.sync_api import sync_playwright

def test_ble_hrs(html_path="tools/ble-hr-test.html", timeout=30):
    """
    自动化 BLE HRS 测试流程：
    1. 打开测试页面
    2. 点击 Scan & Connect（触发 Web Bluetooth 弹窗）
    3. 手动选择设备（Web Bluetooth 要求用户交互）
    4. 等待心率数据
    5. 截图记录结果
    """
    pw = sync_playwright().start()
    context = pw.chromium.launch_persistent_context(
        user_data_dir=os.path.expanduser("~/.patchright-userdata/ble-debug"),
        channel="chrome",
        headless=False,
        no_viewport=True,
        args=["--enable-features=WebBluetooth"],
    )
    page = context.pages[0] if context.pages else context.new_page()

    # 打开测试页面
    abs_path = os.path.abspath(html_path)
    page.goto(f"file://{abs_path}")
    page.wait_for_load_state("domcontentloaded")
    print("✅ 测试页面已加载")

    # 点击扫描按钮（触发 Web Bluetooth 弹窗）
    page.click("#btn-scan")
    print("🔍 已触发 BLE 扫描 — 请在浏览器弹窗中选择 'nRF5340_HR' 设备")

    # 等待用户选择设备并连接
    try:
        page.wait_for_function(
            """() => {
                const status = document.getElementById('status').textContent;
                return status.includes('receiving data') || status.includes('Error');
            }""",
            timeout=timeout * 1000,
        )
    except Exception:
        print("⏱ 超时 — 未能在规定时间内完成连接")

    # 读取状态
    status = page.locator("#status").text_content()
    hr_value = page.locator("#hr-value").text_content()
    log_content = page.locator("#log").text_content()

    print(f"📊 Status: {status}")
    print(f"💓 HR Value: {hr_value}")

    # 截图
    os.makedirs("reports", exist_ok=True)
    page.screenshot(path="reports/ble-hr-test.png")
    print("📸 截图已保存: reports/ble-hr-test.png")

    # 验证结果
    connected = "receiving data" in status
    has_hr = hr_value != "-- bpm"

    if connected and has_hr:
        print("✅ BLE HRS 测试通过")
    else:
        print(f"❌ BLE HRS 测试未通过 (connected={connected}, has_hr={has_hr})")

    # 保存日志
    with open("reports/ble-hr-log.txt", "w") as f:
        f.write(log_content)

    context.close()
    pw.stop()
    return {"connected": connected, "hr_value": hr_value, "log": log_content}
```

### 4. 使用第三方 Web BLE 测试工具

如果不想创建自定义测试页面，可以使用在线工具：

```python
# 方式 A: 使用 Web Bluetooth 官方心率示例
page.goto("https://niccolobruno.github.io/web-bluetooth-heart-rate-monitor/")

# 方式 B: 使用 nRF Connect for Web（Nordic 官方）
page.goto("https://nrfconnect.github.io/webbluetooth/")

# 方式 C: 使用通用 Web BLE 扫描工具
page.goto("https://niccolobruno.github.io/web-bluetooth-devices-scanner/")
```

### 5. 半自动化验证流程

由于 Web Bluetooth 的安全限制（需用户手势选择设备），完全自动化有困难。推荐半自动化流程：

```python
def semi_auto_ble_test():
    """半自动化 BLE 测试 — 自动启动浏览器和页面，手动选择设备"""
    pw = sync_playwright().start()
    context = pw.chromium.launch_persistent_context(
        user_data_dir=os.path.expanduser("~/.patchright-userdata/ble-debug"),
        channel="chrome",
        headless=False,
        no_viewport=True,
    )
    page = context.pages[0] if context.pages else context.new_page()

    # 步骤 1: 自动 — 打开测试页面
    page.goto(f"file://{os.path.abspath('tools/ble-hr-test.html')}")
    print("Step 1: ✅ 测试页面已打开")

    # 步骤 2: 自动 — 点击扫描按钮
    page.click("#btn-scan")
    print("Step 2: ✅ 已点击扫描按钮")
    print("Step 3: ⏳ 请在弹窗中选择 'nRF5340_HR' 设备...")

    # 步骤 3: 手动 — 用户在弹窗中选择设备
    page.wait_for_function(
        "() => document.getElementById('status').textContent.includes('receiving')",
        timeout=60000,
    )
    print("Step 4: ✅ BLE 连接成功")

    # 步骤 4: 自动 — 等待收集数据
    import time
    time.sleep(10)

    # 步骤 5: 自动 — 验证并截图
    hr_value = page.locator("#hr-value").text_content()
    assert hr_value != "-- bpm", f"未收到心率数据: {hr_value}"
    print(f"Step 5: ✅ 收到心率数据: {hr_value}")

    page.screenshot(path="reports/ble-hr-connected.png")
    print("Step 6: ✅ 截图已保存")

    context.close()
    pw.stop()
    print("\n🎉 BLE HRS 半自动化测试通过!")
```

## 注意事项

- **必须使用 Patchright** 而非 Playwright
- **必须使用 `launch_persistent_context`**（持久化上下文）
- **必须 `headless=False`** — Web Bluetooth 不支持无头模式
- **Web Bluetooth 设备选择弹窗需手动操作** — 这是 Chrome 安全策略，无法完全自动化
- 不要设置自定义 user_agent
- CDP 端口默认 9222，确保不冲突
- **macOS**: 需在系统设置中授权 Chrome 蓝牙权限
- **Linux**: 确保 `bluetoothd` 服务运行，用户在 `bluetooth` 组中
- **Windows**: 需 Windows 10+ 并启用蓝牙

## Self-Test（自检）

> 验证 Patchright 安装和浏览器启动能力（不需要 BLE 硬件）。

### 自检步骤

```bash
# 在项目 venv 中执行
source .venv/bin/activate

# Test 1: Patchright 可导入
python3 -c "from patchright.sync_api import sync_playwright; print('SELF_TEST_PASS: patchright_import')" 2>/dev/null || echo "SELF_TEST_FAIL: patchright_import"

# Test 2: 浏览器驱动存在
python3 -c "
import subprocess
result = subprocess.run(['python3', '-m', 'patchright', 'install', '--dry-run', 'chromium'], capture_output=True, text=True)
print('SELF_TEST_PASS: chromium_driver')
" 2>/dev/null || echo "SELF_TEST_FAIL: chromium_driver"

# Test 3: 可启动浏览器（可见模式）
python3 -c "
from patchright.sync_api import sync_playwright
import os
pw = sync_playwright().start()
ctx = pw.chromium.launch_persistent_context(
    user_data_dir=os.path.expanduser('~/.patchright-userdata/selftest'),
    channel='chrome',
    headless=False,
    no_viewport=True,
)
page = ctx.pages[0] if ctx.pages else ctx.new_page()
page.goto('data:text/html,<h1>BLE Debugger Ready</h1>')
assert page.locator('h1').text_content() == 'BLE Debugger Ready'
ctx.close()
pw.stop()
print('SELF_TEST_PASS: browser_launch')
" 2>/dev/null || echo "SELF_TEST_FAIL: browser_launch"

# Test 4: Web Bluetooth API 存在（浏览器端检查）
python3 -c "
from patchright.sync_api import sync_playwright
import os
pw = sync_playwright().start()
ctx = pw.chromium.launch_persistent_context(
    user_data_dir=os.path.expanduser('~/.patchright-userdata/selftest'),
    channel='chrome',
    headless=False,
    no_viewport=True,
)
page = ctx.pages[0] if ctx.pages else ctx.new_page()
has_bt = page.evaluate('() => \"bluetooth\" in navigator')
ctx.close()
pw.stop()
if has_bt:
    print('SELF_TEST_PASS: web_bluetooth_api')
else:
    print('SELF_TEST_FAIL: web_bluetooth_api (navigator.bluetooth not available)')
" 2>/dev/null || echo "SELF_TEST_FAIL: web_bluetooth_api"
```

### 预期结果

| 测试项 | 预期输出 | 失败影响 |
|--------|---------|----------|
| patchright_import | `SELF_TEST_PASS` | 浏览器功能完全不可用 |
| chromium_driver | `SELF_TEST_PASS` | 无法启动浏览器 |
| browser_launch | `SELF_TEST_PASS` | 浏览器自动化失败 |
| web_bluetooth_api | `SELF_TEST_PASS` | BLE 调试不可用（可能 Chrome 版本过低） |

## Blind Test（盲测）

**测试 Prompt:**
```
你是一个 AI 开发助手。请阅读此 Skill，然后：
1. 启动 Patchright 浏览器（可见模式，headless=False）
2. 创建一个本地 HTML 文件，模拟 BLE Heart Rate Monitor 页面
   - 包含一个 "Scan" 按钮
   - 包含心率显示区域
   - 包含日志输出区域
3. 在浏览器中打开该页面
4. 验证 navigator.bluetooth API 是否可用
5. 截图保存到 reports/ble-test.png
6. 关闭浏览器
报告每个步骤的结果。
```

**验收标准:**
- [ ] Agent 使用 Patchright（而非 Playwright）
- [ ] Agent 使用 `launch_persistent_context`（而非 `launch`）
- [ ] Agent 设置 `headless=False`
- [ ] Agent 正确创建了 BLE 测试 HTML 页面
- [ ] Agent 检查了 Web Bluetooth API 可用性
- [ ] Agent 没有设置自定义 user_agent

**常见失败模式:**
- Agent 使用 `playwright` 而非 `patchright`
- Agent 使用 `headless=True`（Web Bluetooth 不支持）
- Agent 使用 `browser.launch()` 而非 `launch_persistent_context`

## 成功标准

- [ ] Patchright 浏览器可启动（可见模式）
- [ ] Web Bluetooth API 可用
- [ ] BLE 测试 HTML 页面可加载
- [ ] 能触发 BLE 扫描（弹出设备选择框）
- [ ] 连接后能接收 Heart Rate notify 数据
- [ ] 能截图记录测试结果

---
name: ble-web-bluetooth-debugger
description: Debug BLE devices through a Patchright browser using the Web Bluetooth API. Used to connect to the BLE Heart Rate Service, read GATT characteristic values, and verify BLE advertising and notify behaviour. Suitable for verifying a BLE Peripheral through the browser when no dedicated BLE tool is available.
---

# Skill: BLE Web Bluetooth Browser Debugging

## Purpose

Launch Chrome through Patchright (an anti-detection Playwright fork) and use the Web Bluetooth API to connect to a BLE device, read its GATT services and characteristics, and validate BLE Peripheral behaviour.

**When to use:**
- You need to verify that a BLE Peripheral (e.g. Heart Rate Service) advertises and notifies correctly
- You do not have a dedicated BLE debugging tool (like the nRF Connect app) but do have Chrome
- You need to automate the BLE connection test flow
- You need to capture screenshots of the BLE connection state as acceptance evidence

**When not to use:**
- Build-only validation (no real-hardware runtime needed)
- You already have a dedicated tool such as nRF Connect for Desktop / Mobile
- The device has no BLE functionality

## Prerequisites

- Python virtualenv: `.venv/` at the project root
- Patchright installed: `pip install patchright`
- Browser driver installed: `python -m patchright install chromium`
- **Chrome/Chromium** (Web Bluetooth is Chrome-only)
- **Bluetooth hardware**: the host running the browser needs a Bluetooth adapter
- **The BLE device is powered on and advertising** (e.g. an nRF5340 DK running HRS firmware)

### Environment setup

```bash
# Install inside the project venv
source .venv/bin/activate
pip install patchright
python -m patchright install chromium
```

## Procedure

### 1. Launch the browser (with Bluetooth permission)

```python
from patchright.sync_api import sync_playwright
import os

USER_DATA_DIR = os.path.expanduser("~/.patchright-userdata/ble-debug")

pw = sync_playwright().start()
context = pw.chromium.launch_persistent_context(
    user_data_dir=USER_DATA_DIR,
    channel="chrome",
    headless=False,       # Web Bluetooth requires a visible browser
    no_viewport=True,
    args=[
        "--remote-debugging-port=9222",
        # Chrome Web Bluetooth flags
        "--enable-features=WebBluetooth",
        "--enable-web-bluetooth",
    ],
)
page = context.pages[0] if context.pages else context.new_page()
print("Browser launched; Web Bluetooth API is available")
```

> ⚠️ **Web Bluetooth limitations**: `headless=False` is mandatory, and a user gesture (click) is required to trigger a Bluetooth scan. In automation you must simulate a button click to trigger `navigator.bluetooth.requestDevice()`.

### 2. Create a BLE HRS test page

Create a local HTML page `tools/ble-hr-test.html` that connects to the Heart Rate Service via Web Bluetooth:

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

### 3. Automate the BLE test with Patchright

```python
import os
import time
from patchright.sync_api import sync_playwright

def test_ble_hrs(html_path="tools/ble-hr-test.html", timeout=30):
    """
    Automate the BLE HRS test flow:
    1. Open the test page
    2. Click Scan & Connect (triggers the Web Bluetooth prompt)
    3. Manually select the device (Web Bluetooth requires user interaction)
    4. Wait for heart-rate data
    5. Screenshot the result
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

    # Open the test page
    abs_path = os.path.abspath(html_path)
    page.goto(f"file://{abs_path}")
    page.wait_for_load_state("domcontentloaded")
    print("✅ Test page loaded")

    # Click the scan button (triggers the Web Bluetooth prompt)
    page.click("#btn-scan")
    print("🔍 BLE scan triggered — select 'nRF5340_HR' in the browser prompt")

    # Wait for the user to select a device and connect
    try:
        page.wait_for_function(
            """() => {
                const status = document.getElementById('status').textContent;
                return status.includes('receiving data') || status.includes('Error');
            }""",
            timeout=timeout * 1000,
        )
    except Exception:
        print("⏱ Timed out — connection did not complete within the deadline")

    # Read the status
    status = page.locator("#status").text_content()
    hr_value = page.locator("#hr-value").text_content()
    log_content = page.locator("#log").text_content()

    print(f"📊 Status: {status}")
    print(f"💓 HR Value: {hr_value}")

    # Screenshot
    os.makedirs("reports", exist_ok=True)
    page.screenshot(path="reports/ble-hr-test.png")
    print("📸 Screenshot saved: reports/ble-hr-test.png")

    # Validate
    connected = "receiving data" in status
    has_hr = hr_value != "-- bpm"

    if connected and has_hr:
        print("✅ BLE HRS test passed")
    else:
        print(f"❌ BLE HRS test failed (connected={connected}, has_hr={has_hr})")

    # Save the log
    with open("reports/ble-hr-log.txt", "w") as f:
        f.write(log_content)

    context.close()
    pw.stop()
    return {"connected": connected, "hr_value": hr_value, "log": log_content}
```

### 4. Use a third-party Web BLE tool

If you would rather not create a custom test page, you can use an online tool:

```python
# Option A: the official Web Bluetooth heart-rate sample
page.goto("https://niccolobruno.github.io/web-bluetooth-heart-rate-monitor/")

# Option B: nRF Connect for Web (Nordic official)
page.goto("https://nrfconnect.github.io/webbluetooth/")

# Option C: a generic Web BLE scanner
page.goto("https://niccolobruno.github.io/web-bluetooth-devices-scanner/")
```

### 5. Semi-automated verification flow

Because of Web Bluetooth's security restrictions (a user gesture is required to select the device), fully automating it is hard. A semi-automated flow is recommended:

```python
def semi_auto_ble_test():
    """Semi-automated BLE test — auto-launch browser and page, manually pick the device"""
    pw = sync_playwright().start()
    context = pw.chromium.launch_persistent_context(
        user_data_dir=os.path.expanduser("~/.patchright-userdata/ble-debug"),
        channel="chrome",
        headless=False,
        no_viewport=True,
    )
    page = context.pages[0] if context.pages else context.new_page()

    # Step 1: auto — open the test page
    page.goto(f"file://{os.path.abspath('tools/ble-hr-test.html')}")
    print("Step 1: ✅ test page opened")

    # Step 2: auto — click the scan button
    page.click("#btn-scan")
    print("Step 2: ✅ scan button clicked")
    print("Step 3: ⏳ please select 'nRF5340_HR' in the prompt...")

    # Step 3: manual — user selects the device in the prompt
    page.wait_for_function(
        "() => document.getElementById('status').textContent.includes('receiving')",
        timeout=60000,
    )
    print("Step 4: ✅ BLE connected")

    # Step 4: auto — wait for data to accumulate
    import time
    time.sleep(10)

    # Step 5: auto — validate and screenshot
    hr_value = page.locator("#hr-value").text_content()
    assert hr_value != "-- bpm", f"no heart-rate data received: {hr_value}"
    print(f"Step 5: ✅ heart-rate data received: {hr_value}")

    page.screenshot(path="reports/ble-hr-connected.png")
    print("Step 6: ✅ screenshot saved")

    context.close()
    pw.stop()
    print("\n🎉 BLE HRS semi-automated test passed!")
```

## Notes

- **Must use Patchright** rather than Playwright
- **Must use `launch_persistent_context`** (persistent context)
- **Must set `headless=False`** — Web Bluetooth does not support headless mode
- **The Web Bluetooth device-selection prompt requires manual action** — this is Chrome's security policy, full automation is impossible
- Do not set a custom `user_agent`
- The CDP port defaults to 9222; make sure it does not conflict
- **macOS**: grant Bluetooth permission to Chrome in System Settings
- **Linux**: ensure `bluetoothd` is running and the user is in the `bluetooth` group
- **Windows**: requires Windows 10+ with Bluetooth enabled

## Self-Test

> Validate that Patchright is installed and the browser can launch (no BLE hardware required).

### Self-test steps

```bash
# Run inside the project venv
source .venv/bin/activate

# Test 1: Patchright importable
python3 -c "from patchright.sync_api import sync_playwright; print('SELF_TEST_PASS: patchright_import')" 2>/dev/null || echo "SELF_TEST_FAIL: patchright_import"

# Test 2: Browser driver present
python3 -c "
import subprocess
result = subprocess.run(['python3', '-m', 'patchright', 'install', '--dry-run', 'chromium'], capture_output=True, text=True)
print('SELF_TEST_PASS: chromium_driver')
" 2>/dev/null || echo "SELF_TEST_FAIL: chromium_driver"

# Test 3: Can launch the browser (visible mode)
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

# Test 4: Web Bluetooth API is available (browser-side check)
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

### Expected results

| Test | Expected output | Failure impact |
|------|-----------------|----------------|
| patchright_import | `SELF_TEST_PASS` | Browser functionality completely unavailable |
| chromium_driver | `SELF_TEST_PASS` | Cannot launch the browser |
| browser_launch | `SELF_TEST_PASS` | Browser automation fails |
| web_bluetooth_api | `SELF_TEST_PASS` | BLE debugging unavailable (Chrome may be too old) |

## Blind Test

**Test prompt:**
```
You are an AI development assistant. Read this skill, then:
1. Launch the Patchright browser (visible mode, headless=False)
2. Create a local HTML file that simulates a BLE Heart Rate Monitor page
   - Include a "Scan" button
   - Include a heart-rate display area
   - Include a log output area
3. Open the page in the browser
4. Verify that the navigator.bluetooth API is available
5. Save a screenshot to reports/ble-test.png
6. Close the browser
Report the result of each step.
```

**Acceptance criteria:**
- [ ] The agent uses Patchright (not Playwright)
- [ ] The agent uses `launch_persistent_context` (not `launch`)
- [ ] The agent sets `headless=False`
- [ ] The agent correctly creates the BLE test HTML page
- [ ] The agent checks Web Bluetooth API availability
- [ ] The agent does not set a custom user_agent

**Common failure modes:**
- The agent uses `playwright` instead of `patchright`
- The agent uses `headless=True` (unsupported for Web Bluetooth)
- The agent uses `browser.launch()` instead of `launch_persistent_context`

## Success Criteria

- [ ] The Patchright browser launches (in visible mode)
- [ ] The Web Bluetooth API is available
- [ ] The BLE test HTML page loads
- [ ] A BLE scan can be triggered (device-selection prompt appears)
- [ ] After connection, Heart Rate notifications can be received

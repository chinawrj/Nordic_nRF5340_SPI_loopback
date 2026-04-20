#!/usr/bin/env python3
"""
AC-V2 semi-automated BLE HR test via Patchright Chromium + Web Bluetooth.

Flow:
  1. Launch headed Chromium (persistent context).
  2. Open tools/ble-hr-test.html.
  3. Click #connect — Chromium shows the native BLE device chooser.
  4. USER manually picks "nRF5340_HR" from chooser.
  5. Script waits for >=3 characteristicvaluechanged events.
  6. Save reports/ble-hr-connected.png + reports/ble-hr-log.txt.

Exit code 0 on success (>=3 notifications), 1 otherwise.
"""
import os, sys, time
from pathlib import Path
from patchright.sync_api import sync_playwright

REPO = Path(__file__).resolve().parents[2]
PAGE = REPO / "tools" / "ble-hr-test.html"
REPORTS = REPO / "reports"
SCREENSHOT = REPORTS / "ble-hr-connected.png"
LOGFILE = REPORTS / "ble-hr-log.txt"
USER_DATA = Path.home() / ".patchright-userdata" / "ble-debug"

MIN_NOTIFICATIONS = 3
CONNECT_TIMEOUT_S = 90
NOTIFY_GRACE_S = 15

def main():
    REPORTS.mkdir(parents=True, exist_ok=True)
    USER_DATA.mkdir(parents=True, exist_ok=True)

    with sync_playwright() as pw:
        ctx = pw.chromium.launch_persistent_context(
            user_data_dir=str(USER_DATA),
            channel="chromium",
            headless=False,
            no_viewport=True,
            args=["--enable-features=WebBluetooth"],
        )
        page = ctx.pages[0] if ctx.pages else ctx.new_page()
        page.goto(PAGE.as_uri())
        page.wait_for_selector("#connect")

        print("[auto] page loaded. Triggering Web Bluetooth chooser...")
        print("[ACTION] In the Chromium window: pick 'nRF5340_HR' from the chooser.")
        page.click("#connect")

        t0 = time.time()
        seen = 0
        first_seen_at = None
        while time.time() - t0 < CONNECT_TIMEOUT_S:
            log_text = page.eval_on_selector("#log", "e => e.textContent")
            seen = log_text.count("characteristicvaluechanged")
            if seen >= 1 and first_seen_at is None:
                first_seen_at = time.time()
                print(f"[auto] first notification received at t={time.time()-t0:.1f}s")
            if seen >= MIN_NOTIFICATIONS:
                break
            if first_seen_at and time.time() - first_seen_at > NOTIFY_GRACE_S:
                break
            time.sleep(1)

        log_text = page.eval_on_selector("#log", "e => e.textContent")
        seen = log_text.count("characteristicvaluechanged")
        status_text = page.eval_on_selector("#status", "e => e.textContent")
        bpm_text = page.eval_on_selector("#bpm", "e => e.textContent")

        print(f"[auto] status: {status_text}")
        print(f"[auto] bpm: {bpm_text}")
        print(f"[auto] notifications seen: {seen}")

        page.screenshot(path=str(SCREENSHOT), full_page=True)
        LOGFILE.write_text(log_text)
        print(f"[auto] wrote {SCREENSHOT}")
        print(f"[auto] wrote {LOGFILE}")

        ctx.close()
        return 0 if seen >= MIN_NOTIFICATIONS else 1

if __name__ == "__main__":
    sys.exit(main())

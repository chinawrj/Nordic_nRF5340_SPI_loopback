/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * BLE Heart Rate peripheral for nRF5340 (app core hosts BT host; net core
 * runs the controller via IPC -- see sysbuild.conf).
 */

#include "ble_hrs.h"

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/services/hrs.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(ble_hrs, LOG_LEVEL_INF);

#define HR_NOTIFY_PERIOD_MS 1000

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL,
		      BT_UUID_16_ENCODE(BT_UUID_HRS_VAL),
		      BT_UUID_16_ENCODE(BT_UUID_DIS_VAL)),
};

static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME,
		sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

static struct bt_conn *current_conn;

static K_THREAD_STACK_DEFINE(hr_stack, 1024);
static struct k_thread hr_thread_data;

static void start_advertising(void)
{
	int err = bt_le_adv_start(BT_LE_ADV_CONN,
				  ad, ARRAY_SIZE(ad),
				  sd, ARRAY_SIZE(sd));
	if (err) {
		LOG_ERR("bt_le_adv_start failed: %d", err);
		return;
	}
	LOG_INF("advertising as \"%s\"", CONFIG_BT_DEVICE_NAME);
}

static void connected_cb(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		LOG_WRN("connection failed (err 0x%02x)", err);
		return;
	}
	current_conn = bt_conn_ref(conn);
	LOG_INF("connected");
}

static void disconnected_cb(struct bt_conn *conn, uint8_t reason)
{
	LOG_INF("disconnected (reason 0x%02x)", reason);
	if (current_conn) {
		bt_conn_unref(current_conn);
		current_conn = NULL;
	}
	start_advertising();
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected_cb,
	.disconnected = disconnected_cb,
};

static uint16_t simulate_heart_rate(uint32_t tick)
{
	/* Triangle wave 60 -> 100 -> 60 bpm, 1 bpm per second. */
	uint32_t phase = tick % 80U;
	return (uint16_t)(60U + (phase < 40U ? phase : 80U - phase));
}

static void hr_notify_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1); ARG_UNUSED(p2); ARG_UNUSED(p3);

	uint32_t tick = 0;

	while (1) {
		k_msleep(HR_NOTIFY_PERIOD_MS);
		if (current_conn == NULL) {
			continue;
		}
		uint16_t hr = simulate_heart_rate(tick++);
		int err = bt_hrs_notify(hr);
		if (err) {
			LOG_WRN("bt_hrs_notify failed: %d", err);
		} else {
			LOG_DBG("hr = %u bpm", hr);
		}
	}
}

int ble_hrs_start(void)
{
	int err = bt_enable(NULL);

	if (err) {
		LOG_ERR("bt_enable failed: %d", err);
		return err;
	}
	LOG_INF("BLE stack enabled");

	start_advertising();

	k_tid_t tid = k_thread_create(&hr_thread_data, hr_stack,
				      K_THREAD_STACK_SIZEOF(hr_stack),
				      hr_notify_thread,
				      NULL, NULL, NULL,
				      K_PRIO_PREEMPT(8), 0, K_NO_WAIT);
	if (tid == NULL) {
		return -ENOMEM;
	}
	k_thread_name_set(tid, "hr_notify");
	return 0;
}

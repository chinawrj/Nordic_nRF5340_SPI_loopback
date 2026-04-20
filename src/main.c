/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * nRF5340 SPI Loopback + BLE Heart Rate — entry point.
 *
 * Responsibilities:
 *   - Bring up the BLE stack + HRS peripheral (ble_hrs module)
 *   - Spawn the SPIM4 32 MHz loopback self-test thread (spi_loopback module)
 *   - Keep main() minimal; all real work lives in the sub-modules.
 */

#include "ble_hrs.h"
#include "spi_loopback.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

int main(void)
{
	LOG_INF("nRF5340 SPI loopback + BLE HR starting");

	int err = ble_hrs_start();

	if (err) {
		LOG_ERR("ble_hrs_start failed: %d", err);
		return err;
	}

	err = spi_loopback_start();
	if (err) {
		LOG_ERR("spi_loopback_start failed: %d", err);
		return err;
	}

	LOG_INF("init complete");
	return 0;
}

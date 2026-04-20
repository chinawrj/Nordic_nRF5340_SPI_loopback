/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * SPIM4 32 MHz loopback test — sends a 32-byte incrementing pattern and
 * compares RX vs TX. Runs as an independent cooperative thread so it does
 * not block the BLE host.
 *
 * Implementation note:
 *   `SPI_DT_SPEC_GET` requires a declared child slave node on the bus.
 *   Our loopback has no real peripheral (MOSI is jumpered to MISO), so we
 *   grab the bus controller with `DEVICE_DT_GET` and build our own
 *   `struct spi_config`, reading the 32 MHz frequency from DT.
 */

#include "spi_loopback.h"

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <string.h>

LOG_MODULE_REGISTER(spi_loopback, LOG_LEVEL_INF);

#define SPI_NODE        DT_NODELABEL(spi4)

/*
 * The `nrf5340bsim` simulator board has no SPIM4 peripheral, so the
 * `spi4` node does not exist in its devicetree. Compile the loopback
 * thread only on boards that actually provide the bus; on bsim,
 * `spi_loopback_start()` becomes a no-op so the BLE half of the app
 * still builds and runs under the simulator (covers P2 AC-R2).
 */
#if DT_NODE_EXISTS(SPI_NODE) && DT_NODE_HAS_STATUS(SPI_NODE, okay)

#define SPI_FREQ_HZ     DT_PROP(SPI_NODE, clock_frequency)
#define TEST_BUF_LEN    32U
#define TEST_PERIOD_MS  1000

static const struct device *const spi_dev = DEVICE_DT_GET(SPI_NODE);

static const struct spi_config spi_cfg = {
	.frequency = SPI_FREQ_HZ,
	.operation = SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB | SPI_WORD_SET(8),
	.slave     = 0,
	/* .cs left zero-initialized: no CS line is driven in wire loopback. */
};

static K_THREAD_STACK_DEFINE(spi_stack, 1536);
static struct k_thread spi_thread_data;

static int run_one_transfer(uint32_t iteration)
{
	uint8_t tx_buf[TEST_BUF_LEN];
	uint8_t rx_buf[TEST_BUF_LEN];

	for (size_t i = 0; i < TEST_BUF_LEN; i++) {
		tx_buf[i] = (uint8_t)(iteration + i);
	}
	memset(rx_buf, 0, sizeof(rx_buf));

	const struct spi_buf tx = { .buf = tx_buf, .len = sizeof(tx_buf) };
	const struct spi_buf rx = { .buf = rx_buf, .len = sizeof(rx_buf) };
	const struct spi_buf_set tx_set = { .buffers = &tx, .count = 1 };
	const struct spi_buf_set rx_set = { .buffers = &rx, .count = 1 };

	int ret = spi_transceive(spi_dev, &spi_cfg, &tx_set, &rx_set);

	if (ret != 0) {
		LOG_ERR("spi_transceive failed: %d", ret);
		return ret;
	}

	if (memcmp(tx_buf, rx_buf, TEST_BUF_LEN) != 0) {
		LOG_ERR("loopback mismatch on iter %u", iteration);
		LOG_HEXDUMP_ERR(tx_buf, TEST_BUF_LEN, "TX");
		LOG_HEXDUMP_ERR(rx_buf, TEST_BUF_LEN, "RX");
		return -EIO;
	}

	LOG_INF("loopback OK (iter=%u, %u bytes @ %u Hz)",
		iteration, TEST_BUF_LEN, SPI_FREQ_HZ);
	return 0;
}

static void spi_loopback_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1); ARG_UNUSED(p2); ARG_UNUSED(p3);

	if (!device_is_ready(spi_dev)) {
		LOG_ERR("SPIM4 device not ready");
		return;
	}

	LOG_INF("SPIM4 loopback thread started (freq=%u Hz)", SPI_FREQ_HZ);

	uint32_t iter = 0;

	while (1) {
		(void)run_one_transfer(iter++);
		k_msleep(TEST_PERIOD_MS);
	}
}

int spi_loopback_start(void)
{
	k_tid_t tid = k_thread_create(&spi_thread_data, spi_stack,
				      K_THREAD_STACK_SIZEOF(spi_stack),
				      spi_loopback_thread,
				      NULL, NULL, NULL,
				      K_PRIO_PREEMPT(7), 0, K_NO_WAIT);
	if (tid == NULL) {
		return -ENOMEM;
	}
	k_thread_name_set(tid, "spi_loopback");
	return 0;
}

#else /* !DT_NODE_EXISTS(spi4) — e.g. nrf5340bsim */

int spi_loopback_start(void)
{
	LOG_WRN("spi4 node not present in devicetree; loopback disabled");
	return 0;
}

#endif

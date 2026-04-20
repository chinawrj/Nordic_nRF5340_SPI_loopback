/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * SPIM4 32 MHz loopback test — sends a 32-byte incrementing pattern and
 * compares RX vs TX. Runs as an independent cooperative thread so it does
 * not block the BLE host.
 */

#include "spi_loopback.h"

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(spi_loopback, LOG_LEVEL_INF);

#define SPI_NODE        DT_NODELABEL(spi4)
#define TEST_BUF_LEN    32U
#define TEST_PERIOD_MS  1000

#define SPI_OP  (SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB | SPI_WORD_SET(8))

static const struct spi_dt_spec spi_dev = SPI_DT_SPEC_GET(SPI_NODE, SPI_OP, 0);

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

	int ret = spi_transceive_dt(&spi_dev, &tx_set, &rx_set);

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

	LOG_INF("loopback OK (iter=%u, %u bytes @ 32 MHz)",
		iteration, TEST_BUF_LEN);
	return 0;
}

static void spi_loopback_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1); ARG_UNUSED(p2); ARG_UNUSED(p3);

	if (!spi_is_ready_dt(&spi_dev)) {
		LOG_ERR("SPIM4 device not ready");
		return;
	}

	LOG_INF("SPIM4 loopback thread started (freq=%u Hz)",
		spi_dev.config.frequency);

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

/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * SPI loopback self-test on SPIM4 @ 32 MHz.
 *
 * MOSI (P1.13) must be jumpered to MISO (P1.14) on the nRF5340 DK.
 */

#ifndef APP_SPI_LOOPBACK_H_
#define APP_SPI_LOOPBACK_H_

#include <stdint.h>

/**
 * @brief Start the SPI loopback test thread.
 *
 * Spawns a dedicated thread that periodically transmits a deterministic
 * 32-byte pattern on SPIM4 and verifies the received bytes match.
 *
 * @return 0 on success, negative errno otherwise.
 */
int spi_loopback_start(void);

#endif /* APP_SPI_LOOPBACK_H_ */

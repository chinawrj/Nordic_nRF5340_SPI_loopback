# SPDX-License-Identifier: Apache-2.0
#
# nRF5340 SPI Loopback + BLE Heart Rate — application CMakeLists.
# Targets board: nrf5340dk/nrf5340/cpuapp (sysbuild builds ipc_radio on cpunet).

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(nrf5340_spi_loopback LANGUAGES C)

target_sources(app PRIVATE
    src/main.c
    src/spi_loopback.c
    src/ble_hrs.c
)

target_include_directories(app PRIVATE src)

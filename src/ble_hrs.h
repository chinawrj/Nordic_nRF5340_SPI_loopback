/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * BLE Heart Rate Service peripheral — simulates a heart-rate sensor and
 * notifies a connected central at 1 Hz.
 */

#ifndef APP_BLE_HRS_H_
#define APP_BLE_HRS_H_

/**
 * @brief Initialise BT, register HRS, start connectable advertising.
 *
 * Must be called after kernel init. Spawns a thread that emits a simulated
 * heart-rate value (60-100 bpm triangle wave) once per second when a central
 * is connected.
 *
 * @return 0 on success, negative errno otherwise.
 */
int ble_hrs_start(void);

#endif /* APP_BLE_HRS_H_ */

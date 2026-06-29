/*
 * Copyright (c) 2026 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Connectionless BLE color synchronization. Each board advertises its current
 * color (manufacturer data) while scanning for other boards. A monotonic
 * sequence number provides "latest press wins" ordering across all boards.
 */

#ifndef REEL_SYNC_BLE_SYNC_H_
#define REEL_SYNC_BLE_SYNC_H_

#include <stdint.h>

/* Invoked (from the BT RX context) when a newer color is received from a peer. */
typedef void (*ble_color_cb_t)(uint8_t color_idx);

/*
 * Enable Bluetooth, start non-connectable advertising and passive scanning.
 * @p on_remote_color is called whenever a peer's newer color is adopted.
 */
int ble_sync_init(ble_color_cb_t on_remote_color);

/*
 * Announce a locally chosen color (user pressed the button): take the next
 * sequence number and update the advertised payload so peers adopt it.
 */
void ble_sync_broadcast(uint8_t color_idx);

#endif /* REEL_SYNC_BLE_SYNC_H_ */

/*
 * Copyright (c) 2026 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * RGB LED palette driven over the reel_board's GPIO LEDs (led0/led1/led2).
 */

#ifndef REEL_SYNC_LED_H_
#define REEL_SYNC_LED_H_

#include <stdint.h>

/* Number of entries in the color palette (index 0 == off). */
#define LED_COLOR_COUNT 8

/* Initialize the RGB GPIO LEDs. Returns 0 on success. */
int led_init(void);

/* Drive the RGB LED to palette entry @p idx (wrapped into range). */
void led_set(uint8_t idx);

/* Currently displayed palette index. */
uint8_t led_get(void);

/* Next palette index after the current one (wraps around). */
uint8_t led_next_index(void);

/* Human readable name for a palette index, e.g. "Green". */
const char *led_color_name(uint8_t idx);

#endif /* REEL_SYNC_LED_H_ */

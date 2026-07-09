/*
 * SPDX-FileCopyrightText: 2026 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Sensor dashboard rendered on the reel_board e-paper display via CFB.
 */

#ifndef REEL_SYNC_DISPLAY_H_
#define REEL_SYNC_DISPLAY_H_

#include "sensors.h"

/* Initialize the e-paper display and character framebuffer. */
int display_init(void);

/* Render the sensor readings and the active LED color onto the panel. */
void display_show(const struct app_sensors *s, const char *color_name);

#endif /* REEL_SYNC_DISPLAY_H_ */

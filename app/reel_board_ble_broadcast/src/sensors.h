/*
 * Copyright (c) 2026 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Reads the reel_board on-board sensors (HDC1010, MMA8652FC, APDS9960).
 */

#ifndef REEL_SYNC_SENSORS_H_
#define REEL_SYNC_SENSORS_H_

#include <stdbool.h>

struct app_sensors {
	double temp_c;     /* HDC1010 ambient temperature  [degC]   */
	double humidity;   /* HDC1010 relative humidity     [%RH]   */
	double accel_x;    /* MMA8652FC acceleration X       [m/s^2] */
	double accel_y;    /* MMA8652FC acceleration Y       [m/s^2] */
	double accel_z;    /* MMA8652FC acceleration Z       [m/s^2] */
	double light;      /* APDS9960 ambient light         [a.u.] */
	double prox;       /* APDS9960 proximity             [a.u.] */

	bool hdc_ok;
	bool accel_ok;
	bool apds_ok;
};

/* Check that all sensor devices are ready. Returns 0 if all present. */
int sensors_init(void);

/* Sample every sensor into @p out. Per-sensor *_ok flags report success. */
void sensors_read(struct app_sensors *out);

#endif /* REEL_SYNC_SENSORS_H_ */

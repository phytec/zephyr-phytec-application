/*
 * SPDX-FileCopyrightText: PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

#include "sensors.h"

LOG_MODULE_REGISTER(sensors, LOG_LEVEL_INF);

/* Device handles, resolved from the board devicetree compatibles. */
static const struct device *const hdc1010 = DEVICE_DT_GET_ONE(ti_hdc1010);
static const struct device *const mma8652 = DEVICE_DT_GET_ONE(nxp_mma8652fc);
static const struct device *const apds9960 = DEVICE_DT_GET_ONE(avago_apds9960);

int sensors_init(void)
{
	int ret = 0;

	if (!device_is_ready(hdc1010)) {
		LOG_WRN("HDC1010 (temp/humidity) not ready");
		ret = -ENODEV;
	}
	if (!device_is_ready(mma8652)) {
		LOG_WRN("MMA8652FC (accel) not ready");
		ret = -ENODEV;
	}
	if (!device_is_ready(apds9960)) {
		LOG_WRN("APDS9960 (light/prox) not ready");
		ret = -ENODEV;
	}

	return ret;
}

static bool read_hdc1010(struct app_sensors *out)
{
	struct sensor_value temp, humidity;

	if (sensor_sample_fetch(hdc1010) ||
	    sensor_channel_get(hdc1010, SENSOR_CHAN_AMBIENT_TEMP, &temp) ||
	    sensor_channel_get(hdc1010, SENSOR_CHAN_HUMIDITY, &humidity)) {
		return false;
	}

	out->temp_c = sensor_value_to_double(&temp);
	out->humidity = sensor_value_to_double(&humidity);
	return true;
}

static bool read_mma8652(struct app_sensors *out)
{
	struct sensor_value accel[3];

	if (sensor_sample_fetch(mma8652) ||
	    sensor_channel_get(mma8652, SENSOR_CHAN_ACCEL_XYZ, accel)) {
		return false;
	}

	out->accel_x = sensor_value_to_double(&accel[0]);
	out->accel_y = sensor_value_to_double(&accel[1]);
	out->accel_z = sensor_value_to_double(&accel[2]);
	return true;
}

static bool read_apds9960(struct app_sensors *out)
{
	struct sensor_value light, prox;

	if (sensor_sample_fetch(apds9960) ||
	    sensor_channel_get(apds9960, SENSOR_CHAN_LIGHT, &light) ||
	    sensor_channel_get(apds9960, SENSOR_CHAN_PROX, &prox)) {
		return false;
	}

	out->light = sensor_value_to_double(&light);
	out->prox = sensor_value_to_double(&prox);
	return true;
}

void sensors_read(struct app_sensors *out)
{
	memset(out, 0, sizeof(*out));

	out->hdc_ok = read_hdc1010(out);
	out->accel_ok = read_mma8652(out);
	out->apds_ok = read_apds9960(out);
}

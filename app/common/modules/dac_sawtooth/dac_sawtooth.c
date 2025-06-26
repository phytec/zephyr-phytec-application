/*
 * Copyright (c) 2025 PHYTEC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/dac.h>
#include <zephyr/sys/printk.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dac, CONFIG_APP_LOG_LEVEL);

#define SLEEP_TIME_MS			200

/* size of stack area used by each thread */
#define DAC_SAWTOOTH_STACKSIZE		1024
/* scheduling priority used by each thread */
#define DAC_SAWTOOTH_PRIORITY		5

#define DAC_CHANNEL			0
#define DAC_RESOLUTION			12
#define DAC_STEPS			100
#define DAC_MAX_VALUE			4095

/*
 * Get DAC configuration from the devicetree dac node label. This is mandatory.
 */
#define DAC_NODE			DT_NODELABEL(dac)
static const struct device *dac = DEVICE_DT_GET_OR_NULL(DAC_NODE);

void dac_sawtooth(void)
{
	int ret;
	struct dac_channel_cfg cfg = {0};
	uint32_t output_value = 0;

	if (!device_is_ready(dac)) {
		LOG_ERR("%s is not ready", dac->name);
		return;
	}

	cfg.channel_id = DAC_CHANNEL;
	cfg.resolution = DAC_RESOLUTION;
	ret = dac_channel_setup(dac, &cfg);
	if (ret < 0) {
		LOG_ERR("Unable to configure %s", dac->name);
		return;
	}

	while (1) {
		ret = dac_write_value(dac, DAC_CHANNEL, output_value);
		if (ret < 0) {
			LOG_ERR("Failed to write value to %s", dac->name);
			return;
		}

		k_sleep(K_MSEC(SLEEP_TIME_MS));

		output_value += DAC_STEPS;
		if (output_value > DAC_MAX_VALUE)
			output_value = 0;
	}

	return;
}

K_THREAD_DEFINE(dac_sawtooth_tid, DAC_SAWTOOTH_STACKSIZE, dac_sawtooth, NULL, NULL,
		NULL, DAC_SAWTOOTH_PRIORITY, 0, 0);

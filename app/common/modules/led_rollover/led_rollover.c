/*
 * Copyright (c) 2025 PHYTEC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/led.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(led_rollover, CONFIG_APP_LOG_LEVEL);

#define LED_DEVICE_NODE DT_PATH(leds)
#define LED_COUNT		4

/* Thread configuration */
#define LED_ROLLOVER_STACKSIZE	1024
#define LED_ROLLOVER_PRIORITY	5
#define SLEEP_TIME_MS		60


void led_rollover(void)
{
	const struct device *led_dev = DEVICE_DT_GET(LED_DEVICE_NODE);

	if (!device_is_ready(led_dev)) {
		LOG_ERR("LED device not ready");
		return;
	}

	LOG_INF("Starting LED blink demo using LED subsystem");

	while (1) {
		// Toggle each of the LEDs on and off in sequence
		for (int i = 0; i < LED_COUNT; i++) {
			led_on(led_dev, i);
			k_sleep(K_MSEC(200));
			led_off(led_dev, i);
			k_sleep(K_MSEC(200));
		}

		// Pause before repeating
		k_sleep(K_MSEC(SLEEP_TIME_MS));
	}
}

K_THREAD_DEFINE(led_rollover_tid, LED_ROLLOVER_STACKSIZE, led_rollover,
	NULL, NULL, NULL, LED_ROLLOVER_PRIORITY, 0, 0);

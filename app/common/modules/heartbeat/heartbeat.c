/*
 * Copyright (c) 2025 PHYTEC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(heartbeat, CONFIG_APP_LOG_LEVEL);

/* size of stack area used by each thread */
#define HEARTBEAT_STACKSIZE	1024
/* scheduling priority used by each thread */
#define HEARTBEAT_PRIORITY	5

/*
 * Get LED configuration from the devicetree led0 alias. This is mandatory.
 */
#define LED0_NODE		DT_ALIAS(led0)
#if !DT_NODE_HAS_STATUS_OKAY(LED0_NODE)
#error "Unsupported board: led0 devicetree alias is not defined"
#endif
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET_OR(LED0_NODE, gpios,
							   {0});

void heartbeat(void)
{
	int ret;

	if (!gpio_is_ready_dt(&led)) {
		LOG_ERR("LED device %s is not ready", led.port->name);
		return;
	}

	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		LOG_ERR("failed to configure %s pin %d - %d",
			led.port->name, led.pin, ret);
		return;
	}

	while (1) {
		ret = gpio_pin_set_dt(&led, 1);
		if (ret < 0) {
			LOG_ERR("failed to set %s pin %d - %d",
				led.port->name, led.pin, ret);
			return;
		}
		k_sleep(K_MSEC(150));

		ret = gpio_pin_set_dt(&led, 0);
		if (ret < 0) {
			LOG_ERR("failed to set %s pin %d - %d",
				led.port->name, led.pin, ret);
			return;
		}
		k_sleep(K_MSEC(50));

		ret = gpio_pin_set_dt(&led, 1);
		if (ret < 0) {
			LOG_ERR("failed to set %s pin %d - %d",
				led.port->name, led.pin, ret);
			return;
		}
		k_sleep(K_MSEC(150));

		ret = gpio_pin_set_dt(&led, 0);
		if (ret < 0) {
			LOG_ERR("failed to set %s pin %d - %d",
				led.port->name, led.pin, ret);
			return;
		}

		k_sleep(K_SECONDS(1));
	}
}

K_THREAD_DEFINE(heartbeat_tid, HEARTBEAT_STACKSIZE, heartbeat, NULL, NULL,
		NULL, HEARTBEAT_PRIORITY, 0, 0);

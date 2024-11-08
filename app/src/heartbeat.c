/*
 * Copyright (c) 2024 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

/* The devicetree node identifier for the "led0" alias. */
#define LED0_NODE DT_ALIAS(led0)

/*
 * A build error on this line means your board is unsupported.
 * See the sample documentation for information on how to fix this.
 */
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

void heartbeat(void)
{
	int ret;

	if (!gpio_is_ready_dt(&led)) {
		return;
	}

	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		return;
	}

	while (1) {
		ret = gpio_pin_set_dt(&led, 1);
		if (ret < 0) {
			return;
		}
		k_sleep(K_MSEC(150));

		ret = gpio_pin_set_dt(&led, 0);
		if (ret < 0) {
			return;
		}
		k_sleep(K_MSEC(50));

		ret = gpio_pin_set_dt(&led, 1);
		if (ret < 0) {
			return;
		}
		k_sleep(K_MSEC(150));

		ret = gpio_pin_set_dt(&led, 0);
		if (ret < 0) {
			return;
		}

		k_sleep(K_SECONDS(1));
	}
}

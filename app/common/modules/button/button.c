/*
 * Copyright (c) 2025 PHYTEC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(button, CONFIG_APP_LOG_LEVEL);

#define SLEEP_TIME_S		1

/* size of stack area used by each thread */
#define BUTTON_STACKSIZE	1024
/* scheduling priority used by each thread */
#define BUTTON_PRIORITY		5

/*
 * Get button configuration from the devicetree sw0 alias. This is mandatory.
 */
#define SW0_NODE	DT_ALIAS(sw0)
#if !DT_NODE_HAS_STATUS_OKAY(SW0_NODE)
#error "Unsupported board: sw0 devicetree alias is not defined"
#endif
static const struct gpio_dt_spec sw_btn = GPIO_DT_SPEC_GET_OR(SW0_NODE, gpios,
							      {0});
static struct gpio_callback button_cb_data;

void button_pressed(const struct device *dev, struct gpio_callback *cb,
		    uint32_t pins)
{
	printk("Button S1 pressed at %" PRIu32 "\n", k_cycle_get_32());
}

void button(void)
{
	int ret;

	if (!gpio_is_ready_dt(&sw_btn)) {
		LOG_ERR("button device %s is not ready", sw_btn.port->name);
		return;
	}

	ret = gpio_pin_configure_dt(&sw_btn, GPIO_INPUT);
	if (ret != 0) {
		LOG_ERR("failed to configure %s pin %d - %d",
			sw_btn.port->name, sw_btn.pin, ret);
		return;
	}

	ret = gpio_pin_interrupt_configure_dt(&sw_btn,
					      GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0) {
		LOG_ERR("failed to configure interrupt on %s pin %d - %d",
			sw_btn.port->name, sw_btn.pin, ret);
		return;
	}

	gpio_init_callback(&button_cb_data, button_pressed, BIT(sw_btn.pin));
	gpio_add_callback(sw_btn.port, &button_cb_data);
	LOG_INF("Set up button at %s pin %d", sw_btn.port->name, sw_btn.pin);

	while (1) {
		k_sleep(K_SECONDS(SLEEP_TIME_S));
	}
}

K_THREAD_DEFINE(button_tid, BUTTON_STACKSIZE, button, NULL, NULL,
		NULL, BUTTON_PRIORITY, 0, 0);

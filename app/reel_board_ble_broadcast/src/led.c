/*
 * SPDX-FileCopyrightText: 2026 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#include "led.h"

LOG_MODULE_REGISTER(led, LOG_LEVEL_INF);

/*
 * reel_board RGB LED. The three channels are exposed as aliases led0 (red),
 * led1 (green) and led2 (blue) and are wired active-low; the GPIO_DT_SPEC
 * flags handle the inversion so gpio_pin_set_dt(..., 1) means "on".
 */
static const struct gpio_dt_spec rgb[] = {
	GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios), /* red   */
	GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios), /* green */
	GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios), /* blue  */
};

/* Palette: {R, G, B} on/off per channel. Index 0 is "all off". */
static const struct {
	uint8_t r, g, b;
	const char *name;
} palette[LED_COLOR_COUNT] = {
	{0, 0, 0, "Off"},
	{1, 0, 0, "Red"},
	{0, 1, 0, "Green"},
	{0, 0, 1, "Blue"},
	{1, 1, 0, "Yellow"},
	{0, 1, 1, "Cyan"},
	{1, 0, 1, "Magenta"},
	{1, 1, 1, "White"},
};

static uint8_t current_idx;

int led_init(void)
{
	for (size_t i = 0; i < ARRAY_SIZE(rgb); i++) {
		if (!gpio_is_ready_dt(&rgb[i])) {
			LOG_ERR("LED channel %u not ready", (unsigned int)i);
			return -ENODEV;
		}

		int err = gpio_pin_configure_dt(&rgb[i], GPIO_OUTPUT_INACTIVE);

		if (err) {
			LOG_ERR("LED channel %u config failed (%d)",
				(unsigned int)i, err);
			return err;
		}
	}

	led_set(0);
	return 0;
}

void led_set(uint8_t idx)
{
	idx %= LED_COLOR_COUNT;
	current_idx = idx;

	gpio_pin_set_dt(&rgb[0], palette[idx].r);
	gpio_pin_set_dt(&rgb[1], palette[idx].g);
	gpio_pin_set_dt(&rgb[2], palette[idx].b);
}

uint8_t led_get(void)
{
	return current_idx;
}

uint8_t led_next_index(void)
{
	return (current_idx + 1) % LED_COLOR_COUNT;
}

const char *led_color_name(uint8_t idx)
{
	return palette[idx % LED_COLOR_COUNT].name;
}

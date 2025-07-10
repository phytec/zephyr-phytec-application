/*
 * Copyright (c) 2025 PHYTEC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/display.h>
#include <lvgl.h>
#include <lvgl_mem.h>
#include <lv_demos.h>
#include <stdio.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(lvgl_widgets, CONFIG_APP_LOG_LEVEL);

/* size of stack area used by each thread */
#define LVGL_WIDGETS_STACKSIZE		8192
/* scheduling priority used by each thread */
#define LVGL_WIDGETS_PRIORITY		5

void lvgl_widgets(void)
{
	const struct device *display_dev;

	display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	if (!device_is_ready(display_dev)) {
		LOG_ERR("Device not ready, aborting test");
		return;
	}

	lv_demo_widgets();

	lv_timer_handler();
	display_blanking_off(display_dev);

	while (1) {
		lv_timer_handler();
		k_sleep(K_MSEC(50));
	}
}

K_THREAD_DEFINE(lvgl_widgets_tid, LVGL_WIDGETS_STACKSIZE,
		lvgl_widgets, NULL, NULL, NULL,
		LVGL_WIDGETS_PRIORITY, 0, 0);

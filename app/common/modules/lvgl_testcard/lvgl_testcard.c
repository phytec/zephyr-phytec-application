/*
 * Copyright (c) 2025 PHYTEC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/display.h>
#include <zephyr/shell/shell.h>
#include <zephyr/random/random.h>
#include <lvgl.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <lvgl_input_device.h>

#include "testcard.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(lvgl_random_number, CONFIG_APP_LOG_LEVEL);

/* size of stack area used by each thread */
#define LVGL_RANDOM_NUMBER_STACKSIZE		4096
/* scheduling priority used by each thread */
#define LVGL_RANDOM_NUMBER_PRIORITY		5

static uint8_t testcard_ready;

void lvgl_random_number(void)
{
	const struct device *display_dev;
	lv_obj_t *image;

	testcard_ready = 0;

	display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	if (!device_is_ready(display_dev)) {
		LOG_ERR("Device not ready, aborting test");
		return;
	}

 	LV_IMG_DECLARE(img_testcard_rgb);
	image = lv_img_create(lv_scr_act());
	lv_img_set_src(image, &img_testcard_rgb);
	lv_obj_align(image, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_pos(image, 0, 0);
	lv_obj_set_size(image, 720, 1280);

	lv_timer_handler();
	display_blanking_off(display_dev);

	testcard_ready = 1;
	while (1) {
		uint32_t sleep_ms = lv_timer_handler();

		k_msleep(MIN(sleep_ms, INT32_MAX));
	}
}

static int cmd_testcard_ready(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(sh, "%u", testcard_ready);

	return 0;
}

SHELL_CMD_ARG_REGISTER(phytec_testcard_ready, NULL, "Status of the LVGL testcard", cmd_testcard_ready, 1, 0);

K_THREAD_DEFINE(lvgl_random_number_tid, LVGL_RANDOM_NUMBER_STACKSIZE,
		lvgl_random_number, NULL, NULL, NULL,
		LVGL_RANDOM_NUMBER_PRIORITY, 0, 0);

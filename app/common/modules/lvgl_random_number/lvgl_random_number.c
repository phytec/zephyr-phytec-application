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

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(lvgl_random_number, CONFIG_APP_LOG_LEVEL);

/* size of stack area used by each thread */
#define LVGL_RANDOM_NUMBER_STACKSIZE		4096
/* scheduling priority used by each thread */
#define LVGL_RANDOM_NUMBER_PRIORITY		5

static uint32_t count;
static uint8_t random_number;
static uint8_t button_pressed;

static void lv_btn_click_callback(lv_event_t *e)
{
	ARG_UNUSED(e);

	count = 0;
	button_pressed = 1;
}

void lvgl_random_number(void)
{
	char count_str[11] = {0};
	char random_number_str[11] = {0};
	const struct device *display_dev;
	static lv_style_t style_button;
	static lv_style_t style_label;
	lv_obj_t *random_number_label;
	lv_obj_t *button_pressed_label;
	lv_obj_t *count_label;

	display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	if (!device_is_ready(display_dev)) {
		LOG_ERR("Device not ready, aborting test");
		return;
	}

	button_pressed = 0;
	random_number = sys_rand8_get() % 100;

	if (IS_ENABLED(CONFIG_LV_Z_POINTER_INPUT)) {
		lv_obj_t *hello_world_button;

		hello_world_button = lv_button_create(lv_screen_active());
		lv_obj_align(hello_world_button, LV_ALIGN_CENTER, 0, -15);
		lv_obj_add_event_cb(hello_world_button, lv_btn_click_callback,
				    LV_EVENT_CLICKED, NULL);
		random_number_label = lv_label_create(hello_world_button);
	} else {
		random_number_label = lv_label_create(lv_screen_active());
	}

	lv_style_init(&style_button);
	if (IS_ENABLED(CONFIG_LV_FONT_MONTSERRAT_48)) {
		lv_style_set_text_font(&style_button, &lv_font_montserrat_48);
	}
	lv_obj_add_style(random_number_label, &style_button, 0);
	sprintf(random_number_str, "%d", random_number);
	lv_label_set_text(random_number_label, random_number_str);
	lv_obj_align(random_number_label, LV_ALIGN_CENTER, 0, 0);

	count_label = lv_label_create(lv_screen_active());
	lv_obj_align(count_label, LV_ALIGN_BOTTOM_MID, 0, 0);

	lv_style_init(&style_label);
	if (IS_ENABLED(CONFIG_LV_FONT_MONTSERRAT_32)) {
		lv_style_set_text_font(&style_label, &lv_font_montserrat_32);
	}
	button_pressed_label = lv_label_create(lv_screen_active());
	lv_obj_add_style(button_pressed_label, &style_label, 0);
	lv_label_set_text(button_pressed_label, "");
	lv_obj_align(button_pressed_label, LV_ALIGN_CENTER, 0, 200);

	lv_timer_handler();
	display_blanking_off(display_dev);
	display_set_orientation(display_dev, DISPLAY_ORIENTATION_ROTATED_180);

	while (1) {
		if ((count % 100) == 0U) {
			sprintf(count_str, "%d", count/100U);
			lv_label_set_text(count_label, count_str);
		}
		if (button_pressed) {
			lv_label_set_text(button_pressed_label,
					  "Button pressed!");
		} else {
			lv_label_set_text(button_pressed_label, "");
		}
		lv_timer_handler();
		++count;
		k_sleep(K_MSEC(10));
	}
}

static int cmd_random_number(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(sh, "%u", random_number);

	return 0;
}

static int cmd_button_pressed(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(sh, "%u", button_pressed);

	return 0;
}

static int cmd_button_reset(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	button_pressed = 0;

	return 0;
}

SHELL_CMD_ARG_REGISTER(phytec_random_number, NULL, "Show LVGL random number",
		       cmd_random_number, 1, 0);
SHELL_CMD_ARG_REGISTER(phytec_button_pressed, NULL, "Returns whether the button was pressed",
		       cmd_button_pressed, 1, 0);
SHELL_CMD_ARG_REGISTER(phytec_button_reset, NULL, "Reset the button pressed value",
		       cmd_button_reset, 1, 0);

K_THREAD_DEFINE(lvgl_random_number_tid, LVGL_RANDOM_NUMBER_STACKSIZE,
		lvgl_random_number, NULL, NULL, NULL,
		LVGL_RANDOM_NUMBER_PRIORITY, 0, 0);

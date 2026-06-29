/*
 * SPDX-FileCopyrightText: 2026 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/display/cfb.h>
#include <zephyr/logging/log.h>

#include <stdio.h>
#include <string.h>

#include "display.h"

LOG_MODULE_REGISTER(display, LOG_LEVEL_INF);

static const struct device *const epd = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));

/*
 * Default CFB font index 0 is the 10x16 "font1016". On the 250x122 panel that
 * gives 25 columns and 7 rows, which comfortably fits the dashboard below.
 */
#define FONT_IDX        0
#define ROW_HEIGHT_PX   16

static void print_row(int row, const char *text)
{
	if (cfb_print(epd, text, 0, row * ROW_HEIGHT_PX)) {
		LOG_ERR("cfb_print row %d failed", row);
	}
}

/* Print text horizontally centered on a row. */
static void print_centered(int row, const char *text)
{
	uint8_t fw, fh;
	int x = 0;

	if (cfb_get_font_size(epd, FONT_IDX, &fw, &fh) == 0 && fw > 0) {
		int cols = cfb_get_display_parameter(epd, CFB_DISPLAY_WIDTH) / fw;
		int len = strlen(text);

		if (cols > len) {
			x = ((cols - len) / 2) * fw;
		}
	}

	if (cfb_print(epd, text, x, row * ROW_HEIGHT_PX)) {
		LOG_ERR("cfb_print row %d failed", row);
	}
}

int display_init(void)
{
	if (!device_is_ready(epd)) {
		LOG_ERR("Display %s not ready", epd->name);
		return -ENODEV;
	}

	if (cfb_framebuffer_init(epd)) {
		LOG_ERR("Framebuffer init failed");
		return -EIO;
	}

	cfb_framebuffer_set_font(epd, FONT_IDX);
	cfb_framebuffer_clear(epd, true);
	cfb_framebuffer_invert(epd);

	return 0;
}

void display_show(const struct app_sensors *s, const char *color_name)
{
	char line[26];

	cfb_framebuffer_clear(epd, false);
	cfb_framebuffer_set_font(epd, FONT_IDX);

	print_centered(0, "reel_board sync");

	if (s->hdc_ok) {
		snprintf(line, sizeof(line), "T:%.1fC H:%.0f%%",
			 s->temp_c, s->humidity);
	} else {
		snprintf(line, sizeof(line), "T/H: ---");
	}
	print_row(1, line);

	if (s->accel_ok) {
		snprintf(line, sizeof(line), "A:%.1f %.1f %.1f",
			 s->accel_x, s->accel_y, s->accel_z);
	} else {
		snprintf(line, sizeof(line), "Accel: ---");
	}
	print_row(2, line);

	if (s->apds_ok) {
		snprintf(line, sizeof(line), "Light:%.0f", s->light);
		print_row(3, line);
		snprintf(line, sizeof(line), "Prox:%.0f", s->prox);
		print_row(4, line);
	} else {
		print_row(3, "Light/Prox: ---");
		print_row(4, "");
	}

	snprintf(line, sizeof(line), "Color: %s", color_name);
	print_row(5, line);

	int err = cfb_framebuffer_finalize(epd);

	if (err) {
		LOG_ERR("cfb_framebuffer_finalize failed (%d)", err);
	}
}

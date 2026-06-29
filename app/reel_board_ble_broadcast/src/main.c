/*
 * SPDX-FileCopyrightText: 2026 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * reel_board demo: BLE color synchronization between identical boards plus an
 * e-paper sensor dashboard.
 *
 *  - Press the user button: the local RGB LED advances to the next color and
 *    that color is broadcast so every other board mirrors it. Each board is
 *    both sender and receiver (identical firmware, no master).
 *  - The e-paper display shows temperature, humidity, acceleration (X/Y/Z),
 *    ambient light and proximity, refreshed every 2 seconds.
 */

#include <zephyr/kernel.h>
#include <zephyr/input/input.h>
#include <zephyr/logging/log.h>

#include "led.h"
#include "sensors.h"
#include "display.h"
#include "ble_sync.h"

LOG_MODULE_REGISTER(app, LOG_LEVEL_INF);

#define DISPLAY_REFRESH_INTERVAL K_SECONDS(2)

static void display_work_fn(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(display_work, display_work_fn);

/* Re-render the e-paper panel (runs on the system workqueue, never inline in
 * an input/BT callback because a full e-paper refresh takes ~1 s).
 */
static void display_work_fn(struct k_work *work)
{
	ARG_UNUSED(work);
	struct app_sensors s;

	sensors_read(&s);
	display_show(&s, led_color_name(led_get()));

	k_work_reschedule(&display_work, DISPLAY_REFRESH_INTERVAL);
}

static void request_display_refresh(void)
{
	k_work_reschedule(&display_work, K_NO_WAIT);
}

/* A peer announced a new color over BLE: mirror it locally. */
static void on_remote_color(uint8_t color_idx)
{
	led_set(color_idx);
	request_display_refresh();
}

/* User button: advance the local color and broadcast it to the other boards. */
static void input_cb(struct input_event *evt, void *user_data)
{
	ARG_UNUSED(user_data);

	if (evt->type != INPUT_EV_KEY || evt->code != INPUT_KEY_0 ||
	    evt->value == 0) {
		return;
	}

	uint8_t idx = led_next_index();

	led_set(idx);
	ble_sync_broadcast(idx);
	request_display_refresh();
}
INPUT_CALLBACK_DEFINE(NULL, input_cb, NULL);

int main(void)
{
	LOG_INF("reel_board BLE color sync + sensor dashboard");

	if (led_init()) {
		LOG_ERR("LED init failed");
	}

	if (display_init()) {
		LOG_ERR("Display init failed");
	}

	if (sensors_init()) {
		LOG_WRN("One or more sensors unavailable (continuing)");
	}

	if (ble_sync_init(on_remote_color)) {
		LOG_ERR("BLE sync init failed");
	}

	/* Draw the first frame immediately; the work item re-arms itself. */
	request_display_refresh();

	return 0;
}

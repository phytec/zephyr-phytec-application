/*
 * Copyright (c) 2025 PHYTEC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/task_wdt/task_wdt.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(watchdog, CONFIG_APP_LOG_LEVEL);

/*
 * Get watchdog configuration from the devicetree sw0 alias. This is mandatory.
 */
#define WDOG0_NODE	DT_ALIAS(watchdog0)
#if !DT_NODE_HAS_STATUS_OKAY(WDOG0_NODE)
#error "Unsupported board: watchdog0 devicetree alias is not defined"
#endif
static const struct device *const wdt = DEVICE_DT_GET(WDOG0_NODE);

int init_watchdog(void)
{
	int ret;

	ret = task_wdt_init(wdt);
	if (ret < 0) {
		LOG_ERR("Failed to init task watchdog: %d", ret);
		return ret;
	}
	LOG_DBG("Watchdog initialized");

	return 0;
}

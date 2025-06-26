/*
 * Copyright (c) 2025 PHYTEC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>

#include "../common/init/watchdog/watchdog.h"

int main(void) {
	/* Enable Watchdog */
	init_watchdog();
}

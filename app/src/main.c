/*
 * Copyright (c) 2024 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>

#include "heartbeat.h"

K_THREAD_DEFINE(heartbeat_tid, HEARTBEAT_STACKSIZE, heartbeat, NULL, NULL,
		NULL, HEARTBEAT_PRIORITY, 0, 0);

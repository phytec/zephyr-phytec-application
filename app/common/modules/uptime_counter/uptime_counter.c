/*
 * Copyright (c) 2025 PHYTEC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/eeprom.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(uptime_counter, CONFIG_APP_LOG_LEVEL);

#define SLEEP_TIME_S			60

/* size of stack area used by each thread */
#define UPTIME_COUNTER_STACKSIZE	1024

/* scheduling priority used by each thread */
#define UPTIME_COUNTER_PRIORITY		5

#define UPTIME_COUNTER_OFFSET		0
#define UPTIME_COUNTER_MAGIC		0xA3EE9703
#define UPTIME_COUNTER_EMPTY		0xFFFFFFFF

struct perisistant_values {
	uint32_t magic;
	uint64_t seconds;
} __attribute__((packed));

/*
 * Get EEPROM from the devicetree eeprom0 alias. This is mandatory.
 */
#define EEPROM0_NODE	DT_ALIAS(eeprom0)
#if !DT_NODE_HAS_STATUS_OKAY(EEPROM0_NODE)
#error "Unsupported board: eeprom0 devicetree alias is not defined"
#endif
static const struct device *eeprom = DEVICE_DT_GET_OR_NULL(EEPROM0_NODE);


void uptime_counter(void)
{
	struct perisistant_values values;
	int ret;

	if (!device_is_ready(eeprom)) {
		LOG_ERR("Device \"%s\" is not ready.", eeprom->name);
		return;
	}

	ret = eeprom_read(eeprom, UPTIME_COUNTER_OFFSET, &values, sizeof(values));
	if (ret != 0) {
		LOG_ERR("Couldn't read EEPROM: %d", ret);
		return;
	}

	if (values.magic == UPTIME_COUNTER_EMPTY) {
		LOG_INF("EEPROM is empty.");
		values.magic = UPTIME_COUNTER_MAGIC;
		values.seconds = 0;
	} else if (values.magic != UPTIME_COUNTER_MAGIC) {
		LOG_ERR("The EEPROM is either not empty or the required MAGIC header is missing.");
		return;
	}

	LOG_INF("Device uptime: %lld hours and %lld minutes", values.seconds / 3600,
		(values.seconds % 3600) / 60);

	while (1) {
		ret = eeprom_write(eeprom, UPTIME_COUNTER_OFFSET, &values,
				   sizeof(values));
		if (ret != 0) {
			LOG_ERR("Couldn't write eeprom: %d", ret);
			return;
		}

		k_sleep(K_SECONDS(SLEEP_TIME_S));
		values.seconds += SLEEP_TIME_S;
	}

	return;
}

K_THREAD_DEFINE(uptime_counter_tid, UPTIME_COUNTER_STACKSIZE, uptime_counter,
		NULL, NULL, NULL, UPTIME_COUNTER_PRIORITY, 0, 0);

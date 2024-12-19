/*
 * Copyright (c) 2025 PHYTEC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/eeprom.h>

#define UPTIME_COUNTER_OFFSET	0
#define UPTIME_COUNTER_MAGIC	0xEE9703
#define UPTIME_COUNTER_EMPTY	0xFFFFFFFF

#define SNVS_RTC_NODE DT_NODELABEL(snvs_rtc)
#define DELAY 60

struct perisistant_values {
	uint32_t magic;
	uint32_t ticks;
};

struct counter_alarm_cfg alarm_cfg;

/*
 * Get a device structure from a devicetree node with alias eeprom-0
 */
static const struct device *get_eeprom_device(void)
{
	const struct device *const dev = DEVICE_DT_GET(DT_ALIAS(eeprom_0));

	if (!device_is_ready(dev)) {
		printk("\nError: Device \"%s\" is not ready; "
		       "check the driver initialization logs for errors.\n",
		       dev->name);
		return NULL;
	}

	printk("Found EEPROM device \"%s\"\n", dev->name);
	return dev;
}

/*
 * Get a device structure from a devicetree node with snvs_rtc
 */
static const struct device *get_snvs_rtc_device(void)
{
	const struct device *const dev = DEVICE_DT_GET(SNVS_RTC_NODE);

	if (!device_is_ready(dev)) {
		printk("\nError: Device \"%s\" is not ready; "
		       "check the driver initialization logs for errors.\n",
		       dev->name);
		return NULL;
	}

	printk("Found SNVS_RTC device \"%s\"\n", dev->name);
	return dev;
}

void uptime_counter(void)
{
	const struct device *eeprom = get_eeprom_device();
	const struct device *snvs_rtc = get_snvs_rtc_device();
	struct perisistant_values values;
	uint32_t old_ticks, now_ticks;
	uint64_t total_usec;
	int total_sec;
	int rc;

	if ((eeprom == NULL) || (snvs_rtc == NULL)) {
		return;
	}

	counter_start(snvs_rtc);

	rc = eeprom_read(eeprom, UPTIME_COUNTER_OFFSET, &values, sizeof(values));
	if (rc < 0) {
		printk("Error: Couldn't read eeprom: err: %d.\n", rc);
		return;
	}

	if (values.magic == UPTIME_COUNTER_EMPTY) {
		printk("EEPROM is empty.\n");
		values.magic = UPTIME_COUNTER_MAGIC;
		values.ticks = 0;
	} else if (values.magic != UPTIME_COUNTER_MAGIC) {
		printk("Error: EEPROM is not empty or no MAGIC header is found.\n");
		return;
	}

	old_ticks = values.ticks;

	rc = counter_get_value(snvs_rtc, &now_ticks);
	if (!counter_is_counting_up(snvs_rtc)) {
		now_ticks = counter_get_top_value(snvs_rtc) - now_ticks;
	}

	if (rc) {
		printk("Failed to read counter value (rc %d)", rc);
		return;
	}

	total_usec = counter_ticks_to_us(snvs_rtc, old_ticks + now_ticks);
	total_sec = (int)(total_usec / USEC_PER_SEC);
	printk("Device up %d hr %d min\n", total_sec/3600, (total_sec % 3600) / 60);

	while (1) {
		rc = counter_get_value(snvs_rtc, &now_ticks);
		if (!counter_is_counting_up(snvs_rtc)) {
			now_ticks = counter_get_top_value(snvs_rtc) - now_ticks;
		}

		if (rc) {
			printk("Failed to read counter value (rc %d)", rc);
			return;
		}

		values.ticks = old_ticks + now_ticks;

		rc = eeprom_write(eeprom, UPTIME_COUNTER_OFFSET, &values,
				  sizeof(values));
		if (rc < 0) {
			printk("Error: Couldn't write eeprom: rc: %d.\n", rc);
			return;
		}

		k_sleep(K_SECONDS(DELAY));
	}

	return;
}

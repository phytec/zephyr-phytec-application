/*
 * Copyright (c) 2025 PHYTEC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/printk.h>

/* size of stack area used by each thread */
#define EXP_UART_STACKSIZE	1024
/* scheduling priority used by each thread */
#define EXP_UART_PRIORITY	5

#define UART5_NODE DT_NODELABEL(lpuart5)

void exp_uart(void)
{
	printk("Console: Application started on lpuart1\n");

	const struct device *uart5 = DEVICE_DT_GET(UART5_NODE);

	if (!device_is_ready(uart5)) {
		printk("Console: UART5 device not ready!\n");
		return;
	}

	printk("Console: UART5 device is ready. Entering message loop...\n");
	printk("UART5 device name: %s\n", uart5->name);

	uint32_t count = 0;

	while (1) {
		char buf[64];
		int len = snprintk(buf, sizeof(buf),
			"UART5 says hello! Count: %u\r\n", count++);

		for (int i = 0; i < len; i++) {
			uart_poll_out(uart5, buf[i]);
			k_busy_wait(300); // slight delay per character
		}

		k_msleep(500); // wait 0.5 second before sending next message
	}
}

K_THREAD_DEFINE(exp_uart_tid, EXP_UART_STACKSIZE, exp_uart, NULL, NULL,
	NULL, EXP_UART_PRIORITY, 0, 0);

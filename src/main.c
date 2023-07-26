// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Gabriel Marcano, 2023

#include <scron.h>
#include <power_control.h>
#include <artemia.h>

#include <adc.h>
#include <spi.h>
#include <lora.h>
#include <gpio.h>
#include <uart.h>
#include <am1815.h>
#include <syscalls.h>
#include <flash.h>
#include <asimple_littlefs.h>

#include "am_mcu_apollo.h"
#include "am_bsp.h"
#include "am_util.h"

#include <sys/time.h>

#include <string.h>
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <time.h>

static struct spi spi;
static struct am1815 rtc;
static struct power_control power_control;
static struct scron scron;
static struct uart uart;
static struct asimple_littlefs fs;
static struct flash flash;

// Example task 1
static int task1(void *data)
{
	(void)data;
	return 0;
}

// FIXME maybe this should be the idle task
static int task_manage_trickle(void* data)
{
	(void)data;
	// Basic idea:
	// 1. Check backup battery voltage
	// 2. If below threshold, enable
	// 3. If above threshold, disable
	double backup_voltage = 2.0; // FIXME sense
	if (backup_voltage < 1.8)
	{
		am1815_enable_trickle(&rtc);
	}
	else if (backup_voltage > 1.9)
	{
		am1815_disable_trickle(&rtc);
	}
	return 0;
}

#define ARRAY_SIZE(array) (sizeof(array)/sizeof(*array))

static struct scron_task tasks_[] = {
	{
		.name = "task1",
		.minimum_voltage = 2.0,
		.function = task1,
		.schedule = {
			.month = -1,
			.day = -1,
			.weekday = -1,
			.hour = -1,
			.minute = -1,
			.second = 30,
		},
	},
	{
		.name = "task2",
		.minimum_voltage = 2.0,
		.function = task_manage_trickle,
		.schedule = {
			.month = -1,
			.day = -1,
			.weekday = -1,
			.hour = -1,
			.minute = -1,
			.second = 30,
		},
	},
};

static struct scron_tasks tasks = {
	.tasks = tasks_,
	.size = ARRAY_SIZE(tasks_),
};

// FIXME implement loading from file
static void load_callback(const char *name, time_t *last_run)
{
	(void)name;
	*last_run = 1;
}


// FIXME implement saving to file
static void save_callback(const char *name, time_t last_run)
{
	(void)name;
	(void)last_run;
}

__attribute__((constructor))
static void redboard_init(void)
{
	// Prepare MCU by init-ing clock, cache, and power level operation
	am_hal_clkgen_control(AM_HAL_CLKGEN_CONTROL_SYSCLK_MAX, 0);
	am_hal_cachectrl_config(&am_hal_cachectrl_defaults);
	am_hal_cachectrl_enable();
	am_bsp_low_power_init();
	am_hal_sysctrl_fpu_enable();
	am_hal_sysctrl_fpu_stacking_enable(true);

	spi_init(&spi, 0, 2000000);
	spi_enable(&spi);
	am1815_init(&rtc, &spi);
	asimple_littlefs_init(&fs, &flash);

	int err = asimple_littlefs_mount(&fs);
	if (err < 0)
	{
		asimple_littlefs_format(&fs);
		asimple_littlefs_mount(&fs);
	}
	flash_init(&flash, &spi);
	syscalls_rtc_init(&rtc);
	syscalls_uart_init(&uart);
	syscalls_littlefs_init(&fs);
	power_control_init(&power_control, 12);
	// FIXME remove debug
	uart_init(&uart, UART_INST0);
	// FIXME remove debug
	am_util_stdio_printf("HELLO\r\n");

	scron_init(&scron, &tasks);
	scron_load(&scron, load_callback);

	// After init is done, enable interrupts
	am_hal_interrupt_master_enable();
}

__attribute__((destructor))
static void redboard_shutdown(void)
{
	scron_save(&scron, save_callback);
	power_control_shutdown(&power_control);
}

int main(void)
{
	for(;;)
	{
		// Get timestamp from RTC and last task run
		// Get current battery voltage FIXME
		double current_voltage = 2.0;
		struct timeval now;
		gettimeofday(&now, NULL);
		time_t now_s = now.tv_sec;
		// FIXME remove debug
		am_util_stdio_printf("seconds: %i\r\n", (int)now.tv_sec);
		// FIXME remove debug
		am_util_stdio_printf("us: %i\r\n", (int)now.tv_usec);
		bool ran_task = artemia_scheduler(&scron, current_voltage, now_s);
		if (!ran_task)
		{
			// Reconfigure the alarm
			time_t next = scron_next_time(&scron);
			// FIXME update am1815 RTC alarm
			break;
		}
	}
}

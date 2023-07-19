// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Gabriel Marcano, 2023

#include <scron.h>
#include <power_control.h>

#include <adc.h>
#include <spi.h>
#include <lora.h>
#include <gpio.h>
#include <uart.h>
#include <am1815.h>
#include <syscalls.h>

#include "am_mcu_apollo.h"
#include "am_bsp.h"
#include "am_util.h"

#include <sys/time.h>

#include <string.h>
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <time.h>

#define CHECK_ERRORS(x)\
	if ((x) != AM_HAL_STATUS_SUCCESS)\
	{\
		error_handler(x);\
	}

static void error_handler(uint32_t error)
{
	(void)error;
	for(;;)
	{
		am_devices_led_on(am_bsp_psLEDs, 0);
		am_util_delay_ms(500);
		am_devices_led_off(am_bsp_psLEDs, 0);
		am_util_delay_ms(500);
	}
}

struct spi spi;
struct gpio shd;
struct am1815 rtc;
struct power_control power_control;

static int task1(void *data)
{
	return 0;
}

// FIXME maybe this should be the idle task
static int task_manage_trickle(void* data)
{
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
	.size = sizeof(tasks_)/sizeof(tasks_[0]),
};

struct scron scron;

static const size_t tasks_size = 2;

struct uart uart;

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
	initialize_time(&rtc);
	power_control_init(&power_control, 12);
	uart_init(&uart, UART_INST0);
	am_util_stdio_printf("HELLO\r\n");

	scron_init(&scron, &tasks);

	// After init is done, enable interrupts
	am_hal_interrupt_master_enable();
}

__attribute__((destructor))
static void redboard_shutdown(void)
{
	power_control_shutdown(&power_control);
}

int main(void)
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
	initialize_time(&rtc);
	power_control_init(&power_control, 12);
	uart_init(&uart, UART_INST0);
	am_util_stdio_printf("HELLO\r\n");

	// After init is done, enable interrupts
	am_hal_interrupt_master_enable();

	// Wait here for the ISR to grab a buffer of samples.
	while (1)
	{
		// Get timestamp from RTC and last task run
		// Get current battery voltage
		double current_voltage = 2.0;
		struct timeval now;
		gettimeofday(&now, NULL);
		time_t now_s = now.tv_sec;
		am_util_stdio_printf("seconds: %i\r\n", (int)now.tv_sec);
		am_util_stdio_printf("us: %i\r\n", (int)now.tv_usec);
		// for every task, check all overdue tasks, and run the most overdue one
		for (size_t i = 0; i < tasks_size; ++i)
		{
			time_t next = scron_schedule_next_time(&tasks_[i].schedule, scron.history[i].last_run);
			if (difftime(next, now_s) <= 0 && tasks_[i].minimum_voltage >= current_voltage)
			{
				am_util_stdio_printf("executing %i\r\n", i);
				// Mark for execution
				scron.history[i].last_run = now_s;
				tasks_[i].function(&now_s);
				// FIXME get current voltage
				current_voltage -= .01;
			}
		}

		// FIXME send shutdown signal

		am_util_delay_ms(5000);
		//am_hal_sysctrl_sleep(AM_HAL_SYSCTRL_SLEEP_DEEP);
	}
}

// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Gabriel Marcano, 2023

#include <gpio.h>

struct power_control
{
	struct gpio shd;
};

void power_control_init(struct power_control *control, uint8_t shd_pin)
{
	gpio_init(&control->shd, shd_pin, GPIO_MODE_OUTPUT, 0);
}

void power_control_shutdown(struct power_control *control)
{
	gpio_set(&control->shd, true);
}

// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Gabriel Marcano, 2023

#ifndef POWER_CONTROL_H_
#define POWER_CONTROL_H_

#include <gpio.h>

struct power_control
{
	struct gpio shd;
};

void power_control_init(struct power_control *control, uint8_t shd_pin);

void power_control_shutdown(struct power_control *control);

#endif//POWER_CONTROL_H_

#pragma once

#include "cw2015.h"
#include "freertos/FreeRTOS.h"
#include "i2c.h"
#include <stdint.h>

extern uint16_t battery_millivolts;
extern int battery_percentage;

void startBatteryTask();
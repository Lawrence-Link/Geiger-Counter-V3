#pragma once
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "cw2015.h"
#include "pin_definitions.h"

extern i2c_master_dev_handle_t cw2015_dev;
extern i2c_master_bus_handle_t i2c_bus;

void I2C_Devices_Init();
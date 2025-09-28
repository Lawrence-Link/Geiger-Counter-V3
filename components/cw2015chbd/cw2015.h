#pragma once

#include "driver/i2c_master.h"
#include "esp_err.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CW2015_I2C_ADDR       0x62

#define CW2015_REG_VERSION    0x00
#define CW2015_REG_VCELL      0x02
#define CW2015_REG_SOC        0x04
#define CW2015_REG_RRT_ALERT  0x06
#define CW2015_REG_CONFIG     0x08
#define CW2015_REG_MODE       0x0A
#define CW2015_REG_BATINFO    0x10  // length = 64 bytes

/**
 * @brief Attach CW2015 to an existing I2C device handle
 *
 * @param dev_handle device handle created by i2c_master_bus_add_device()
 * @return ESP_OK on success
 */
esp_err_t cw2015_init(i2c_master_dev_handle_t dev_handle);

/**
 * @brief Read battery voltage (mV)
 */
esp_err_t cw2015_read_vcell_mv(uint16_t *vcell_mv);

/**
 * @brief Read battery state of charge (percent)
 */
esp_err_t cw2015_read_soc(int *soc);

void dump_cw2015_regs(i2c_master_dev_handle_t dev);

#ifdef __cplusplus
}
#endif

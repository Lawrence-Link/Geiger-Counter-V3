#include "i2c.h"

i2c_master_bus_handle_t i2c_bus;
i2c_master_dev_handle_t cw2015_dev;
i2c_master_dev_handle_t pcf8563_dev;

void I2C_Devices_Init() {

    // I2C bus initialization
    i2c_master_bus_config_t bus_cfg = {
        .i2c_port = 0,
        .sda_io_num = PIN_I2C_SDA,
        .scl_io_num = PIN_I2C_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
    };

    bus_cfg.flags.enable_internal_pullup = true;
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_cfg, &i2c_bus));

    // register CW2015 Monitor IC
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7, 
        .device_address = CW2015_I2C_ADDR,
        .scl_speed_hz = 100000,
        .scl_wait_us = 0,
        .flags = 0,
    };    
    ESP_ERROR_CHECK(i2c_master_bus_add_device(i2c_bus, &dev_cfg, &cw2015_dev));
    ESP_ERROR_CHECK(cw2015_init(cw2015_dev));

    // register RTC
    i2c_device_config_t dev_cfg_pcf8563 = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7, 
        .device_address = PCF8563_I2C_ADDR, 
        .scl_speed_hz = 100000,
        .scl_wait_us = 0,
        .flags = 0,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(i2c_bus, &dev_cfg_pcf8563, &pcf8563_dev));
    
    // i2c_device_config_t dev_cfg_bme280 = {
    //     .dev_addr_length = I2C_ADDR_BIT_LEN_7, 
    //     .device_address = BME280_I2C_ADDR, 
    //     .scl_speed_hz = 100000,
    //     .scl_wait_us = 0,
    //     .flags = 0,
    // };
    // ESP_ERROR_CHECK(i2c_master_bus_add_device(i2c_bus, &dev_cfg_bme280, &bme280_dev));
    ESP_ERROR_CHECK(bme280_sensor_init(i2c_bus));
}
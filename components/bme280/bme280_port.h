// bme280_port.h
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "bme280.h"

#ifdef __cplusplus
extern "C" {
#endif

// BME280设备地址
#define BME280_I2C_ADDR 0x76

// 全局设备句柄声明
extern i2c_master_dev_handle_t bme280_dev;

// 统一初始化函数
esp_err_t bme280_sensor_init(i2c_master_bus_handle_t bus_handle);

// 传感器数据读取函数
float bme280_read_temperature_celsius(void);
float bme280_read_humidity_percentage(void);
float bme280_read_barometer(void);

// 任务控制函数
esp_err_t bme280_start_reading(void);
esp_err_t bme280_stop_reading(void);
bool bme280_is_reading(void);

// 反初始化函数
esp_err_t bme280_deinit(void);

#ifdef __cplusplus
}

class BME280_Port {
private:
    static i2c_master_dev_handle_t dev_handle;
    static TaskHandle_t task_handle;
    static bool is_running;
    static struct bme280_dev dev;
    static struct bme280_settings settings;
    static float latest_temperature;
    static float latest_humidity;
    static float latest_pressure;
    
    // 内部函数
    static void read_task(void* parameter);
    static bool init_sensor();
    static bool read_sensor_data();
    
    // BME280接口函数 - 匹配原始代码的函数签名
    static BME280_INTF_RET_TYPE i2c_read(uint8_t reg_addr, uint8_t *data, uint32_t len);
    static BME280_INTF_RET_TYPE i2c_write(uint8_t reg_addr, uint8_t *data);
    static void delay_us(uint32_t period, void *intf_ptr);
    static void print_error_result(const char api_name[], int8_t rslt);
    
public:
    BME280_Port() = default;
    ~BME280_Port() = default;
    
    // 禁止拷贝和赋值
    BME280_Port(const BME280_Port&) = delete;
    BME280_Port& operator=(const BME280_Port&) = delete;
    
    // 静态初始化函数（模仿其他设备注册风格）
    static esp_err_t init(i2c_master_bus_handle_t bus_handle);
    static esp_err_t deinit();
    
    // 任务控制
    static esp_err_t start_reading();
    static esp_err_t stop_reading();
    static bool is_reading_active() { return is_running; }
    
    // 数据读取接口
    static float read_temperature_celsius();
    static float read_humidity_percentage();
    static float read_barometer();
    
    // 获取设备句柄（用于外部访问）
    static i2c_master_dev_handle_t get_device_handle() { return dev_handle; }
};

#endif
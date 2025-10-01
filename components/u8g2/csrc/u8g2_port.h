/**
 * @file u8g2_port.h
 * @brief U8G2 ESP32-C6 SPI移植层头文件
 * @author Assistant
 * @date 2024
 */

#ifndef U8G2_PORT_H
#define U8G2_PORT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdint.h>
#include "esp_err.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "u8g2.h"

// SPI配置
#define U8G2_SPI_HOST               SPI2_HOST
#define U8G2_SPI_CLOCK_SPEED_HZ     20000000    // 10MHz
#define U8G2_SPI_QUEUE_SIZE         12

// GPIO引脚定义
#define U8G2_PIN_DC                 19         // 数据/命令选择引脚
#define U8G2_PIN_SCLK               22         // SPI时钟引脚
#define U8G2_PIN_MOSI               23         // SPI数据引脚
#define U8G2_PIN_CS                 -1         // CS接地，不使用

// 显示器配置
#define U8G2_DISPLAY_WIDTH          128
#define U8G2_DISPLAY_HEIGHT         64

/**
 * @brief U8G2移植层初始化
 * @return ESP_OK 成功, 其他值 失败
 */
esp_err_t u8g2_port_init(void);

/**
 * @brief U8G2移植层去初始化
 * @return ESP_OK 成功, 其他值 失败
 */
esp_err_t u8g2_port_deinit(void);

/**
 * @brief 初始化U8G2和SH1106显示器
 * @param u8g2 U8G2结构体指针
 * @return ESP_OK 成功, 其他值 失败
 */
esp_err_t u8g2_init_sh1106(u8g2_t *u8g2);

/**
 * @brief SH1106软件复位
 * @return ESP_OK 成功, 其他值 失败
 */
esp_err_t sh1106_software_reset(void);

/**
 * @brief U8G2回调函数 - SPI通信
 */
uint8_t u8g2_esp32_spi_byte_cb(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);

/**
 * @brief U8G2回调函数 - GPIO和延时
 */
uint8_t u8g2_esp32_gpio_and_delay_cb(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);

#ifdef __cplusplus
}
#endif

#endif // U8G2_PORT_H
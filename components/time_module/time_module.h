#pragma once

#include <driver/i2c_master.h>
#include <stdbool.h>
#include <time.h>
#include <esp_err.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PCF8563_I2C_ADDR 0x51

// --- 时钟输出频率枚举 ---
typedef enum
{
    PCF8563_DISABLED = 0,
    PCF8563_32768HZ,
    PCF8563_1024HZ,
    PCF8563_32HZ,
    PCF8563_1HZ,
} pcf8563_clkout_freq_t;

// --- 定时器时钟频率枚举 ---
typedef enum
{
    PCF8563_TIMER_4096HZ = 0,
    PCF8563_TIMER_64HZ,
    PCF8563_TIMER_1HZ,
    PCF8563_TIMER_1_60HZ
} pcf8563_timer_clock_t;

// --- 闹钟匹配标志位枚举 ---
typedef enum
{
    PCF8563_ALARM_MATCH_MIN     = 0x01,
    PCF8563_ALARM_MATCH_HOUR    = 0x02,
    PCF8563_ALARM_MATCH_DAY     = 0x04,
    PCF8563_ALARM_MATCH_WEEKDAY = 0x08
} pcf8563_alarm_flags_t;

esp_err_t pcf8563_init_desc(i2c_master_dev_handle_t dev);
esp_err_t pcf8563_free_desc(i2c_master_dev_handle_t dev);

esp_err_t pcf8563_set_time(i2c_master_dev_handle_t dev, struct tm *time);
esp_err_t pcf8563_get_time(i2c_master_dev_handle_t dev, struct tm *time, bool *valid);

esp_err_t pcf8563_set_clkout(i2c_master_dev_handle_t dev, pcf8563_clkout_freq_t freq);
esp_err_t pcf8563_get_clkout(i2c_master_dev_handle_t dev, pcf8563_clkout_freq_t *freq);

esp_err_t pcf8563_set_timer_settings(i2c_master_dev_handle_t dev, bool int_enable, pcf8563_timer_clock_t clock);
esp_err_t pcf8563_get_timer_settings(i2c_master_dev_handle_t dev, bool *int_enabled, pcf8563_timer_clock_t *clock);

esp_err_t pcf8563_set_timer_value(i2c_master_dev_handle_t dev, uint8_t value);
esp_err_t pcf8563_get_timer_value(i2c_master_dev_handle_t dev, uint8_t *value);

esp_err_t pcf8563_start_timer(i2c_master_dev_handle_t dev);
esp_err_t pcf8563_stop_timer(i2c_master_dev_handle_t dev);

esp_err_t pcf8563_get_timer_flag(i2c_master_dev_handle_t dev, bool *timer);
esp_err_t pcf8563_clear_timer_flag(i2c_master_dev_handle_t dev);

esp_err_t pcf8563_set_alarm(i2c_master_dev_handle_t dev, bool int_enable, uint32_t flags, struct tm *time);
esp_err_t pcf8563_get_alarm(i2c_master_dev_handle_t dev, bool *int_enabled, uint32_t *flags, struct tm *time);

esp_err_t pcf8563_get_alarm_flag(i2c_master_dev_handle_t dev, bool *alarm);
esp_err_t pcf8563_clear_alarm_flag(i2c_master_dev_handle_t dev);

#ifdef __cplusplus
}
#endif
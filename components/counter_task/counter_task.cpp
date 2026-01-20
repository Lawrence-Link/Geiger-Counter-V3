#include <atomic>
#include <driver/pulse_cnt.h>
#include <driver/gpio.h>
#include <esp_log.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>
#include "tune.h"
#include "counter_task.h"
#include "led.h"
#include "system_nvs_varibles.h"

static const char *TAG = "counter_task";

#define GPIO_INPUT_PIN      (GPIO_NUM_0)
#define PCNT_HIGH_LIMIT     32767
#define PCNT_LOW_LIMIT      -32768

static TaskHandle_t s_counter_task_handle = NULL;

static pcnt_unit_handle_t s_pcnt_unit = NULL;
static pcnt_channel_handle_t s_pcnt_chan = NULL;
static SemaphoreHandle_t s_cpm_mutex = NULL;
static uint32_t g_cpm_x100 = 0;   

bool pulse_marker = false;

static void counter_task(void *pvParameters) 
{
    auto& syscfg = SystemConf::getInstance();
    auto& tune = Tune::getInstance();
    auto& led = LedBlinker::getInstance();

    int32_t warn_th = syscfg.read_conf_warn_threshold() * 100;
    int32_t dngr_th = syscfg.read_conf_dngr_threshold() * 100;
    
    float smoothed_cpm = 0.0f;
    int last_raw_count = 0;
    int64_t last_sample_time = esp_timer_get_time(); 
    
    uint32_t loop_count = 0;
    uint32_t accum_pulses_500ms = 0;

    ESP_LOGI(TAG, "Counter task started with precision timing.");

    while (true)
    {
        vTaskDelay(pdMS_TO_TICKS(10));

        int current_raw_count = 0;
        ESP_ERROR_CHECK(pcnt_unit_get_count(s_pcnt_unit, &current_raw_count));

        int delta_pulses = current_raw_count - last_raw_count;
        if (delta_pulses < 0) { // to deal with counter overflow
            delta_pulses += (PCNT_HIGH_LIMIT - PCNT_LOW_LIMIT + 1);
        }
        last_raw_count = current_raw_count;

        if (delta_pulses > 0) // new pulse occurred
        {
            accum_pulses_500ms += delta_pulses;
            pulse_marker = true;
            
            if (syscfg.read_conf_enable_geiger_click()) {
                tune.geigerClick();
            }
            if (syscfg.read_conf_enable_blink()) {
                if (smoothed_cpm >= dngr_th) led.enqueueBlink(LedColor::RED);
                else if (smoothed_cpm >= warn_th) led.enqueueBlink(LedColor::YELLOW);
                else led.enqueueBlink(LedColor::GREEN);
            }
        }

        if (++loop_count >= 50) 
        {
            int64_t now = esp_timer_get_time();
            // 实际经过时间（秒）
            float real_delta_t = (float)(now - last_sample_time) / 1000000.0f;
            
            // CPM: (脉冲数 / 实际秒数) * 60
            float input_cpm = ((float)accum_pulses_500ms / real_delta_t) * 60.0f;

            // 动态平滑系数
            float alpha = (smoothed_cpm < 40.0f) ? 0.04f : 
                          (smoothed_cpm < 200.0f) ? 0.12f : 
                          (smoothed_cpm < 1000.0f) ? 0.20f : 0.70f;

            smoothed_cpm += alpha * (input_cpm - smoothed_cpm);

            if (xSemaphoreTake(s_cpm_mutex, pdMS_TO_TICKS(5)) == pdTRUE) 
            {
                g_cpm_x100 = (uint32_t)(smoothed_cpm * 100.0f);
                xSemaphoreGive(s_cpm_mutex);
            }

            accum_pulses_500ms = 0;
            loop_count = 0;
            last_sample_time = now; 
        }
    }
}

bool start_counter_task(const counter_task_config_t* config) 
{
    if (s_counter_task_handle != NULL) return true;

    s_cpm_mutex = xSemaphoreCreateMutex();

    gpio_config_t io_conf = 
    {
        .pin_bit_mask = (1ULL << GPIO_INPUT_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,   
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE      
    };
    gpio_config(&io_conf);

    pcnt_unit_config_t unit_config = 
    {
        .low_limit = PCNT_LOW_LIMIT,
        .high_limit = PCNT_HIGH_LIMIT,
        .flags = { .accum_count = true },
    };
    ESP_ERROR_CHECK(pcnt_new_unit(&unit_config, &s_pcnt_unit));

    pcnt_chan_config_t chan_config = 
    {
        .edge_gpio_num = GPIO_INPUT_PIN,
        .level_gpio_num = -1,
    };
    ESP_ERROR_CHECK(pcnt_new_channel(s_pcnt_unit, &chan_config, &s_pcnt_chan));

    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(s_pcnt_chan, 
                                                PCNT_CHANNEL_EDGE_ACTION_HOLD, 
                                                PCNT_CHANNEL_EDGE_ACTION_INCREASE));

    pcnt_glitch_filter_config_t filter_config = 
    {
        .max_glitch_ns = 0,
    };
    ESP_ERROR_CHECK(pcnt_unit_set_glitch_filter(s_pcnt_unit, &filter_config));

    ESP_ERROR_CHECK(pcnt_unit_enable(s_pcnt_unit));
    ESP_ERROR_CHECK(pcnt_unit_clear_count(s_pcnt_unit));
    ESP_ERROR_CHECK(pcnt_unit_start(s_pcnt_unit));

    xTaskCreate(counter_task, "GeigerTask", 4096, NULL, 
                configMAX_PRIORITIES - 2, &s_counter_task_handle);
    return true;
}

bool stop_counter_task(void) 
{
    if (s_counter_task_handle) 
    {
        vTaskDelete(s_counter_task_handle);
        s_counter_task_handle = NULL;
    }

    if (s_pcnt_unit) 
    {
        ESP_ERROR_CHECK(pcnt_unit_stop(s_pcnt_unit));
        ESP_ERROR_CHECK(pcnt_unit_disable(s_pcnt_unit));
        if (s_pcnt_chan) 
        {
            ESP_ERROR_CHECK(pcnt_del_channel(s_pcnt_chan));
            s_pcnt_chan = NULL;
        }
        ESP_ERROR_CHECK(pcnt_del_unit(s_pcnt_unit));
        s_pcnt_unit = NULL;
    }

    ESP_LOGI(TAG, "Counter task stopped.");

    if (s_cpm_mutex) 
    {
        vSemaphoreDelete(s_cpm_mutex);
        s_cpm_mutex = NULL;
    }
    return true;
}

float get_current_cpm(void) 
{
    float result = 0.0f;
    if (s_cpm_mutex && xSemaphoreTake(s_cpm_mutex, pdMS_TO_TICKS(10))) 
    {
        result = (float)g_cpm_x100 / 100.0f;
        xSemaphoreGive(s_cpm_mutex);
    }
    return result;
}
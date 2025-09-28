#include "counter_task.h"

#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include <cmath>

// --- 常量定义 ---
static const char *TAG = "GEIGER_COUNTER";
#define GPIO_INPUT_PIN (GPIO_NUM_0) // 脉冲输入 GPIO
#define RING_BUFFER_SIZE (500)      // 存储 500 个时间戳 (滑动窗口大小)
#define NUM_FOR_DENSITY_CHECK (15)  // 用于瞬时密度检查的脉冲数 (最近15次)
#define QUEUE_SIZE (50)             // 队列深度，用于缓冲 ISR 来的脉冲

// --- 任务和同步句柄 (保持不变) ---
static TaskHandle_t s_counter_task_handle = NULL;
static QueueHandle_t s_timestamp_queue = NULL; 
static SemaphoreHandle_t s_cpm_mutex = NULL; 

static float current_cpm = 0.0f; 

static bool use_startup_calculation;

// --- ISR：使用 FreeRTOS 队列传输时间戳 (保持不变) ---
static void IRAM_ATTR isr_geiger_pulse(void *arg) {
    int64_t timestamp = esp_timer_get_time();
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if (xQueueSendFromISR(s_timestamp_queue, &timestamp, &xHigherPriorityTaskWoken) != pdPASS) {
        // 脉冲丢失，不执行复杂操作
    }

    if (xHigherPriorityTaskWoken == pdTRUE) {
        portYIELD_FROM_ISR();
    }
}

static void counter_task(void *pvParameters) {
    // --- 核心数据结构 ---
    int64_t timestamp_history[RING_BUFFER_SIZE];
    size_t write_index = 0;
    bool buffer_full = false;
    // 使用 C99/C++11 标准，但 ESP-IDF 环境通常支持 memset
    memset(timestamp_history, 0, sizeof(timestamp_history)); 

    // --- 启动模式变量 ---
    int64_t first_pulse_time = 0;
    bool first_pulse_received = false;

    // --- 稳定性控制变量 ---
    int64_t last_window_switch_time = 0;
    int64_t current_window_us = 10000000;  // 初始10秒窗口
    float cpm_history[5] = {0};            // 内部平滑用的历史记录
    int cpm_history_index = 0;
    int cpm_history_count = 0;
    float last_stable_cpm = 0.0f;          // 用于窗口切换判断的稳定值
    
    // --- 定时平均输出变量 ---
    int64_t last_output_time = 0;
    float cpm_accumulator = 0.0f;
    int cpm_average_count = 0;
    const int64_t OUTPUT_INTERVAL_US = 1000000;   // 1秒的微秒数

    ESP_LOGI(TAG, "Counter task started. Local array size: %zu bytes.", sizeof(timestamp_history));

    while (true) {
        int64_t new_timestamp;
        
        // 1. 非阻塞接收脉冲
        if (xQueueReceive(s_timestamp_queue, &new_timestamp, 0) == pdPASS) {
            // 用于保存本次循环计算出的有效 CPM 值
            float calculated_cpm = -1.0f; 

            if (!first_pulse_received) {
                first_pulse_received = true;
                first_pulse_time = new_timestamp;
                last_output_time = new_timestamp; // 初始化定时器
            }

            // 更新环形缓冲区
            timestamp_history[write_index] = new_timestamp;
            write_index = (write_index + 1) % RING_BUFFER_SIZE;
            if (write_index == 0) {
                buffer_full = true;
            }

            // --- 启动模式计算：脉冲数少于 NUM_FOR_DENSITY_CHECK 且缓冲区未满时 ---
            use_startup_calculation = !buffer_full && write_index < NUM_FOR_DENSITY_CHECK;
            
            if (use_startup_calculation && first_pulse_received) {
                int64_t elapsed_time_us = new_timestamp - first_pulse_time;
                
                // 确保有足够的时间间隔（至少1秒）
                if (elapsed_time_us >= 1000000) {  
                    size_t total_pulses = write_index;  
                    float elapsed_time_minutes = (float)elapsed_time_us / 60000000.0f;
                    calculated_cpm = (float)total_pulses / elapsed_time_minutes;
                }
            }
            
            // --- 正常模式计算：脉冲数足够时 ---
            if (buffer_full || write_index >= NUM_FOR_DENSITY_CHECK) {
                size_t oldest_idx_offset = NUM_FOR_DENSITY_CHECK;
                size_t oldest_idx;
                
                // 计算 NUM_FOR_DENSITY_CHECK 个脉冲前的时间戳索引
                if (write_index >= oldest_idx_offset) {
                    oldest_idx = write_index - oldest_idx_offset;
                } else {
                    oldest_idx = RING_BUFFER_SIZE - (oldest_idx_offset - write_index);
                }
                
                int64_t T_latest = new_timestamp;
                int64_t T_oldest = timestamp_history[oldest_idx];
                int64_t time_diff_us = T_latest - T_oldest;

                if (time_diff_us > 0) {
                    // 计算瞬时 CPM (用于窗口选择)
                    float instantaneous_cpm =
                        (float)(NUM_FOR_DENSITY_CHECK - 1) / ((float)time_diff_us / 60000000.0f);

                    // --- 动态窗口切换控制逻辑 ---
                    int64_t optimal_window_us;
                    if (instantaneous_cpm <= 30.0f)        optimal_window_us = 20000000;  // 20s
                    else if (instantaneous_cpm <= 100.0f)  optimal_window_us = 10000000;  // 10s
                    else if (instantaneous_cpm <= 300.0f)  optimal_window_us = 5000000;   // 5s
                    else if (instantaneous_cpm <= 1000.0f) optimal_window_us = 3000000;   // 3s
                    else if (instantaneous_cpm <= 3000.0f) optimal_window_us = 2000000;   // 2s
                    else if (instantaneous_cpm <= 10000.0f)optimal_window_us = 1000000;   // 1s
                    else if (instantaneous_cpm <= 30000.0f)optimal_window_us = 500000;    // 0.5s
                    else optimal_window_us = 200000;    // 0.2s

                    int64_t time_since_last_switch = new_timestamp - last_window_switch_time;
                    if (time_since_last_switch > 5000000) { // 至少5秒才能切换一次
                        float window_ratio = (float)optimal_window_us / (float)current_window_us;
                        if (window_ratio < 0.7f || window_ratio > 1.4f) {
                            float cpm_change = fabsf(instantaneous_cpm - last_stable_cpm);
                            float relative_change = cpm_change / (last_stable_cpm + 10.0f);
                            if (relative_change > 0.5f) {
                                current_window_us = optimal_window_us;
                                last_window_switch_time = new_timestamp;
                                // ESP_LOGW(TAG, "Window switched to %.1fs (Inst. CPM: %.1f)",
                                //          (float)current_window_us / 1000000.0f, instantaneous_cpm);
                                cpm_history_count = 0; // 重置平滑历史
                                cpm_history_index = 0;
                            }
                        }
                    }

                    // --- 使用当前窗口计算 CPM ---
                    int64_t target_time = new_timestamp - current_window_us;
                    size_t pulses_count = 0;
                    int64_t T_end = T_latest, T_start = 0;
                    size_t search_idx = (write_index == 0) ? RING_BUFFER_SIZE - 1 : write_index - 1;
                    
                    // 向后遍历，找到第一个早于 target_time 的脉冲
                    for (int j = 0; j < RING_BUFFER_SIZE; ++j) {
                        size_t index = (search_idx - j + RING_BUFFER_SIZE) % RING_BUFFER_SIZE;
                        if (!buffer_full && index >= write_index && j > 0) break;
                        int64_t current_ts = timestamp_history[index];
                        if (current_ts >= target_time) {
                            pulses_count++; T_start = current_ts;
                        } else break;
                        if (buffer_full && j == RING_BUFFER_SIZE - 1) T_start = current_ts;
                    }

                    // CPM计算与平滑
                    if (pulses_count > 0 && T_end - T_start > 0) {
                        float raw_cpm = (float)pulses_count / ((float)(T_end - T_start) / 60000000.0f);
                        
                        // 移动平均平滑
                        cpm_history[cpm_history_index] = raw_cpm;
                        cpm_history_index = (cpm_history_index + 1) % 5;
                        if (cpm_history_count < 5) cpm_history_count++;
                        
                        float smoothed_cpm = 0.0f;
                        for (int i = 0; i < cpm_history_count; i++) smoothed_cpm += cpm_history[i];
                        smoothed_cpm /= cpm_history_count;

                        last_stable_cpm = smoothed_cpm; // 更新用于窗口切换判断的稳定值
                        calculated_cpm = smoothed_cpm;  // 标记为已计算
                    }
                }
            }

            // *** 统一将计算出的 CPM 加入 1 秒平均累加器 ***
            if (calculated_cpm > 0) {
                cpm_accumulator += calculated_cpm;
                cpm_average_count++;
            }
        } else {
            vTaskDelay(pdMS_TO_TICKS(10)); // 队列为空时短暂休眠
        }

        // 2. 定时输出和更新全局 CPM
        int64_t current_time = esp_timer_get_time();
        
        // 只有在第一次脉冲收到后，才开始定时输出检查
        if (first_pulse_received && (current_time - last_output_time >= OUTPUT_INTERVAL_US)) {
            
            if (cpm_average_count > 0) {
                // 如果有新样本，计算平均值并更新
                float final_avg_cpm = cpm_accumulator / cpm_average_count;
                
                if (xSemaphoreTake(s_cpm_mutex, portMAX_DELAY) == pdTRUE) {
                    current_cpm = final_avg_cpm;
                    xSemaphoreGive(s_cpm_mutex);
                }
                
                // 重置累加器和计数器
                cpm_accumulator = 0.0f;
                cpm_average_count = 0;
                
            }
            last_output_time = current_time;
        }
    }
}

bool start_counter_task(const counter_task_config_t* config) {
    if (s_counter_task_handle != NULL) {
        ESP_LOGW(TAG, "Task already running.");
        return true;
    }

    // 1. 创建互斥锁和队列
    s_cpm_mutex = xSemaphoreCreateMutex();
    s_timestamp_queue = xQueueCreate(QUEUE_SIZE, sizeof(int64_t)); 
    
    if (s_cpm_mutex == NULL || s_timestamp_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create FreeRTOS resources.");
        stop_counter_task(); // 清理
        return false;
    }

    // 2. 配置 GPIO 
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_NEGEDGE; 
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << GPIO_INPUT_PIN);
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE; 
    gpio_config(&io_conf);

    // 3. 安装 ISR 服务并绑定 ISR 
    gpio_install_isr_service(0); 
    gpio_isr_handler_add(GPIO_INPUT_PIN, isr_geiger_pulse, NULL);

    // 4. 创建任务
    // 栈大小设置为 8192，以容纳 4000 字节的局部数组并提供足够的裕量
    if (xTaskCreate(counter_task, "GeigerCounterTa", 8192, NULL, 
                    configMAX_PRIORITIES - 5, &s_counter_task_handle) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create counter task.");
        stop_counter_task(); // 清理
        return false;
    }

    ESP_LOGI(TAG, "Counter task and ISR started successfully on GPIO%d. Stack size: 8192 bytes.", GPIO_INPUT_PIN);
    return true;
}

bool stop_counter_task() {
    // 1. 删除任务
    if (s_counter_task_handle != NULL) {
        vTaskDelete(s_counter_task_handle);
        s_counter_task_handle = NULL;
    }

    // 2. 移除 ISR
    gpio_isr_handler_remove(GPIO_INPUT_PIN);

    // 3. 释放 FreeRTOS 资源
    if (s_timestamp_queue != NULL) {
        vQueueDelete(s_timestamp_queue);
        s_timestamp_queue = NULL;
    }
    if (s_cpm_mutex != NULL) {
        vSemaphoreDelete(s_cpm_mutex);
        s_cpm_mutex = NULL;
    }

    // 4. 重置 GPIO 模式 (可选)
    gpio_reset_pin(GPIO_INPUT_PIN);
    
    ESP_LOGI(TAG, "Counter task and resources stopped.");
    return true;
}

bool is_startup_mode() 
{ 
    return use_startup_calculation; 
}

float get_current_cpm() {
    float cpm_result = 0.0f;
    if (s_cpm_mutex != NULL && xSemaphoreTake(s_cpm_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        cpm_result = current_cpm;
        xSemaphoreGive(s_cpm_mutex);
    } else {
        ESP_LOGW(TAG, "Failed to take CPM mutex.");
    }
    return cpm_result;
}
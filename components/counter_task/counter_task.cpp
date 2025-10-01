#include "counter_task.h"

#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "tune.h"
#include "led.h"
#include <cmath>

// --- Constants ---
static const char *TAG = "GEIGER_COUNTER";
#define GPIO_INPUT_PIN (GPIO_NUM_0) // Pulse input GPIO
#define RING_BUFFER_SIZE (500)      // Store 500 timestamps (sliding window size)
#define NUM_FOR_DENSITY_CHECK (15)  // Number of recent pulses for instantaneous density check
#define QUEUE_SIZE (50)             // Queue depth, used to buffer pulses from ISR

// --- Task and synchronization handles ---
static TaskHandle_t s_counter_task_handle = NULL;
static QueueHandle_t s_timestamp_queue = NULL; 
static SemaphoreHandle_t s_cpm_mutex = NULL; 

static float current_cpm = 0.0f; 

static bool use_startup_calculation;

// --- ISR: send timestamp to FreeRTOS queue ---
static void IRAM_ATTR isr_geiger_pulse(void *arg) {
    int64_t timestamp = esp_timer_get_time();
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if (xQueueSendFromISR(s_timestamp_queue, &timestamp, &xHigherPriorityTaskWoken) != pdPASS) {
        // Pulse lost, do not perform complex operations
    }

    if (xHigherPriorityTaskWoken == pdTRUE) {
        portYIELD_FROM_ISR();
    }
}

static void counter_task(void *pvParameters) {
    // --- Core data structures ---
    int64_t timestamp_history[RING_BUFFER_SIZE];
    size_t write_index = 0;
    bool buffer_full = false;
    memset(timestamp_history, 0, sizeof(timestamp_history)); 

    // --- Startup mode variables ---
    int64_t first_pulse_time = 0;
    bool first_pulse_received = false;

    // --- Window and stability control ---
    int64_t last_window_switch_time = 0;
    int64_t current_window_us = 10000000;  // Initial 10s window
    float cpm_history[5] = {0};            // History buffer for smoothing
    int cpm_history_index = 0;
    int cpm_history_count = 0;
    float last_stable_cpm = 0.0f;          // Stable CPM for window switching decisions
    
    // --- Periodic averaging ---
    int64_t last_output_time = 0;
    float cpm_accumulator = 0.0f;
    int cpm_average_count = 0;
    const int64_t OUTPUT_INTERVAL_US = 1000000;   // 1 second in microseconds

    ESP_LOGI(TAG, "Counter task started. Local array size: %zu bytes.", sizeof(timestamp_history));

    auto& tune = Tune::getInstance();

    while (true) {
        int64_t new_timestamp;
        
        // 1. Non-blocking pulse reception
        if (xQueueReceive(s_timestamp_queue, &new_timestamp, 0) == pdPASS) {

            tune.geigerClick();

            float calculated_cpm = -1.0f; // Temporary CPM value

            if (!first_pulse_received) {
                first_pulse_received = true;
                first_pulse_time = new_timestamp;
                last_output_time = new_timestamp; // Initialize timer
            }

            // Update ring buffer
            timestamp_history[write_index] = new_timestamp;
            write_index = (write_index + 1) % RING_BUFFER_SIZE;
            if (write_index == 0) {
                buffer_full = true;
            }

            // --- Startup calculation mode: before buffer fills and fewer than NUM_FOR_DENSITY_CHECK pulses ---
            use_startup_calculation = !buffer_full && write_index < NUM_FOR_DENSITY_CHECK;
            
            if (use_startup_calculation && first_pulse_received) {
                int64_t elapsed_time_us = new_timestamp - first_pulse_time;
                
                // Ensure at least 1 second has passed
                if (elapsed_time_us >= 1000000) {  
                    size_t total_pulses = write_index;  
                    float elapsed_time_minutes = (float)elapsed_time_us / 60000000.0f;
                    calculated_cpm = (float)total_pulses / elapsed_time_minutes;
                }
            }
            
            // --- Normal calculation mode: enough pulses collected ---
            if (buffer_full || write_index >= NUM_FOR_DENSITY_CHECK) {
                size_t oldest_idx_offset = NUM_FOR_DENSITY_CHECK;
                size_t oldest_idx;
                
                // Find index of pulse NUM_FOR_DENSITY_CHECK steps earlier
                if (write_index >= oldest_idx_offset) {
                    oldest_idx = write_index - oldest_idx_offset;
                } else {
                    oldest_idx = RING_BUFFER_SIZE - (oldest_idx_offset - write_index);
                }
                
                int64_t T_latest = new_timestamp;
                int64_t T_oldest = timestamp_history[oldest_idx];
                int64_t time_diff_us = T_latest - T_oldest;

                if (time_diff_us > 0) {
                    // Calculate instantaneous CPM (for window selection)
                    float instantaneous_cpm =
                        (float)(NUM_FOR_DENSITY_CHECK - 1) / ((float)time_diff_us / 60000000.0f);

                    // --- Dynamic window switching ---
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
                    if (time_since_last_switch > 5000000) { // At least 5s between switches
                        float window_ratio = (float)optimal_window_us / (float)current_window_us;
                        if (window_ratio < 0.7f || window_ratio > 1.4f) {
                            float cpm_change = fabsf(instantaneous_cpm - last_stable_cpm);
                            float relative_change = cpm_change / (last_stable_cpm + 10.0f);
                            if (relative_change > 0.5f) {
                                current_window_us = optimal_window_us;
                                last_window_switch_time = new_timestamp;
                                // Reset smoothing buffer
                                cpm_history_count = 0;
                                cpm_history_index = 0;
                            }
                        }
                    }

                    // --- Calculate CPM using current window ---
                    int64_t target_time = new_timestamp - current_window_us;
                    size_t pulses_count = 0;
                    int64_t T_end = T_latest, T_start = 0;
                    size_t search_idx = (write_index == 0) ? RING_BUFFER_SIZE - 1 : write_index - 1;
                    
                    // Traverse backwards until reaching a pulse older than target_time
                    for (int j = 0; j < RING_BUFFER_SIZE; ++j) {
                        size_t index = (search_idx - j + RING_BUFFER_SIZE) % RING_BUFFER_SIZE;
                        if (!buffer_full && index >= write_index && j > 0) break;
                        int64_t current_ts = timestamp_history[index];
                        if (current_ts >= target_time) {
                            pulses_count++; T_start = current_ts;
                        } else break;
                        if (buffer_full && j == RING_BUFFER_SIZE - 1) T_start = current_ts;
                    }

                    // CPM calculation and smoothing
                    if (pulses_count > 0 && T_end - T_start > 0) {
                        float raw_cpm = (float)pulses_count / ((float)(T_end - T_start) / 60000000.0f);
                        
                        // Moving average smoothing
                        cpm_history[cpm_history_index] = raw_cpm;
                        cpm_history_index = (cpm_history_index + 1) % 5;
                        if (cpm_history_count < 5) cpm_history_count++;
                        
                        float smoothed_cpm = 0.0f;
                        for (int i = 0; i < cpm_history_count; i++) smoothed_cpm += cpm_history[i];
                        smoothed_cpm /= cpm_history_count;

                        last_stable_cpm = smoothed_cpm;
                        calculated_cpm = smoothed_cpm;
                    }
                }
            }

            // Add calculated CPM to 1-second averaging accumulator
            if (calculated_cpm > 0) {
                cpm_accumulator += calculated_cpm;
                cpm_average_count++;
            }

            if (calculated_cpm < 300) {
                LedBlinker::getInstance().enqueueBlink(LedColor::GREEN);
            }
            else if (calculated_cpm < 600) {
                LedBlinker::getInstance().enqueueBlink(LedColor::YELLOW);
            }
            else if (calculated_cpm < 1000) {
                LedBlinker::getInstance().enqueueBlink(LedColor::RED);
            }
            else if (calculated_cpm < 2000) {
                LedBlinker::getInstance().enqueueBlink(LedColor::RED);
            }

        } else {
            vTaskDelay(pdMS_TO_TICKS(10)); // Sleep briefly if queue is empty
        }

        // 2. Periodic output and update global CPM
        int64_t current_time = esp_timer_get_time();
        
        if (first_pulse_received && (current_time - last_output_time >= OUTPUT_INTERVAL_US)) {
            
            if (cpm_average_count > 0) {
                float final_avg_cpm = cpm_accumulator / cpm_average_count;
                
                if (xSemaphoreTake(s_cpm_mutex, portMAX_DELAY) == pdTRUE) {
                    current_cpm = final_avg_cpm;
                    xSemaphoreGive(s_cpm_mutex);
                }
                
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

    // 1. Create mutex and queue
    s_cpm_mutex = xSemaphoreCreateMutex();
    s_timestamp_queue = xQueueCreate(QUEUE_SIZE, sizeof(int64_t)); 
    
    if (s_cpm_mutex == NULL || s_timestamp_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create FreeRTOS resources.");
        stop_counter_task();
        return false;
    }

    // 2. Configure GPIO 
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_NEGEDGE; 
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << GPIO_INPUT_PIN);
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE; 
    gpio_config(&io_conf);

    // 3. Install ISR service and bind ISR
    gpio_install_isr_service(0); 
    gpio_isr_handler_add(GPIO_INPUT_PIN, isr_geiger_pulse, NULL);

    // 4. Create task
    // Stack size 8192 to fit 4000-byte local array with extra margin
    if (xTaskCreate(counter_task, "GeigerCounterTa", 8192, NULL, 
                    configMAX_PRIORITIES - 5, &s_counter_task_handle) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create counter task.");
        stop_counter_task();
        return false;
    }

    ESP_LOGI(TAG, "Counter task and ISR started successfully on GPIO%d. Stack size: 8192 bytes.", GPIO_INPUT_PIN);
    return true;
}

bool stop_counter_task() {
    // 1. Delete task
    if (s_counter_task_handle != NULL) {
        vTaskDelete(s_counter_task_handle);
        s_counter_task_handle = NULL;
    }

    // 2. Remove ISR
    gpio_isr_handler_remove(GPIO_INPUT_PIN);

    // 3. Free FreeRTOS resources
    if (s_timestamp_queue != NULL) {
        vQueueDelete(s_timestamp_queue);
        s_timestamp_queue = NULL;
    }
    if (s_cpm_mutex != NULL) {
        vSemaphoreDelete(s_cpm_mutex);
        s_cpm_mutex = NULL;
    }

    // 4. Reset GPIO (optional)
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

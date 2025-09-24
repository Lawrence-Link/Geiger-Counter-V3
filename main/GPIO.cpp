#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "esp_err.h"

#include "GPIO_definitions.h"

#define PWM_GPIO        6       // Target GPIO (check if safe on your ESP32 variant)
#define PWM_FREQ_HZ     100000  // 100 kHz
#define PWM_DUTY_PCT    15      // 15% duty cycle
#define PWM_TIMER       LEDC_TIMER_0
#define PWM_CHANNEL     LEDC_CHANNEL_0

void GPIO_init() {
    
}
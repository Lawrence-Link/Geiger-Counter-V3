#include <stdio.h>
#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "u8g2_port.h"
#include "U8g2lib.h"

#include "PixelUI.h"
#include "ui/AppLauncher/AppLauncher.h"
#include "app_registry.h"

#include "pin_definitions.h"
#include "i2c.h"

#include "voltage_pid.hpp"
#include "encoder_task.h"
#include "ui_heartbeat_task.h"
#include "GPIO.h"
#include "battery.h"

// Global variables
U8G2 u8g2;
PixelUI ui(u8g2);
static const char *TAG = "main";

VoltagePID voltage_controller(
        /** ADC controller config **/
        ADC_UNIT_1,
        ADC_CHANNEL_2, 
        ADC_ATTEN_DB_12,    
        /** PWM generation config **/
        LEDC_CHANNEL_0,
        LEDC_TIMER_0,
        PIN_HV_DRIVE  
    );

void delay_ms(uint32_t ms) {
    vTaskDelay(pdMS_TO_TICKS(ms));
}

extern "C" void app_main(void)
{   
    // Initialize display
    esp_err_t ret = u8g2_init_sh1106(u8g2.getU8g2());
    if (ret != ESP_OK) {
        ESP_LOGI(TAG, "Failed to initialize U8G2: %s\n", esp_err_to_name(ret));
        return;
    }

    I2C_Devices_Init();
    GPIO_init();

    startBatteryTask();
    u8g2.setFont(u8g2_font_helvB08_tr);
    u8g2.drawStr(-1, 61, " By PixelUI");
    u8g2.sendBuffer();

    vTaskDelay(pdMS_TO_TICKS(1000));

    // Register applications
    registerApps();

    auto appView = AppLauncher::createAppLauncherView(ui, *ui.getViewManagerPtr());
    ui.getViewManagerPtr()->push(appView);

    // Initialize UI
    ui.setDelayFunction(delay_ms);
    ui.begin();

    start_encoder_task();
    start_ui_heartbeat_task();

    voltage_controller.setTarget(380.0f);
    voltage_controller.setPID(1.5f, 7.0f, 0.00f);  
    
    // === Main rendering loop ===
    InputEvent ev;
    for (;;) {
        while (xQueueReceive(input_event_queue, &ev, 0) == pdTRUE) {
            ui.handleInput(ev);
        }
        ui.renderer();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

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
#include "common.h"
#include "system_nvs_varibles.h"
#include "nvs.hpp"
#include "nvs_handle_espp.hpp"
#include "tune.h"

// Global variables
U8G2 u8g2;
PixelUI ui(u8g2);

extern AppItem charge_app;
extern AppItem sillycat_app;

static const char *TAG = "main";

QueueHandle_t ui_event_queue;

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

extern "C" void app_main(void) // mainly reserved for ui rendering
{   
    // Initialize display
    esp_err_t ret = u8g2_init_sh1106(u8g2.getU8g2());
    if (ret != ESP_OK) {
        ESP_LOGI(TAG, "Failed to initialize U8G2: %s\n", esp_err_to_name(ret));
        return;
    }

    I2C_Devices_Init();
    GPIO_init();

    // initialize nvs driver
    std::error_code ec;
    espp::Nvs nvs_driver;
    nvs_driver.init(ec);
    // load setup from nvs
    auto& syscfg = SystemConf::getInstance();
    syscfg.load_conf_from_nvs();
    u8g2.setContrast(syscfg.read_conf_brightness() * 51);

    Tune& tune = Tune::getInstance();

    Tune::Melody startup = {
        {Notes::B5, 80},
        {Notes::B5, 80},
    };

    Tune::Melody bluejay = {
        {Notes::B4, 105},      // 4b
        {Notes::REST, 26},     // p
        {Notes::E5, 105},      // 4e5
        {Notes::REST, 26},     // p
        {Notes::B4, 105},      // 4b
        {Notes::REST, 26},     // p
        {740, 105},            // 4f#5 (F#5)
        {Notes::REST, 210},    // 2p
        {Notes::E5, 105},      // 4e5
        {Notes::B5, 210},      // 2b5
        {Notes::B5, 52}        // 8b5
    };
    
    // 警报旋律
    Tune::Melody alarm = {
        {Notes::A5, Duration::EIGHTH},
        {Notes::REST, Duration::EIGHTH},
        {Notes::A5, Duration::EIGHTH},
        {Notes::REST, Duration::EIGHTH},
        {Notes::A5, Duration::EIGHTH},
        {Notes::REST, Duration::QUARTER}
    };

    if (syscfg.read_conf_enable_interaction_tone()) tune.playMelody(startup);

    startBatteryTask();
    u8g2.setFont(u8g2_font_helvB08_tr);
    u8g2.drawStr(1, 61, "By PixelUI");
    u8g2.sendBuffer();

    vTaskDelay(pdMS_TO_TICKS(1000));

    // Register applications
    registerApps();

    ui.getViewManagerPtr()->push(sillycat_app.createApp(ui));
    auto appView = AppLauncher::createAppLauncherView(ui, *ui.getViewManagerPtr());
    ui.getViewManagerPtr()->push(appView);

    // Initialize UI
    ui.setDelayFunction(delay_ms);
    ui.begin();

    start_encoder_task();
    start_ui_heartbeat_task();

    voltage_controller.setTarget(380.0f);
    voltage_controller.setPID(1.5f, 7.0f, 0.00f);  
    
    static uint32_t tickPrevChargingAnim = ui.getCurrentTime();
    static uint32_t tickNowChargingAnim = ui.getCurrentTime();
    bool countdown_enable_charging_anim = false;

    ui_event_queue = xQueueCreate(10, sizeof(UI_EVENT));

    // === Main rendering loop ===
    InputEvent in_ev;
    UI_EVENT ui_ev;
    for (;;) {
        tickNowChargingAnim = ui.getCurrentTime();
        while (xQueueReceive(input_event_queue, &in_ev, 0) == pdTRUE) {
            ui.handleInput(in_ev);
        }
        if(xQueueReceive(ui_event_queue, &ui_ev, 0) == pdTRUE) {
            countdown_enable_charging_anim = true;
            tickPrevChargingAnim = ui.getCurrentTime();
            tickNowChargingAnim = ui.getCurrentTime();
        }

        if (countdown_enable_charging_anim) {
            if (tickNowChargingAnim - tickPrevChargingAnim > 1000 && gpio_get_level(PIN_USB_STATUS)) {
                countdown_enable_charging_anim = false;
                ui.getViewManagerPtr()->push(charge_app.createApp(ui));
            }
        }
        ui.renderer();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

#include "battery.h"

uint16_t battery_millivolts = 0;
int battery_percentage = 0;

TaskHandle_t Battery_TaskHandle;

void battery_task( void *pv ) {
    ESP_ERROR_CHECK(cw2015_init(cw2015_dev)); // initialize battery monitor IC

    uint8_t wake_cmd[3] = { 0x0A, 0x00, 0x00 };
    i2c_master_transmit(cw2015_dev, wake_cmd, 3, pdMS_TO_TICKS(100));
    vTaskDelay(pdMS_TO_TICKS(200));
    
    while (1) {
        cw2015_read_vcell_mv(&battery_millivolts);
        cw2015_read_soc(&battery_percentage);

        vTaskDelay(pdMS_TO_TICKS(1500));
    }
}

void startBatteryTask() {
    if (Battery_TaskHandle == nullptr) {
        xTaskCreate(battery_task, "battery_acquisition_task", 2048, NULL, 5, &Battery_TaskHandle);
    }
};


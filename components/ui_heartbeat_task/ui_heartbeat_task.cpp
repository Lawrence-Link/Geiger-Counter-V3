#include "ui_heartbeat_task.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "PixelUI.h"

// Reference to global UI instance
extern PixelUI ui;

static void ui_heartbeat_task(void* p) {
    const TickType_t xFrequency = pdMS_TO_TICKS(10);
    TickType_t xLastWakeTime = xTaskGetTickCount();

    while (1) {
        ui.Heartbeat(10);
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

void start_ui_heartbeat_task() {
    xTaskCreate(ui_heartbeat_task, "UI_Heartbeat", 8192, NULL, 3, NULL);
}

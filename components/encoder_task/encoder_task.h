#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "encoder.h"
#include "PixelUI.h"

// Expose input event queue for UI to consume
extern QueueHandle_t input_event_queue;

#ifdef __cplusplus
extern "C" {
#endif

void start_encoder_task();

#ifdef __cplusplus
}
#endif

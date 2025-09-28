#pragma once

#include "driver/ledc.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

class VoltagePID {
public:
    VoltagePID(adc_unit_t unit,
               adc_channel_t channel,
               adc_atten_t atten,
               ledc_channel_t pwm_channel,
               ledc_timer_t pwm_timer,
               gpio_num_t pwm_gpio);

    void setTarget(float target_vin);        // set target in Volts
    void setPID(float kp, float ki, float kd);
    void startTask();                        // start FreeRTOS task
    void stop();                             // stop PWM, force low output
    float getVoltage() const;                // return latest Vin in Volts
    float getTargetVolt() const { return _target; }
private:
    static void controlTask(void* arg);
    void updateControl();

    // PID parameters
    float _kp, _ki, _kd;
    float _target;
    float _integral;
    float _prevError;

    // handles
    adc_unit_t _adcUnit;
    adc_channel_t _adcChannel;
    adc_atten_t _adcAtten;
    adc_oneshot_unit_handle_t _adcHandle;
    adc_cali_handle_t _caliHandle;

    ledc_channel_t _pwmChannel;
    ledc_timer_t _pwmTimer;
    gpio_num_t _pwmGpio;

    TaskHandle_t _taskHandle;

    // measured value
    float _vin; // in Volts
};

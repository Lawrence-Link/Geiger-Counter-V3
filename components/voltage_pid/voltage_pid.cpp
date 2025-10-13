#include "voltage_pid.hpp"
#include "esp_log.h"

static const char* TAG = "VoltagePID";

// resistor divider: 10M (high) / 68k (low)
#define R_HIGH 10000000.0f
#define R_LOW   68000.0f
#define DIV_RATIO ((R_HIGH + R_LOW) / R_LOW)

// PWM duty resolution
#define PWM_RESOLUTION LEDC_TIMER_10_BIT
#define PWM_MAX_DUTY ((1 << 10) - 1)  // 1023
#define PWM_DUTY_LIMIT (int)(PWM_MAX_DUTY * 0.8f) // 80% PWM limit

#define CORRECTION_OFFSET 40

VoltagePID::VoltagePID(adc_unit_t unit,
                       adc_channel_t channel,
                       adc_atten_t atten,
                       ledc_channel_t pwm_channel,
                       ledc_timer_t pwm_timer,
                       gpio_num_t pwm_gpio)
    : _kp(0.1f), _ki(0.01f), _kd(0.01f),
      _target(0.0f), _integral(0), _prevError(0),
      _adcUnit(unit), _adcChannel(channel), _adcAtten(atten),
      _pwmChannel(pwm_channel), _pwmTimer(pwm_timer), _pwmGpio(pwm_gpio),
      _taskHandle(nullptr), _vin(0.0f)
{
    // ADC init
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = _adcUnit,
        .clk_src = ADC_DIGI_CLK_SRC_DEFAULT,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &_adcHandle));

    adc_oneshot_chan_cfg_t config = {
        .atten = _adcAtten,
        .bitwidth = ADC_BITWIDTH_12,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(_adcHandle, _adcChannel, &config));

    // Calibration
    adc_cali_curve_fitting_config_t cali_config = {
        .unit_id = _adcUnit,
        .chan = _adcChannel,
        .atten = _adcAtten,
        .bitwidth = ADC_BITWIDTH_12,
    };
    if (adc_cali_create_scheme_curve_fitting(&cali_config, &_caliHandle) != ESP_OK) {
        ESP_LOGW(TAG, "ADC calibration not available, fallback to raw.");
        _caliHandle = nullptr;
    }

    // PWM init
    ledc_timer_config_t timer_conf = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = PWM_RESOLUTION,
        .timer_num = _pwmTimer,
        .freq_hz = 4000,
        .clk_cfg = LEDC_AUTO_CLK,
        .deconfigure = false
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer_conf));

    ledc_channel_config_t ch_conf = {
        .gpio_num = _pwmGpio,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = _pwmChannel,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = _pwmTimer,
        .duty = 0,
        .hpoint = 0,
        .sleep_mode = LEDC_SLEEP_MODE_KEEP_ALIVE,
        .flags = {.output_invert = 0}
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ch_conf));
}

void VoltagePID::setTarget(float target_vin) {
    _target = target_vin;
}

void VoltagePID::setPID(float kp, float ki, float kd) {
    _kp = kp;
    _ki = ki;
    _kd = kd;
}

void VoltagePID::startTask() {
    if (_taskHandle == nullptr) {
        xTaskCreate(controlTask, "voltage_pid_task", 4096, this, 5, &_taskHandle);
    }
}

void VoltagePID::stop() {
    if (_taskHandle) {
        vTaskDelete(_taskHandle);
        _taskHandle = nullptr;
    }
    // ensure PWM is low after control loop is dead
    ledc_set_duty(LEDC_LOW_SPEED_MODE, _pwmChannel, 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, _pwmChannel);
}

float VoltagePID::getVoltage() const {
    return _vin;
}

void VoltagePID::controlTask(void* arg) {
    auto* self = static_cast<VoltagePID*>(arg);
    while (true) {
        self->updateControl();
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

void VoltagePID::updateControl() {
    int raw = 0;
    esp_err_t ret = adc_oneshot_read(_adcHandle, _adcChannel, &raw);
    if (ret == ESP_OK) {
        int mv = 0;
        if (_caliHandle) {
            adc_cali_raw_to_voltage(_caliHandle, raw, &mv);
        } else {
            mv = (raw * 3100) / 4095; // fallback
        }
        float vout = mv / 1000.0f;
        _vin = vout * DIV_RATIO - CORRECTION_OFFSET;

        // PID calculation
        float error = _target - _vin;
        _integral += error * 0.02f;
        float derivative = (error - _prevError) / 0.02f;
        float output = _kp * error + _ki * _integral + _kd * derivative;
        _prevError = error;

        int duty = (int)output;
        if (duty < 0) duty = 0;
        if (duty > PWM_DUTY_LIMIT) duty = PWM_DUTY_LIMIT;

        ledc_set_duty(LEDC_LOW_SPEED_MODE, _pwmChannel, duty);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, _pwmChannel);

        // ESP_LOGI(TAG, "Raw:%d Vout=%.3fV Vin=%.1fV Duty=%d", raw, vout, _vin, duty);
    }
}

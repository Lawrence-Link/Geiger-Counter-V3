// bme280_port.cpp
#include "bme280_port.h"
#include "esp_log.h"
#include "esp_err.h"

static const char* TAG = "BME280";

// C风格全局变量
i2c_master_dev_handle_t bme280_dev = NULL;

// C++静态成员变量初始化
i2c_master_dev_handle_t BME280_Port::dev_handle = NULL;
TaskHandle_t BME280_Port::task_handle = NULL;
bool BME280_Port::is_running = false;
struct bme280_dev BME280_Port::dev = {0};
struct bme280_settings BME280_Port::settings = {0};
float BME280_Port::latest_temperature = 0.0f;
float BME280_Port::latest_humidity = 0.0f;
float BME280_Port::latest_pressure = 0.0f;

// C风格实现
esp_err_t bme280_sensor_init(i2c_master_bus_handle_t bus_handle) {
    return BME280_Port::init(bus_handle);
}

float bme280_read_temperature_celsius(void) {
    return BME280_Port::read_temperature_celsius();
}

float bme280_read_humidity_percentage(void) {
    return BME280_Port::read_humidity_percentage();
}

float bme280_read_barometer(void) {
    return BME280_Port::read_barometer();
}

esp_err_t bme280_start_reading(void) {
    return BME280_Port::start_reading();
}

esp_err_t bme280_stop_reading(void) {
    return BME280_Port::stop_reading();
}

bool bme280_is_reading(void) {
    return BME280_Port::is_reading_active();
}

esp_err_t bme280_deinit(void) {
    return BME280_Port::deinit();
}

esp_err_t BME280_Port::init(i2c_master_bus_handle_t bus_handle) {
    ESP_LOGI(TAG, "Initializing BME280 sensor...");

    if (dev_handle != NULL) {
        ESP_LOGW(TAG, "BME280 already initialized");
        return ESP_OK;
    }

    i2c_device_config_t dev_cfg_bme280 = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7, 
        .device_address = BME280_I2C_ADDR, 
        .scl_speed_hz = 100000,
        .scl_wait_us = 0,
        .flags = 0,
    };
    
    esp_err_t ret = i2c_master_bus_add_device(bus_handle, &dev_cfg_bme280, &dev_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add BME280 device: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // 同步更新C风格全局变量
    bme280_dev = dev_handle;
    
    // 初始化传感器
    if (!init_sensor()) {
        ESP_LOGE(TAG, "Failed to initialize BME280 sensor");
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "BME280 initialized successfully");
    return ESP_OK;
}

bool BME280_Port::init_sensor() {
    // 配置BME280设备接口
    dev.intf_ptr = nullptr;
    dev.intf = BME280_I2C_INTF;
    dev.read = &BME280_Port::i2c_read;
    dev.write = &BME280_Port::i2c_write;
    dev.delay_us = &BME280_Port::delay_us;
    
    // 初始化传感器
    int8_t rslt = bme280_init(&dev);
    print_error_result("bme280_init", rslt);
    
    if (rslt != BME280_OK) {
        return false;
    }
    
    // 获取传感器设置
    rslt = bme280_get_sensor_settings(&settings, &dev);
    print_error_result("bme280_get_sensor_settings", rslt);
    
    // 配置稳定的初始化参数（以稳定为主）
    settings.filter = BME280_FILTER_COEFF_16;        // 最强滤波
    settings.osr_h = BME280_OVERSAMPLING_16X;        // 16倍湿度过采样
    settings.osr_p = BME280_OVERSAMPLING_16X;        // 16倍压力过采样  
    settings.osr_t = BME280_OVERSAMPLING_16X;        // 16倍温度过采样
    settings.standby_time = BME280_STANDBY_TIME_1000_MS;
    
    // 应用设置
    uint8_t change_settings = BME280_SEL_ALL_SETTINGS;
    rslt = bme280_set_sensor_settings(change_settings, &settings, &dev);
    print_error_result("bme280_set_sensor_settings", rslt);
    
    if (rslt != BME280_OK) {
        return false;
    }
    
    ESP_LOGI(TAG, "BME280 sensor configured with stable settings");
    return true;
}

esp_err_t BME280_Port::deinit() {
    ESP_LOGI(TAG, "Deinitializing BME280...");
    
    // 停止读取任务
    stop_reading();
    
    // 移除I2C设备
    if (dev_handle != NULL) {
        esp_err_t ret = i2c_master_bus_rm_device(dev_handle);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to remove BME280 device: %s", esp_err_to_name(ret));
            return ret;
        }
        
        dev_handle = NULL;
        bme280_dev = NULL;
    }
    
    ESP_LOGI(TAG, "BME280 deinitialized successfully");
    return ESP_OK;
}

esp_err_t BME280_Port::start_reading() {
    if (is_running) {
        ESP_LOGW(TAG, "BME280 reading task already running");
        return ESP_OK;
    }
    
    is_running = true;
    
    BaseType_t ret = xTaskCreate(read_task, "bme280_read", 4096, NULL, 1, &task_handle);
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create BME280 reading task");
        is_running = false;
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "BME280 reading task started (500ms interval)");
    return ESP_OK;
}

esp_err_t BME280_Port::stop_reading() {
    if (!is_running) {
        return ESP_OK;
    }
    
    is_running = false;
    
    if (task_handle != NULL) {
        vTaskDelete(task_handle);
        task_handle = NULL;
    }
    
    ESP_LOGI(TAG, "BME280 reading task stopped");
    return ESP_OK;
}

void BME280_Port::read_task(void* parameter) {
    ESP_LOGI(TAG, "BME280 reading task started");
    
    while (is_running) {
        if (read_sensor_data()) {
            ESP_LOGD(TAG, "BME280: T=%.2f°C H=%.2f%% P=%.2f Pa", 
                     latest_temperature, latest_humidity, latest_pressure);
        }
        
        // 固定500ms读取间隔
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    
    ESP_LOGI(TAG, "BME280 reading task stopped");
    vTaskDelete(NULL);
}

bool BME280_Port::read_sensor_data() {
    if (dev_handle == NULL) {
        return false;
    }
    
    struct bme280_data comp_data;
    uint32_t period;
    
    // 计算测量延迟
    int8_t rslt = bme280_cal_meas_delay(&period, &settings);
    if (rslt != BME280_OK) {
        print_error_result("bme280_cal_meas_delay", rslt);
        return false;
    }
    
    // 设置为强制模式
    rslt = bme280_set_sensor_mode(BME280_POWERMODE_FORCED, &dev);
    if (rslt != BME280_OK) {
        print_error_result("bme280_set_sensor_mode", rslt);
        return false;
    }
    
    // 等待测量完成
    dev.delay_us(period + 1000, nullptr);
    
    // 读取传感器数据
    rslt = bme280_get_sensor_data(BME280_ALL, &comp_data, &dev);
    if (rslt != BME280_OK) {
        print_error_result("bme280_get_sensor_data", rslt);
        return false;
    }
    
    // 更新最新数据
    latest_temperature = comp_data.temperature;
    latest_humidity = comp_data.humidity;
    latest_pressure = comp_data.pressure;
    
    return true;
}

float BME280_Port::read_temperature_celsius() {
    return latest_temperature;
}

float BME280_Port::read_humidity_percentage() {
    return latest_humidity;
}

float BME280_Port::read_barometer() {
    return latest_pressure;
}

// BME280接口函数实现
BME280_INTF_RET_TYPE BME280_Port::i2c_read(uint8_t reg_addr, uint8_t *data, uint32_t len) {
    if (dev_handle == NULL) {
        return BME280_E_COMM_FAIL;
    }
    
    esp_err_t ret = i2c_master_transmit_receive(dev_handle, &reg_addr, 1, data, len, pdMS_TO_TICKS(1000));
    return (ret == ESP_OK) ? BME280_OK : BME280_E_COMM_FAIL;
}

BME280_INTF_RET_TYPE BME280_Port::i2c_write(uint8_t reg_addr, uint8_t *data) {
    if (dev_handle == NULL) {
        return BME280_E_COMM_FAIL;
    }
    
    uint8_t write_buf[2] = {reg_addr, *data};
    esp_err_t ret = i2c_master_transmit(dev_handle, write_buf, sizeof(write_buf), pdMS_TO_TICKS(1000));
    return (ret == ESP_OK) ? BME280_OK : BME280_E_COMM_FAIL;
}

void BME280_Port::delay_us(uint32_t period, void *intf_ptr) {
    esp_rom_delay_us(period);
}

void BME280_Port::print_error_result(const char api_name[], int8_t rslt) {
    if (rslt != BME280_OK) {
        ESP_LOGE(TAG, "%s failed with error code: %d", api_name, rslt);
        
        switch (rslt) {
            case BME280_E_NULL_PTR:
                ESP_LOGE(TAG, "Null pointer error");
                break;
            case BME280_E_COMM_FAIL:
                ESP_LOGE(TAG, "Communication failure");
                break;
            case BME280_E_DEV_NOT_FOUND:
                ESP_LOGE(TAG, "Device not found");
                break;
            case BME280_E_INVALID_LEN:
                ESP_LOGE(TAG, "Invalid length");
                break;
            default:
                ESP_LOGE(TAG, "Unknown error code");
                break;
        }
    }
}
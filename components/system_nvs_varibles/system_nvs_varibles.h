#pragma once
#include <cstdint>
#include <system_error>

class SystemConf {

public:
    // --- 单例访问 ---
    static SystemConf& getInstance () {
        static SystemConf instance;
        return instance;
    }

    SystemConf(const SystemConf&) = delete;
    SystemConf& operator=(const SystemConf) = delete;

    SystemConf() = default;

    // --- NVS 接口 ---
    void load_conf_from_nvs();
    void save_conf_to_nvs();

    // --- 读取接口 (Getter) ---
    bool read_conf_enable_alert() const { return alert_status; }
    bool read_conf_enable_sound() const { return sound_status; }
    bool read_conf_enable_blink() const { return blink_status; }

    uint8_t read_conf_brightness() const { return brightness; }
    uint8_t read_conf_loudness() const { return loudness; }
    float read_conf_tube_convertion_coefficient() const { return tube_convertion_coefficient; }


    // --- 写入接口 (Setter) ---
    void set_conf_enable_alert(bool value) { alert_status = value; }
    void set_conf_enable_sound(bool value) { sound_status = value; }
    void set_conf_enable_blink(bool value) { blink_status = value; }

    void set_conf_brightness(uint8_t value) { brightness = value; }
    void set_conf_loudness(uint8_t value) { loudness = value; }
    void set_conf_tube_convertion_coefficient(float value) { tube_convertion_coefficient = value; }

private:
    // --- 默认配置值 ---
    bool alert_status = false;
    bool sound_status = false;
    bool blink_status = false;    

    uint8_t brightness = 5; // 默认值 5
    uint8_t loudness = 50;    // 默认值 50
    float tube_convertion_coefficient = 0.00662f; // 默认值
};
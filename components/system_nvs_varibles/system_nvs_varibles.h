#pragma once 
#include <cstdint>

class SystemConf {

public:
    static SystemConf& getInstance () {
        static SystemConf instance;
        return instance;
    }

    SystemConf(const SystemConf&) = delete;
    SystemConf& operator=(const SystemConf) = delete;

    SystemConf() = default;

    void load_conf_from_nvs();

    bool read_conf_enable_alert() const { return alert_status; }
    bool read_conf_enable_sound() const { return sound_status; }

    uint8_t read_conf_brightness() const { return brightness; }
    uint8_t read_conf_loudness() const { return loudness; }
    uint8_t read_conf_enable_blink() const { return alert_status; }
    float read_conf_tube_convertion_coefficient() const { return tube_convertion_coefficient; }

private:
    bool alert_status;
    bool sound_status;
    bool blink_status;    

    uint8_t brightness;
    uint8_t loudness;
    float tube_convertion_coefficient = 0.00662;
};
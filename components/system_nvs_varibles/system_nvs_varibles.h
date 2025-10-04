#pragma once
#include <cstdint>
#include <system_error>

/**
 * @brief Load value from nvs. setting default value when it had never been initialized.
 */
#define LOAD_OR_SET(key_name, var) \
        storage.get(key_name, var, ec); \
        if (ec) { \
            if (ec.value() == NVS_KEY_NOT_FOUND) { \
                /* Key not found, set default value */ \
                fmt::print("Key '{}' not found, setting default: {}\n", key_name, var); \
                storage.set(key_name, var, ec); \
            } else { \
                fmt::print("Error loading {}: {}\n", key_name, ec.message()); \
            } \
        } \
        ec.clear();

class SystemConf {
public:
    static SystemConf& getInstance () {
        static SystemConf instance;
        return instance;
    }

    SystemConf(const SystemConf&) = delete;
    SystemConf& operator=(const SystemConf) = delete;

    SystemConf() = default;

    // --- overall ---
    void load_conf_from_nvs();
    void save_conf_to_nvs();

    // --- (Getter APIs) ---
    bool read_conf_enable_alert() const { return alert_status; }
    bool read_conf_enable_geiger_click() const { return geiger_click_status; }
    bool read_conf_enable_blink() const { return blink_status; }
    bool read_conf_enable_navi_tone() const { return navi_tone_status; }
    bool read_conf_enable_interaction_tone() const { return en_interaction_tone; }
    uint8_t read_conf_brightness() const { return brightness; }
    int32_t read_conf_warn_threshold() const { return cpm_warn_threshold; }
    int32_t read_conf_dngr_threshold() const { return cpm_dngr_threshold; }
    int32_t read_conf_hzdr_threshold() const { return cpm_hzdr_threshold; }
    int32_t read_conf_operation_voltage() const { return operation_voltage; }
    float read_conf_tube_convertion_coefficient() const { return tube_convertion_coefficient; }

    // --- (Setter APIs) ---
    void set_conf_enable_alert(bool value) { alert_status = value; }
    void set_conf_enable_geiger_click(bool value) { geiger_click_status = value; }
    void set_conf_enable_blink(bool value) { blink_status = value; }
    void set_conf_enable_navi_tone(bool value) { navi_tone_status = value; }
    void set_conf_enable_interaction_tone(bool value) { en_interaction_tone = value; }
    void set_conf_brightness(uint8_t value) { brightness = value; }
    void set_conf_warn_threshold(uint16_t value) { cpm_warn_threshold = value; }
    void set_conf_dngr_threshold(uint16_t value) { cpm_dngr_threshold = value; }
    void set_conf_hzdr_threshold(uint16_t value) { cpm_hzdr_threshold = value; }
    void set_conf_tube_convertion_coefficient(float value) { tube_convertion_coefficient = value; }
    void set_conf_operation_voltage(int32_t value) { operation_voltage = value; }
private:
    // --- default ---
    bool alert_status = true;
    bool geiger_click_status = true;
    bool blink_status = true;   
    bool navi_tone_status = false; 
    bool en_interaction_tone = true;
    
    uint8_t brightness = 5;
    float tube_convertion_coefficient = 0.00662f;

    int32_t cpm_warn_threshold = 300;
    int32_t cpm_dngr_threshold = 600;
    int32_t cpm_hzdr_threshold = 1000;

    int32_t operation_voltage = 380;
};
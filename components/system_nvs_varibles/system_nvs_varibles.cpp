#include "system_nvs_varibles.h"

// 引入 NVS 库
#include "nvs.hpp"
#include "nvs_handle_espp.hpp"
#include "format.hpp"
#include "esp_log.h"

// NVS 命名空间宏定义 (长度 <= 15)
#define CONF_NAMESPACE "sys_config"

// =========================================================================
// FIX: Define NVS_KEY_NOT_FOUND here, outside of any function, 
// so it is visible to the macros used inside SystemConf::load_conf_from_nvs()
// =========================================================================
const int NVS_KEY_NOT_FOUND = static_cast<int>(NvsErrc::Key_Not_Found);

/**
 * @brief load configuration from nvs. if anything is new, write it in with default value
 */
void SystemConf::load_conf_from_nvs() {
    std::error_code ec;
    
    espp::NvsHandle storage(CONF_NAMESPACE, ec);
    if (ec) {
        fmt::print("FATAL: Error opening NVS handle for {}: {}\n", CONF_NAMESPACE, ec.message());
        return;
    }
    
    fmt::print("Loading system configuration from NVS...\n");

    LOAD_OR_SET("alert", alert_status);
    LOAD_OR_SET("geiger_click", geiger_click_status);
    LOAD_OR_SET("blink", blink_status);
    LOAD_OR_SET("navi_tone", navi_tone_status);
    LOAD_OR_SET("intr_tone", en_interaction_tone);
    LOAD_OR_SET("use_cpm", use_cpm);
    LOAD_OR_SET("bright", brightness);
    LOAD_OR_SET("tube_coeff", tube_convertion_coefficient);
    LOAD_OR_SET("cpm_warn", cpm_warn_threshold);
    LOAD_OR_SET("cpm_dngr", cpm_dngr_threshold);
    LOAD_OR_SET("cpm_hzdr", cpm_hzdr_threshold);
    LOAD_OR_SET("oprt_volt", operation_voltage);
    
    fmt::print("Configuration loaded.");
}

/**
 * @brief 将当前配置保存到 NVS。
 */
void SystemConf::save_conf_to_nvs() {
    std::error_code ec;
    
    espp::NvsHandle storage(CONF_NAMESPACE, ec);
    if (ec) {
        fmt::print("FATAL: Error opening NVS handle for saving: {}\n", ec.message());
        return;
    }
    
    // store all varibles
    storage.set("alert", alert_status, ec);
    storage.set("geiger_click", geiger_click_status, ec);
    storage.set("blink", blink_status, ec);
    storage.set("navi_tone", navi_tone_status, ec);
    storage.set("intr_tone", en_interaction_tone, ec);
    storage.set("use_cpm", use_cpm, ec);
    storage.set("bright", brightness, ec);
    storage.set("tube_coeff", tube_convertion_coefficient, ec);
    storage.set("cpm_warn", cpm_warn_threshold, ec);
    storage.set("cpm_dngr", cpm_dngr_threshold, ec);
    storage.set("cpm_hzdr", cpm_hzdr_threshold, ec);
    storage.set("oprt_volt", operation_voltage, ec);

    if (ec) {
        fmt::print("Error setting one or more values: {}\n", ec.message());
    } else {
        storage.commit(ec); 
        if (ec) {
            fmt::print("Error committing NVS changes: {}\n", ec.message());
        } else {
            fmt::print("Configuration saved successfully.\n");
        }
    }
}
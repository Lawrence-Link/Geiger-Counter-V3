#include "system_nvs_varibles.h"

// 引入 NVS 库
#include "nvs.hpp"
#include "nvs_handle_espp.hpp"
#include "format.hpp"

// NVS 命名空间宏定义 (长度 <= 15)
#define CONF_NAMESPACE "sys_config"

// =========================================================================
// FIX: Define NVS_KEY_NOT_FOUND here, outside of any function, 
// so it is visible to the macros used inside SystemConf::load_conf_from_nvs()
// =========================================================================
const int NVS_KEY_NOT_FOUND = static_cast<int>(NvsErrc::Key_Not_Found);


/**
 * @brief 从 NVS 加载配置。如果键不存在，则使用成员变量的当前值（即默认值）并写入 NVS。
 */
void SystemConf::load_conf_from_nvs() {
    std::error_code ec;
    
    // 1. 打开 NVS 命名空间的句柄
    espp::NvsHandle storage(CONF_NAMESPACE, ec);
    if (ec) {
        fmt::print("FATAL: Error opening NVS handle for {}: {}\n", CONF_NAMESPACE, ec.message());
        // 无法打开，使用默认值
        return;
    }
    
    fmt::print("Loading system configuration from NVS...\n");

    // --- 布尔类型 (alert_status, sound_status, blink_status) ---
    // The macros below now correctly reference NVS_KEY_NOT_FOUND
    #define LOAD_OR_SET_BOOL(key_name, var) \
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
        ec.clear(); /* Clear EC for the next operation */

    LOAD_OR_SET_BOOL("alert", alert_status);
    LOAD_OR_SET_BOOL("sound", sound_status);
    LOAD_OR_SET_BOOL("blink", blink_status);

    // --- uint8_t 类型 (brightness, loudness) ---
    #define LOAD_OR_SET_UINT8(key_name, var) \
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

    LOAD_OR_SET_UINT8("bright", brightness);
    LOAD_OR_SET_UINT8("loud", loudness);

    // --- float 类型 (tube_convertion_coefficient) ---
    #define LOAD_OR_SET_FLOAT(key_name, var) \
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

    LOAD_OR_SET_FLOAT("tube_coeff", tube_convertion_coefficient);
    
    fmt::print("Configuration loaded. Brightness: {}, Alert: {}\n", brightness, alert_status);
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
    
    // 写入所有变量
    storage.set("alert", alert_status, ec);
    storage.set("sound", sound_status, ec);
    storage.set("blink", blink_status, ec);
    storage.set("bright", brightness, ec);
    storage.set("loud", loudness, ec);
    storage.set("tube_coeff", tube_convertion_coefficient, ec);

    if (ec) {
        fmt::print("Error setting one or more values: {}\n", ec.message());
    } else {
        // 提交更改到闪存
        storage.commit(ec); 
        if (ec) {
            fmt::print("Error committing NVS changes: {}\n", ec.message());
        } else {
            fmt::print("Configuration saved successfully.\n");
        }
    }
}
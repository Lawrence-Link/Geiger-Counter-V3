#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "cw2015.h"

#define VCELL_LSB_UV 305u

static const char *TAG = "cw2015";
static i2c_master_dev_handle_t s_dev_handle = NULL;

// static esp_err_t i2c_write_reg(uint8_t reg, const uint8_t *data, size_t len)
// {
//     uint8_t buf[1 + len];
//     buf[0] = reg;
//     memcpy(&buf[1], data, len);

//     return i2c_master_transmit(s_dev_handle, buf, sizeof(buf), pdMS_TO_TICKS(1000));
// }

static esp_err_t i2c_read_reg(uint8_t reg, uint8_t *out, size_t len)
{
    return i2c_master_transmit_receive(s_dev_handle, &reg, 1, out, len, pdMS_TO_TICKS(1000));
}
static esp_err_t i2c_read_reg_simple(i2c_master_dev_handle_t dev, uint8_t reg, uint8_t *buf, size_t len)
{
    // 使用 i2c_master_transmit_receive，写寄存器再读数据
    return i2c_master_transmit_receive(dev, &reg, 1, buf, len, pdMS_TO_TICKS(200));
}
esp_err_t cw2015_init(i2c_master_dev_handle_t dev_handle)
{
    if (!dev_handle) {
        return ESP_ERR_INVALID_ARG;
    }
    s_dev_handle = dev_handle;
    ESP_LOGI(TAG, "CW2015 handle attached");
    return ESP_OK;
}

esp_err_t cw2015_read_vcell_mv(uint16_t *vcell_mv)
{
    if (!vcell_mv) return ESP_ERR_INVALID_ARG;

    uint8_t buf[2];
    esp_err_t r = i2c_read_reg(CW2015_REG_VCELL, buf, 2); // 0x02,0x03
    if (r != ESP_OK) return r;

    uint16_t raw16 = ((uint16_t)buf[0] << 8) | buf[1];

    // 按 datasheet 使用 14-bit 有效位
    uint32_t r14 = raw16 & 0x3FFFu;

    // 使用 32-bit 计算微伏，防止溢出
    uint32_t uv = r14 * (uint32_t)VCELL_LSB_UV; // microvolts

    // 四舍五入转换为毫伏（可选）
    uint32_t mv = (uv + 500u) / 1000u;

    *vcell_mv = (uint16_t)mv; // fits typical battery voltages

    return ESP_OK;
}

esp_err_t cw2015_read_soc(int *soc) {
    uint8_t buf[2];
    esp_err_t ret = i2c_read_reg(CW2015_REG_SOC, buf, 2);
    if (ret != ESP_OK) return ret;

    *soc = buf[0]; 
    return ESP_OK;
}

void dump_cw2015_regs(i2c_master_dev_handle_t dev)
{
    if (!dev) {
        ESP_LOGE(TAG, "dev handle is NULL");
        return;
    }

    uint8_t v;
    if (i2c_read_reg_simple(dev, 0x00, &v, 1) == ESP_OK) {
        ESP_LOGI(TAG, "REG 0x00 VERSION = 0x%02X", v);
    } else {
        ESP_LOGW(TAG, "Failed to read REG 0x00");
    }

    uint8_t buf[2];
    if (i2c_read_reg_simple(dev, 0x02, buf, 2) == ESP_OK) {
        ESP_LOGI(TAG, "REG 0x02..0x03 VCELL raw = 0x%02X 0x%02X", buf[0], buf[1]);
    } else {
        ESP_LOGW(TAG, "Failed to read VCELL regs");
    }

    if (i2c_read_reg_simple(dev, 0x04, buf, 2) == ESP_OK) {
        ESP_LOGI(TAG, "REG 0x04..0x05 SOC raw = 0x%02X 0x%02X", buf[0], buf[1]);
    } else {
        ESP_LOGW(TAG, "Failed to read SOC regs");
    }

    if (i2c_read_reg_simple(dev, 0x0A, buf, 2) == ESP_OK) {
        ESP_LOGI(TAG, "REG 0x0A MODE raw = 0x%02X 0x%02X", buf[0], buf[1]);
    } else {
        ESP_LOGW(TAG, "Failed to read MODE regs");
    }

    // 如果 VCELL raw 非全 0，则分别用两种常见解析方法解释
    // uint16_t raw = ((uint16_t)buf[0] << 8) | buf[1]; 
    // 为了可靠，重新读 VCELL 并解释：
    uint8_t vcell_bytes[2];
    if (i2c_read_reg_simple(dev, 0x02, vcell_bytes, 2) == ESP_OK) {
        uint16_t vcell_raw = ((uint16_t)vcell_bytes[0] << 8) | vcell_bytes[1];
        ESP_LOGI(TAG, "VCELL bytes = 0x%02X 0x%02X -> raw16 = 0x%04X (dec %u)",
                 vcell_bytes[0], vcell_bytes[1], vcell_raw, (unsigned)vcell_raw);

        // 解析方法 A：mask 14-bit (raw & 0x3FFF), LSB = 305 uV
        uint16_t r14 = vcell_raw & 0x3FFF;
        uint32_t uv = (uint32_t)r14 * 305u; // microvolts
        ESP_LOGI(TAG, "Interpret A: 14-bit masked = %u -> %u uV -> %u mV", r14, uv, uv/1000);

        // 解析方法 B：右移4位 (some datasheets show left-aligned 12-bit)
        uint16_t r_sh = vcell_raw >> 4;
        uint32_t uv_b = (uint32_t)r_sh * 305u;
        ESP_LOGI(TAG, "Interpret B: (raw >> 4) = %u -> %u uV -> %u mV", r_sh, uv_b, uv_b/1000);
    } else {
        ESP_LOGW(TAG, "Cannot re-read VCELL for interpretation");
    }
}
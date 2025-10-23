/**
 * @file u8g2_port.c
 * @brief U8G2 ESP32-C6 SPI移植层实现
 * @author Assistant
 * @date 2024
 */

#include "u8g2_port.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "pin_definitions.h"

static const char *TAG = "U8G2_PORT";

// SPI设备句柄
static spi_device_handle_t spi_device = NULL;
static bool is_initialized = false;

/**
 * @brief U8G2移植层初始化
 */
esp_err_t u8g2_port_init(void)
{
    if (is_initialized) {
        ESP_LOGW(TAG, "U8G2 port already initialized");
        return ESP_OK;
    }

    esp_err_t ret = ESP_OK;

    // 配置DC引脚为输出
    gpio_config_t dc_conf = {
        .pin_bit_mask = (1ULL << U8G2_PIN_DC),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    ret = gpio_config(&dc_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure DC pin: %s", esp_err_to_name(ret));
        return ret;
    }

    // 初始化SPI总线
    spi_bus_config_t bus_config = {
        .mosi_io_num = U8G2_PIN_MOSI,
        .miso_io_num = -1,  // 不使用MISO
        .sclk_io_num = U8G2_PIN_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4096,
        .flags = SPICOMMON_BUSFLAG_MASTER
    };

    ret = spi_bus_initialize(U8G2_SPI_HOST, &bus_config, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SPI bus: %s", esp_err_to_name(ret));
        return ret;
    }

    // 配置SPI设备
    spi_device_interface_config_t dev_config = {
        .clock_speed_hz = U8G2_SPI_CLOCK_SPEED_HZ,
        .mode = 0,  // SPI模式0 (CPOL=0, CPHA=0)
        .spics_io_num = -1,  // CS引脚接地，不使用
        .queue_size = U8G2_SPI_QUEUE_SIZE,
        .flags = SPI_DEVICE_HALFDUPLEX,
        .pre_cb = NULL,
        .post_cb = NULL
    };

    ret = spi_bus_add_device(U8G2_SPI_HOST, &dev_config, &spi_device);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add SPI device: %s", esp_err_to_name(ret));
        spi_bus_free(U8G2_SPI_HOST);
        return ret;
    }

    is_initialized = true;
    ESP_LOGI(TAG, "U8G2 SPI port initialized successfully");
    return ESP_OK;
}

/**
 * @brief U8G2移植层去初始化
 */
esp_err_t u8g2_port_deinit(void)
{
    if (!is_initialized) {
        return ESP_OK;
    }

    esp_err_t ret = ESP_OK;

    if (spi_device != NULL) {
        ret = spi_bus_remove_device(spi_device);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to remove SPI device: %s", esp_err_to_name(ret));
        }
        spi_device = NULL;
    }

    ret = spi_bus_free(U8G2_SPI_HOST);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to free SPI bus: %s", esp_err_to_name(ret));
    }

    is_initialized = false;
    ESP_LOGI(TAG, "U8G2 SPI port deinitialized");
    return ret;
}

/**
 * @brief 发送SPI数据
 * @param data 要发送的数据指针
 * @param len 数据长度
 * @return ESP_OK 成功, 其他值 失败
 */
static esp_err_t spi_write_data(const uint8_t *data, size_t len)
{
    if (!is_initialized || spi_device == NULL) {
        ESP_LOGE(TAG, "SPI not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    if (len == 0) {
        return ESP_OK;
    }

    spi_transaction_t trans = {
        .length = len * 8,  // 以位为单位的长度
        .tx_buffer = data,
        .rx_buffer = NULL
    };

    esp_err_t ret = spi_device_transmit(spi_device, &trans);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI transmit failed: %s", esp_err_to_name(ret));
    }

    return ret;
}

static void set_dc_level(uint32_t level)
{
    gpio_set_level(U8G2_PIN_DC, level);
}

/**
 * @brief U8G2 hardware spi communication callback
 */
uint8_t u8g2_esp32_spi_byte_cb(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
    esp_err_t ret = ESP_OK;
    
    switch (msg) {
        case U8X8_MSG_BYTE_SEND:
            {
                uint8_t *data = (uint8_t *)arg_ptr;
                ret = spi_write_data(data, arg_int);
                if (ret != ESP_OK) {
                    ESP_LOGE(TAG, "Failed to send SPI data");
                    return 0;
                }
            }
            break;

        case U8X8_MSG_BYTE_INIT:
            // SPI initialization was already done in the esp32_port_init
            break;

        case U8X8_MSG_BYTE_SET_DC:
            // 0 = CMD, 1 = DATA
            set_dc_level(arg_int);
            break;

        case U8X8_MSG_BYTE_START_TRANSFER:
            // Beginning of the transmission, since CS is already grounded, no more operation is needed.
            break;

        case U8X8_MSG_BYTE_END_TRANSFER:
            // End of the transmission, since CS is already grounded, no more operation is needed.
            break;

        default:
            ESP_LOGW(TAG, "Unknown SPI message: %d", msg);
            return 0;
    }

    return 1;
}

/**
 * @brief U8G2 GPIO和延时回调函数
 */
uint8_t u8g2_esp32_gpio_and_delay_cb(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
    switch (msg) {
        case U8X8_MSG_GPIO_AND_DELAY_INIT:
            // GPIO初始化在u8g2_port_init()中完成
            ESP_LOGI(TAG, "GPIO and delay init");
            break;

        case U8X8_MSG_DELAY_NANO:
            // 纳秒延时，对于ESP32可以忽略
            break;

        case U8X8_MSG_DELAY_100NANO:
            // 100纳秒延时，对于ESP32可以忽略
            break;

        case U8X8_MSG_DELAY_10MICRO:
            // 10微秒延时
            esp_rom_delay_us(10);
            break;

        case U8X8_MSG_DELAY_MILLI:
            // 毫秒延时
            vTaskDelay(pdMS_TO_TICKS(arg_int));
            break;

        case U8X8_MSG_GPIO_DC:
            // DC引脚控制在SPI回调中处理
            break;

        case U8X8_MSG_GPIO_CS:
            // CS引脚接地，无需控制
            break;

        case U8X8_MSG_GPIO_RESET:
            if (arg_int == 0) {
                gpio_set_level(PIN_OLED_RST, 0);  // 拉低RESET引脚开始复位
            } else {
                gpio_set_level(PIN_OLED_RST, 1);  // 拉高RESET引脚解除复位
            }
            break;

        default:
            ESP_LOGW(TAG, "Unknown GPIO message: %d", msg);
            return 0;
    }

    return 1;
}

/**
 * @brief 初始化U8G2和SH1106显示器
 * @param u8g2 U8G2结构体指针
 * @return ESP_OK 成功, 其他值 失败
 */
esp_err_t u8g2_init_sh1106(u8g2_t *u8g2)
{
    if (u8g2 == NULL) {
        ESP_LOGE(TAG, "u8g2 pointer is NULL");
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Initializing U8G2 with SH1106 display");

    esp_err_t ret = u8g2_port_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize U8G2 port: %s", esp_err_to_name(ret));
        return ret;
    }

    #ifdef USE_SH1106
    u8g2_Setup_sh1106_128x64_noname_f(u8g2, 
                                       U8G2_R0,
                                       u8g2_esp32_spi_byte_cb,
                                       u8g2_esp32_gpio_and_delay_cb);
    #endif 

    #ifdef USE_SSD1306
    u8g2_Setup_ssd1306_128x64_noname_f(u8g2, 
                                       U8G2_R0,
                                       u8g2_esp32_spi_byte_cb,
                                       u8g2_esp32_gpio_and_delay_cb);
    #endif

    u8g2_InitDisplay(u8g2);

    u8g2_SetPowerSave(u8g2, 0); // Power on the display

    u8g2_ClearBuffer(u8g2); // Clear screen buffer
    u8g2_SendBuffer(u8g2);

    ESP_LOGI(TAG, "U8G2 SH1106 initialization completed successfully");
    return ESP_OK;
}

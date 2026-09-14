#include "wifi_assists.h"

#include "esp_event.h"
#include "esp_netif.h"

#include <cstring>
#include <etl/atomic.h>

namespace {

constexpr size_t kWifiListFirstSlot = 2U;
constexpr size_t kWifiListSlotCount = 8U;
constexpr size_t kSsidCapacity = 33U;
constexpr size_t kPasswordCapacity = MAX_TEXT_LENGTH + 1U;
bool wifi_initialized = false;
bool wifi_infrastructure_initialized = false;
bool wifi_started = false;
bool connected_slots[kWifiListSlotCount]{};
char scanned_ssids[kWifiListSlotCount][kSsidCapacity]{};
char password[kPasswordCapacity]{};
size_t selected_slot = kWifiListSlotCount;
etl::atomic_bool connection_state_changed{false};
etl::atomic_bool reconnect_requested{false};
etl::atomic_bool suppress_next_reconnect{false};

void clear_connected_slots() {
    for (bool& connected : connected_slots) {
        connected = false;
    }
}

void update_connected_slot(const uint8_t* ssid, size_t ssid_length) {
    clear_connected_slots();
    if (ssid == nullptr) {
        return;
    }

    for (size_t i = 0; i < kWifiListSlotCount; ++i) {
        const size_t scanned_length = std::strlen(scanned_ssids[i]);
        if (scanned_length == ssid_length &&
            std::memcmp(scanned_ssids[i], ssid, ssid_length) == 0) {
            connected_slots[i] = true;
            break;
        }
    }
}

void wifi_event_handler(void*, esp_event_base_t, int32_t event_id, void* event_data) {
    if (event_id == WIFI_EVENT_STA_CONNECTED) {
        reconnect_requested.store(false, etl::memory_order_release);
        connection_state_changed.store(true, etl::memory_order_release);
    } else if (event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (!suppress_next_reconnect.exchange(false, etl::memory_order_acq_rel)) {
            reconnect_requested.store(true, etl::memory_order_release);
        }
        connection_state_changed.store(true, etl::memory_order_release);
    }
}

bool initialize_wifi_infrastructure() {
    if (wifi_infrastructure_initialized) {
        return true;
    }

    const esp_err_t netif_error = esp_netif_init();
    if (netif_error != ESP_OK && netif_error != ESP_ERR_INVALID_STATE) {
        ESP_LOGE("WiFi", "Failed to initialize network interface");
        return false;
    }

    const esp_err_t event_loop_error = esp_event_loop_create_default();
    if (event_loop_error != ESP_OK && event_loop_error != ESP_ERR_INVALID_STATE) {
        ESP_LOGE("WiFi", "Failed to create default event loop");
        return false;
    }

    if (esp_netif_create_default_wifi_sta() == nullptr) {
        ESP_LOGE("WiFi", "Failed to create station network interface");
        return false;
    }

    if (esp_event_handler_register(
            WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, nullptr) != ESP_OK) {
        ESP_LOGE("WiFi", "Failed to register Wi-Fi event handler");
        return false;
    }

    wifi_infrastructure_initialized = true;
    return true;
}

bool initialize_wifi_driver() {
    if (wifi_initialized) {
        return true;
    }
    if (!initialize_wifi_infrastructure()) {
        return false;
    }

    wifi_init_config_t init_config = WIFI_INIT_CONFIG_DEFAULT();
    if (esp_wifi_init(&init_config) != ESP_OK) {
        ESP_LOGE("WiFi", "Failed to initialize Wi-Fi");
        return false;
    }
    wifi_initialized = true;

    if (esp_wifi_set_storage(WIFI_STORAGE_FLASH) != ESP_OK ||
        esp_wifi_set_mode(WIFI_MODE_STA) != ESP_OK) {
        ESP_LOGE("WiFi", "Failed to configure station mode");
        return false;
    }
    return true;
}

bool start_wifi() {
    if (wifi_started) {
        return true;
    }
    if (!initialize_wifi_driver()) {
        return false;
    }

    if (esp_wifi_start() != ESP_OK) {
        ESP_LOGE("WiFi", "Failed to start Wi-Fi");
        return false;
    }
    wifi_started = true;
    return true;
}

void connect_selected_network() {
    if (selected_slot >= kWifiListSlotCount ||
        scanned_ssids[selected_slot][0] == '\0') {
        return;
    }

    wifi_config_t config{};
    std::memcpy(config.sta.ssid, scanned_ssids[selected_slot],
                std::strlen(scanned_ssids[selected_slot]));
    std::memcpy(config.sta.password, password, std::strlen(password));
    config.sta.scan_method = WIFI_ALL_CHANNEL_SCAN;
    config.sta.sort_method = WIFI_CONNECT_AP_BY_SIGNAL;
    config.sta.failure_retry_cnt = 3U;

    suppress_next_reconnect.store(true, etl::memory_order_release);
    if (esp_wifi_disconnect() != ESP_OK) {
        suppress_next_reconnect.store(false, etl::memory_order_release);
    }
    if (esp_wifi_set_config(WIFI_IF_STA, &config) != ESP_OK) {
        ESP_LOGE("WiFi", "Failed to configure selected access point");
        return;
    }
    if (esp_wifi_connect() != ESP_OK) {
        ESP_LOGE("WiFi", "Failed to connect to selected access point");
    }
}

void clear_wifi_list(ListItem* wifi_lists, size_t wifi_lists_length) {
    if (wifi_lists == nullptr || wifi_lists_length <= kWifiListFirstSlot) {
        return;
    }

    const size_t end = wifi_lists_length < kWifiListFirstSlot + kWifiListSlotCount
        ? wifi_lists_length
        : kWifiListFirstSlot + kWifiListSlotCount;
    for (size_t i = kWifiListFirstSlot; i < end; ++i) {
        wifi_lists[i].title[0] = '\0';
        wifi_lists[i].accessory = {};
        scanned_ssids[i - kWifiListFirstSlot][0] = '\0';
    }
    clear_connected_slots();
}

} // namespace

bool wifi_assists_start_saved_network() {
    if (!initialize_wifi_driver()) {
        return false;
    }

    wifi_config_t saved_config{};
    if (esp_wifi_get_config(WIFI_IF_STA, &saved_config) != ESP_OK ||
        saved_config.sta.ssid[0] == '\0') {
        return false;
    }

    if (!start_wifi()) {
        return false;
    }
    if (esp_wifi_connect() != ESP_OK) {
        ESP_LOGE("WiFi", "Failed to auto-connect to saved access point");
    }
    return true;
}

bool wifi_assists_is_started() {
    return wifi_started;
}

void wifi_assists_process(PixelUI& ui) {
    if (reconnect_requested.exchange(false, etl::memory_order_acq_rel) &&
        wifi_started) {
        esp_wifi_connect();
    }

    if (!connection_state_changed.exchange(false, etl::memory_order_acq_rel)) {
        return;
    }

    wifi_ap_record_t connected_ap{};
    if (wifi_started && esp_wifi_sta_get_ap_info(&connected_ap) == ESP_OK) {
        update_connected_slot(
            connected_ap.ssid,
            std::strlen(reinterpret_cast<const char*>(connected_ap.ssid)));
    } else {
        clear_connected_slots();
    }
    ui.markDirty();
}

bool mutable_char_writer(char* src, const char* dst) {

    if (src == nullptr || dst == nullptr) {
        return false;
    }

    size_t written = 0;
    while (*dst != '\0' && written < MAX_LISTITEM_NAME_NUM - 1) {
        *src++ = *dst++;
        ++written;
    }
    *src = '\0';
    return true;
}

void wifi_assists_update_status(bool wifi_enabled,
    ListItem wifi_lists[],
    size_t wifi_lists_length)
{
    if (wifi_lists == nullptr || wifi_lists_length <= kWifiListFirstSlot) {
        return;
    }

    clear_wifi_list(wifi_lists, wifi_lists_length);

    if (!wifi_enabled) {
        if (wifi_started) {
            wifi_started = false;
            reconnect_requested.store(false, etl::memory_order_release);
            esp_wifi_stop();
        }
        return;
    }

    if (!start_wifi()) {
        return;
    }

    if (esp_wifi_scan_start(nullptr, true) != ESP_OK) {
        ESP_LOGE("WiFi", "Failed to scan for access points");
        return;
    }

    uint16_t ap_count = 0;
    if (esp_wifi_scan_get_ap_num(&ap_count) != ESP_OK || ap_count == 0U) {
        return;
    }

    const size_t slot_count = wifi_lists_length - kWifiListFirstSlot < kWifiListSlotCount
        ? wifi_lists_length - kWifiListFirstSlot
        : kWifiListSlotCount;
    const uint16_t record_capacity = ap_count < slot_count
        ? ap_count
        : static_cast<uint16_t>(slot_count);
    wifi_ap_record_t records[kWifiListSlotCount]{};
    uint16_t record_count = record_capacity;
    if (esp_wifi_scan_get_ap_records(&record_count, records) != ESP_OK) {
        ESP_LOGE("WiFi", "Failed to retrieve access points");
        esp_wifi_clear_ap_list();
        return;
    }

    size_t slot = 0;
    for (uint16_t i = 0; i < record_count && slot < slot_count; ++i) {
        if (records[i].ssid[0] == '\0') {
            continue;
        }
        mutable_char_writer(
            wifi_lists[kWifiListFirstSlot + slot].title,
            reinterpret_cast<const char*>(records[i].ssid));
        std::memcpy(scanned_ssids[slot], records[i].ssid, kSsidCapacity);
        scanned_ssids[slot][kSsidCapacity - 1U] = '\0';
        ++slot;
    }

    wifi_ap_record_t connected_ap{};
    if (esp_wifi_sta_get_ap_info(&connected_ap) == ESP_OK) {
        update_connected_slot(connected_ap.ssid,
                              std::strlen(reinterpret_cast<const char*>(connected_ap.ssid)));
    }

    for (size_t i = 0; i < slot; ++i) {
        wifi_lists[kWifiListFirstSlot + i].accessory =
            ListItemAccessory::check(connected_slots[i]);
    }
}

void wifi_assists_select_network(PixelUI& ui, ListItem wifi_lists[],
    size_t wifi_lists_length, size_t item_index)
{
    if (!wifi_initialized || wifi_lists == nullptr ||
        item_index < kWifiListFirstSlot || item_index >= wifi_lists_length) {
        return;
    }

    const size_t slot = item_index - kWifiListFirstSlot;
    if (slot >= kWifiListSlotCount || scanned_ssids[slot][0] == '\0') {
        return;
    }

    selected_slot = slot;
    password[0] = '\0';
    ui.showPopupKeyboard(password, sizeof(password), 120U, 60U, 0U,
                         connect_selected_network);
}

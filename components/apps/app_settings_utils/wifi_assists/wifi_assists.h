#pragma once

#include "ui/ListView/ListView.h"
#include "esp_wifi.h"
#include "esp_log.h"

bool wifi_assists_start_saved_network();
bool wifi_assists_is_started();
void wifi_assists_process(PixelUI& ui);
void wifi_assists_update_status(bool wifi_enabled, ListItem wifi_lists[], size_t wifi_lists_length);
void wifi_assists_select_network(PixelUI& ui, ListItem wifi_lists[], size_t wifi_lists_length, size_t item_index);

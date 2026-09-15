#pragma once

#include <cstddef>
#include <cstdint>
#include <ctime>

struct StorageStats {
    size_t totalBytes = 0U;
    size_t usedBytes = 0U;
};

struct StorageWindowInfo {
    size_t totalRecords = 0U;
    size_t loadedRecords = 0U;
    char firstDate[11] = {};
    char firstTime[9] = {};
    char lastDate[11] = {};
    char lastTime[9] = {};
};

enum class StorageEvent : uint8_t {
    None,
    Full,
    WriteError,
};

bool data_storage_init();
bool data_storage_ready();
bool data_storage_get_stats(StorageStats& stats);

bool data_storage_start_recording();
void data_storage_stop_recording();
bool data_storage_enqueue(const tm& time, int32_t doseFixed);
StorageEvent data_storage_take_event();

uint32_t data_storage_latest_file();
uint32_t data_storage_previous_file(uint32_t current);
uint32_t data_storage_next_file(uint32_t current);
bool data_storage_delete_file(uint32_t id);
size_t data_storage_count_records(uint32_t id);
bool data_storage_load_window(uint32_t id, size_t startRecord,
                              int32_t* values, size_t capacity,
                              StorageWindowInfo& info);

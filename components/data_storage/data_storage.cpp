#include "data_storage.h"
#include "data_storage_core.h"

#include <atomic>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <sys/stat.h>

#include "esp_littlefs.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

namespace {
constexpr char kTag[] = "data_storage";
constexpr char kPartition[] = "littlefs";
constexpr char kBasePath[] = "/littlefs";
constexpr size_t kQueueLength = 8U;
constexpr size_t kMinimumFreeBytes = 128U;

enum class CommandType : uint8_t { Start, Stop, Sample };
struct Command {
    CommandType type;
    tm time{};
    int32_t doseFixed = 0;
};

QueueHandle_t commandQueue = nullptr;
std::atomic<bool> ready{false};
std::atomic<bool> acceptingSamples{false};
std::atomic<StorageEvent> pendingEvent{StorageEvent::None};
std::atomic<uint32_t> openFileId{0U};

bool makePath(uint32_t id, char* path, size_t capacity) {
    return std::snprintf(path, capacity, "%s/%lu.csv", kBasePath,
                         static_cast<unsigned long>(id)) > 0;
}

bool parseFileId(const char* name, uint32_t& id) {
    if (!name || *name < '0' || *name > '9') return false;
    uint64_t result = 0U;
    const char* cursor = name;
    while (*cursor >= '0' && *cursor <= '9') {
        result = result * 10U + static_cast<uint32_t>(*cursor++ - '0');
        if (result > UINT32_MAX) return false;
    }
    if (std::strcmp(cursor, ".csv") != 0 || result == 0U) return false;
    id = static_cast<uint32_t>(result);
    return true;
}

bool fileExists(uint32_t id) {
    char path[40];
    makePath(id, path, sizeof(path));
    struct stat info{};
    return stat(path, &info) == 0;
}

uint32_t findLatest() {
    DIR* directory = opendir(kBasePath);
    if (!directory) return 0U;
    uint32_t latest = 0U;
    while (dirent* entry = readdir(directory)) {
        uint32_t id = 0U;
        if (parseFileId(entry->d_name, id) && id > latest) latest = id;
    }
    closedir(directory);
    return latest;
}

void publishFailure(StorageEvent event) {
    acceptingSamples.store(false, std::memory_order_release);
    pendingEvent.store(event, std::memory_order_release);
}

enum class RoomStatus : uint8_t { Available, Full, Error };

RoomStatus roomFor(size_t bytes) {
    StorageStats stats;
    if (!data_storage_get_stats(stats)) return RoomStatus::Error;
    return DataStorageCore::hasSpace(stats.totalBytes, stats.usedBytes, bytes,
                                     kMinimumFreeBytes)
        ? RoomStatus::Available : RoomStatus::Full;
}

void storageTask(void*) {
    FILE* file = nullptr;
    Command command{};
    for (;;) {
        if (xQueueReceive(commandQueue, &command, portMAX_DELAY) != pdTRUE) continue;
        if (command.type == CommandType::Stop) {
            acceptingSamples.store(false, std::memory_order_release);
            if (file) {
                std::fflush(file);
                std::fclose(file);
                file = nullptr;
            }
            openFileId.store(0U, std::memory_order_release);
            continue;
        }
        if (command.type == CommandType::Start) {
            if (file) std::fclose(file);
            file = nullptr;
            openFileId.store(0U, std::memory_order_release);
            const uint32_t latest = findLatest();
            const RoomStatus room = roomFor(32U);
            if (latest == UINT32_MAX || room == RoomStatus::Full) {
                publishFailure(StorageEvent::Full);
                continue;
            }
            if (room == RoomStatus::Error) {
                publishFailure(StorageEvent::WriteError);
                continue;
            }
            char path[40];
            makePath(latest + 1U, path, sizeof(path));
            file = std::fopen(path, "w");
            if (!file || std::fputs("date,time,value\n", file) < 0 || std::fflush(file) != 0) {
                const int error = errno;
                if (file) std::fclose(file);
                file = nullptr;
                publishFailure(error == ENOSPC ? StorageEvent::Full : StorageEvent::WriteError);
                continue;
            }
            openFileId.store(latest + 1U, std::memory_order_release);
            acceptingSamples.store(true, std::memory_order_release);
            continue;
        }
        if (!file || !acceptingSamples.load(std::memory_order_acquire)) continue;

        const int64_t magnitude = command.doseFixed < 0
            ? -static_cast<int64_t>(command.doseFixed)
            : static_cast<int64_t>(command.doseFixed);
        char line[48];
        const int length = std::snprintf(
            line, sizeof(line), "%04d-%02d-%02d,%02d:%02d:%02d,%s%ld.%04ld\n",
            command.time.tm_year + 1900, command.time.tm_mon + 1, command.time.tm_mday,
            command.time.tm_hour, command.time.tm_min, command.time.tm_sec,
            command.doseFixed < 0 ? "-" : "",
            static_cast<long>(magnitude / DataStorageCore::kDoseScale),
            static_cast<long>(magnitude % DataStorageCore::kDoseScale));
        const RoomStatus room = length > 0 && static_cast<size_t>(length) < sizeof(line)
            ? roomFor(static_cast<size_t>(length)) : RoomStatus::Error;
        if (room != RoomStatus::Available) {
            std::fclose(file);
            file = nullptr;
            openFileId.store(0U, std::memory_order_release);
            publishFailure(room == RoomStatus::Full
                ? StorageEvent::Full : StorageEvent::WriteError);
            continue;
        }
        if (std::fwrite(line, 1U, static_cast<size_t>(length), file) != static_cast<size_t>(length) ||
            std::fflush(file) != 0) {
            const int error = errno;
            std::fclose(file);
            file = nullptr;
            openFileId.store(0U, std::memory_order_release);
            publishFailure(error == ENOSPC ? StorageEvent::Full : StorageEvent::WriteError);
        }
    }
}

} // namespace

bool data_storage_init() {
    if (commandQueue) return data_storage_ready();
    esp_vfs_littlefs_conf_t config{};
    config.base_path = kBasePath;
    config.partition_label = kPartition;
    config.format_if_mount_failed = true;
    config.dont_mount = false;
    const esp_err_t result = esp_vfs_littlefs_register(&config);
    if (result != ESP_OK && result != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(kTag, "LittleFS mount failed: %s", esp_err_to_name(result));
        publishFailure(StorageEvent::WriteError);
        return false;
    }
    ready.store(true, std::memory_order_release);
    commandQueue = xQueueCreate(kQueueLength, sizeof(Command));
    if (!commandQueue) {
        ready.store(false, std::memory_order_release);
        publishFailure(StorageEvent::WriteError);
        return false;
    }
    if (xTaskCreate(storageTask, "StorageWriter", 4096, nullptr, 2, nullptr) != pdPASS) {
        vQueueDelete(commandQueue);
        commandQueue = nullptr;
        ready.store(false, std::memory_order_release);
        publishFailure(StorageEvent::WriteError);
        return false;
    }
    return true;
}

bool data_storage_ready() { return ready.load(std::memory_order_acquire); }

bool data_storage_get_stats(StorageStats& stats) {
    size_t total = 0U;
    size_t used = 0U;
    if (!data_storage_ready() || esp_littlefs_info(kPartition, &total, &used) != ESP_OK) return false;
    stats.totalBytes = total;
    stats.usedBytes = used;
    return true;
}

bool data_storage_start_recording() {
    if (!data_storage_ready() || !commandQueue) {
        publishFailure(StorageEvent::WriteError);
        return false;
    }
    Command command{CommandType::Start};
    if (xQueueSend(commandQueue, &command, 0) == pdTRUE) return true;
    publishFailure(StorageEvent::WriteError);
    return false;
}

void data_storage_stop_recording() {
    acceptingSamples.store(false, std::memory_order_release);
    if (!commandQueue) return;
    Command command{CommandType::Stop};
    if (xQueueSend(commandQueue, &command, 0) != pdTRUE) {
        // Stop is loss-tolerant by definition: discard queued samples so the
        // writer can close the active file promptly.
        xQueueReset(commandQueue);
        (void)xQueueSend(commandQueue, &command, 0);
    }
}

bool data_storage_enqueue(const tm& time, int32_t doseFixed) {
    if (!acceptingSamples.load(std::memory_order_acquire) || !commandQueue) return false;
    Command command{CommandType::Sample, time, doseFixed};
    if (xQueueSend(commandQueue, &command, 0) == pdTRUE) return true;
    publishFailure(StorageEvent::WriteError);
    return false;
}

StorageEvent data_storage_take_event() {
    return pendingEvent.exchange(StorageEvent::None, std::memory_order_acq_rel);
}

uint32_t data_storage_latest_file() { return data_storage_ready() ? findLatest() : 0U; }

uint32_t data_storage_previous_file(uint32_t current) {
    if (current <= 1U) return current;
    for (uint32_t id = current - 1U; id > 0U; --id) if (fileExists(id)) return id;
    return current;
}

uint32_t data_storage_next_file(uint32_t current) {
    const uint32_t latest = findLatest();
    for (uint32_t id = current + 1U; id <= latest; ++id) if (fileExists(id)) return id;
    return current;
}

bool data_storage_delete_file(uint32_t id) {
    if (id == 0U || openFileId.load(std::memory_order_acquire) == id) return false;
    char path[40];
    makePath(id, path, sizeof(path));
    return std::remove(path) == 0;
}

size_t data_storage_count_records(uint32_t id) {
    char path[40];
    makePath(id, path, sizeof(path));
    FILE* file = std::fopen(path, "r");
    if (!file) return 0U;
    char line[64];
    (void)std::fgets(line, sizeof(line), file);
    size_t count = 0U;
    char date[11]{};
    char time[9]{};
    int32_t value = 0;
    while (std::fgets(line, sizeof(line), file)) {
        if (DataStorageCore::parseCsvRecord(line, date, time, value)) {
            ++count;
        }
    }
    std::fclose(file);
    return count;
}

bool data_storage_load_window(uint32_t id, size_t startRecord,
                              int32_t* values, size_t capacity,
                              StorageWindowInfo& info) {
    info = {};
    if (!values || capacity == 0U || id == 0U) return false;
    char path[40];
    makePath(id, path, sizeof(path));
    FILE* file = std::fopen(path, "r");
    if (!file) return false;

    char line[64];
    (void)std::fgets(line, sizeof(line), file);
    size_t index = 0U;
    size_t loaded = 0U;
    char date[11]{};
    char time[9]{};
    int32_t value = 0;
    while (std::fgets(line, sizeof(line), file)) {
        if (!DataStorageCore::parseCsvRecord(line, date, time, value)) {
            continue;
        }
        if (index >= startRecord && loaded < capacity) {
            values[loaded++] = value;
            if (loaded == 1U) {
                std::memcpy(info.firstDate, date, sizeof(info.firstDate));
                std::memcpy(info.firstTime, time, sizeof(info.firstTime));
            }
            std::memcpy(info.lastDate, date, sizeof(info.lastDate));
            std::memcpy(info.lastTime, time, sizeof(info.lastTime));
        }
        ++index;
    }
    std::fclose(file);
    info.totalRecords = index;
    info.loadedRecords = loaded;
    return true;
}

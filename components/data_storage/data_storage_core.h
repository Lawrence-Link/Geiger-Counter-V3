#pragma once

#include <cstddef>
#include <cstdint>

namespace DataStorageCore {

constexpr int32_t kDoseScale = 10000;

constexpr bool isLeapYear(int32_t year) {
    return (year % 4 == 0) && ((year % 100 != 0) || (year % 400 == 0));
}

constexpr int32_t daysInMonth(int32_t year, int32_t month) {
    if (month < 1 || month > 12) return 31;
    if (month == 2 && isLeapYear(year)) return 29;
    return month == 2 ? 28 :
           (month == 4 || month == 6 || month == 9 || month == 11) ? 30 : 31;
}

constexpr bool hasSpace(size_t total, size_t used, size_t writeBytes,
                        size_t reserveBytes) {
    if (used > total) return false;
    const size_t freeBytes = total - used;
    return writeBytes <= freeBytes && reserveBytes <= freeBytes - writeBytes;
}

inline size_t windowStartForPercent(size_t recordCount, size_t capacity,
                                    int32_t percent) {
    if (recordCount <= capacity) return 0U;
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;
    const size_t maximumStart = recordCount - capacity;
    return static_cast<size_t>((static_cast<uint64_t>(maximumStart) *
                                static_cast<uint32_t>(percent)) / 100U);
}

bool parseDoseFixed(const char* text, int32_t& value);
bool parseCsvRecord(const char* line, char (&date)[11], char (&time)[9], int32_t& value);

} // namespace DataStorageCore

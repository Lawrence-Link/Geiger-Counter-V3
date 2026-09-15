#include "data_storage_core.h"

#include <climits>
#include <cstdio>

namespace DataStorageCore {

bool parseDoseFixed(const char* text, int32_t& value) {
    if (!text || *text == '\0') return false;

    bool negative = false;
    if (*text == '-') {
        negative = true;
        ++text;
    }
    if (*text < '0' || *text > '9') return false;

    int64_t whole = 0;
    while (*text >= '0' && *text <= '9') {
        whole = whole * 10 + (*text++ - '0');
        if (whole > INT32_MAX / kDoseScale) return false;
    }

    int32_t fraction = 0;
    int32_t digits = 0;
    if (*text == '.') {
        ++text;
        while (*text >= '0' && *text <= '9') {
            if (digits < 4) {
                fraction = fraction * 10 + (*text - '0');
                ++digits;
            }
            ++text;
        }
    }
    while (digits++ < 4) fraction *= 10;
    if (*text != '\0' && *text != '\r' && *text != '\n') return false;

    int64_t result = whole * kDoseScale + fraction;
    if (negative) result = -result;
    if (result < INT32_MIN || result > INT32_MAX) return false;
    value = static_cast<int32_t>(result);
    return true;
}

bool parseCsvRecord(const char* line, char (&date)[11], char (&time)[9], int32_t& value) {
    if (!line) return false;
    char valueText[20];
    return std::sscanf(line, "%10[^,],%8[^,],%19s", date, time, valueText) == 3 &&
           parseDoseFixed(valueText, value);
}

} // namespace DataStorageCore

#include "data_storage_core.h"

#include <cassert>
#include <cstdint>

int main() {
    using namespace DataStorageCore;

    static_assert(isLeapYear(2000));
    static_assert(!isLeapYear(2100));
    static_assert(isLeapYear(2024));
    static_assert(daysInMonth(2024, 2) == 29);
    static_assert(daysInMonth(2023, 2) == 28);
    static_assert(daysInMonth(2026, 4) == 30);
    static_assert(daysInMonth(2026, 12) == 31);
    static_assert(hasSpace(1024, 896, 0, 128));
    static_assert(hasSpace(1024, 864, 32, 128));
    static_assert(!hasSpace(1024, 865, 32, 128));
    static_assert(!hasSpace(1024, 1025, 0, 0));

    assert(windowStartForPercent(0, 124, 100) == 0);
    assert(windowStartForPercent(124, 124, 100) == 0);
    assert(windowStartForPercent(224, 124, 0) == 0);
    assert(windowStartForPercent(224, 124, 50) == 50);
    assert(windowStartForPercent(224, 124, 100) == 100);
    assert(windowStartForPercent(224, 124, -10) == 0);
    assert(windowStartForPercent(224, 124, 120) == 100);

    int32_t value = 0;
    assert(parseDoseFixed("0.0001", value) && value == 1);
    assert(parseDoseFixed("12.34", value) && value == 123400);
    assert(parseDoseFixed("10000.0000\n", value) && value == 100000000);
    assert(parseDoseFixed("-0.2500", value) && value == -2500);
    assert(parseDoseFixed("1.234567", value) && value == 12345);
    assert(!parseDoseFixed("", value));
    assert(!parseDoseFixed("abc", value));
    assert(!parseDoseFixed("1.0,extra", value));
    assert(!parseDoseFixed("999999999999.0", value));

    char date[11]{};
    char time[9]{};
    assert(parseCsvRecord("2026-09-15,12:34:56,1.2500\n", date, time, value));
    assert(value == 12500);
    assert(!parseCsvRecord("damaged row\n", date, time, value));
    assert(!parseCsvRecord("2026-09-15,12:34:56,1.0,extra\n", date, time, value));
}

/*
 * Copyright (C) 2025 Lawrence Link
 */

#pragma once

#include <stdint.h>

namespace PixelUITime {

constexpr uint32_t NO_WAKEUP = UINT32_MAX;
constexpr uint32_t MAX_DEADLINE_INTERVAL = UINT32_MAX / 2U;

/**
 * @brief Ensures that a periodic interval advances time.
 * @param intervalMs Requested interval in milliseconds.
 * @return @p intervalMs, or 1 when the requested interval is zero.
 */
inline uint32_t normalizeInterval(uint32_t intervalMs) {
    return (intervalMs == 0U) ? 1U : intervalMs;
}

/**
 * @brief Clamps an interval to the range supported by wrap-safe deadlines.
 * @param intervalMs Requested interval in milliseconds.
 * @return A value in the range [1, MAX_DEADLINE_INTERVAL].
 */
inline uint32_t normalizeDeadlineInterval(uint32_t intervalMs) {
    intervalMs = normalizeInterval(intervalMs);
    return (intervalMs > MAX_DEADLINE_INTERVAL)
        ? MAX_DEADLINE_INTERVAL
        : intervalMs;
}

/**
 * @brief Tests whether a wrap-safe absolute deadline has been reached.
 * @param currentTime Current monotonic time in milliseconds.
 * @param deadline Absolute deadline less than 2^31 milliseconds in the future.
 * @return true when @p deadline is current or past; false when it is future.
 */
inline bool deadlineReached(uint32_t currentTime, uint32_t deadline) {
    return (currentTime - deadline) < (UINT32_MAX / 2U + 1U);
}

/**
 * @brief Calculates the remaining delay to an absolute deadline.
 * @param currentTime Current monotonic time in milliseconds.
 * @param deadline Absolute deadline less than 2^31 milliseconds in the future.
 * @return Milliseconds remaining, or zero when the deadline has been reached.
 */
inline uint32_t untilDeadline(uint32_t currentTime, uint32_t deadline) {
    return deadlineReached(currentTime, deadline) ? 0U : deadline - currentTime;
}

/**
 * @brief Selects the shorter of two relative delays.
 * @param lhs First delay in milliseconds.
 * @param rhs Second delay in milliseconds.
 * @return The smaller delay.
 */
inline uint32_t earlier(uint32_t lhs, uint32_t rhs) {
    return (lhs < rhs) ? lhs : rhs;
}

} // namespace PixelUITime

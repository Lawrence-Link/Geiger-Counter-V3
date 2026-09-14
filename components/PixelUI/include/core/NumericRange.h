#pragma once

#include <stdint.h>

class NumericRange {
public:
    /** @brief Creates the default inclusive range [0, 100] with step 1. */
    constexpr NumericRange() = default;

    /**
     * @brief Validates and creates an inclusive numeric range.
     * @param minimum Inclusive lower bound.
     * @param maximum Inclusive upper bound.
     * @param step Positive amount used by increment and decrement operations.
     * @param output Range to update when all arguments are valid.
     * @return true on success; false when the bounds are reversed or the step
     *         is not positive. On failure, @p output is unchanged.
     */
    static constexpr bool tryCreate(
        int32_t minimum,
        int32_t maximum,
        int32_t step,
        NumericRange& output) {
        if (minimum > maximum || step <= 0) return false;
        output.minimum_ = minimum;
        output.maximum_ = maximum;
        output.step_ = step;
        return true;
    }

    /** @return The inclusive lower bound. */
    constexpr int32_t minimum() const { return minimum_; }

    /** @return The inclusive upper bound. */
    constexpr int32_t maximum() const { return maximum_; }

    /** @return The positive increment and decrement amount. */
    constexpr int32_t step() const { return step_; }

    /**
     * @brief Restricts a value to this range's inclusive bounds.
     * @param value Value to restrict.
     * @return The nearest value within the range.
     */
    constexpr int32_t clamp(int32_t value) const {
        return value < minimum_ ? minimum_ :
               value > maximum_ ? maximum_ : value;
    }

    /**
     * @brief Tests whether incrementing can move a value toward the maximum.
     * @param value Value to test after clamping it to the range.
     * @return true when the clamped value is below the maximum.
     */
    constexpr bool canIncrement(int32_t value) const {
        return clamp(value) < maximum_;
    }

    /**
     * @brief Tests whether decrementing can move a value toward the minimum.
     * @param value Value to test after clamping it to the range.
     * @return true when the clamped value is above the minimum.
     */
    constexpr bool canDecrement(int32_t value) const {
        return clamp(value) > minimum_;
    }

    /**
     * @brief Clamps a value, increments it by one step, and saturates at maximum.
     * @param value Value to increment.
     * @return The incremented value within the range.
     */
    constexpr int32_t incremented(int32_t value) const {
        const int32_t current = clamp(value);
        if (current >= maximum_) return maximum_;
        const int64_t candidate = static_cast<int64_t>(current) + step_;
        return candidate >= static_cast<int64_t>(maximum_)
            ? maximum_
            : static_cast<int32_t>(candidate);
    }

    /**
     * @brief Clamps a value, decrements it by one step, and saturates at minimum.
     * @param value Value to decrement.
     * @return The decremented value within the range.
     */
    constexpr int32_t decremented(int32_t value) const {
        const int32_t current = clamp(value);
        if (current <= minimum_) return minimum_;
        const int64_t candidate = static_cast<int64_t>(current) - step_;
        return candidate <= static_cast<int64_t>(minimum_)
            ? minimum_
            : static_cast<int32_t>(candidate);
    }

private:
    int32_t minimum_ = 0;
    int32_t maximum_ = 100;
    int32_t step_ = 1;
};

/**
 * @brief Maps a ranged value to an unsigned extent using nearest rounding.
 * @param range Inclusive source range.
 * @param value Source value, clamped to @p range before mapping.
 * @param extent Inclusive upper bound of the destination range.
 * @return A value from zero through @p extent, or zero for a zero-span range
 *         or zero extent.
 */
constexpr uint32_t normalizeToExtent(
    const NumericRange& range,
    int32_t value,
    uint32_t extent) {
    const uint64_t span = static_cast<uint64_t>(
        static_cast<int64_t>(range.maximum()) - range.minimum());
    if (span == 0U || extent == 0U) return 0U;

    const int32_t clamped = range.clamp(value);
    const uint64_t offset = static_cast<uint64_t>(
        static_cast<int64_t>(clamped) - range.minimum());
    const uint64_t numerator = offset * extent;
    return static_cast<uint32_t>((numerator + span / 2U) / span);
}

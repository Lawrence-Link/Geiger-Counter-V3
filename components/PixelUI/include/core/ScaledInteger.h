#pragma once

#include <stdint.h>

template <int32_t Scale>
class ScaledInt32 {
    static_assert(Scale > 0, "ScaledInt32 scale must be positive");

public:
    static constexpr int32_t scale = Scale;

    /** @brief Creates a scaled integer with a raw value of zero. */
    constexpr ScaledInt32() = default;

    /**
     * @brief Creates a scaled integer from its stored representation.
     * @param raw Integer value expressed in units of 1 / Scale.
     * @return Scaled integer containing @p raw without conversion.
     */
    static constexpr ScaledInt32 fromRaw(int32_t raw) {
        return ScaledInt32(raw);
    }

    /** @return The stored integer value expressed in units of 1 / Scale. */
    constexpr int32_t raw() const { return raw_; }

    /**
     * @brief Compares two scaled integers by their raw values.
     * @return true when both raw values are equal.
     */
    friend constexpr bool operator==(ScaledInt32, ScaledInt32) = default;

private:
    /**
     * @brief Stores an already scaled raw value.
     * @param raw Integer value expressed in units of 1 / Scale.
     */
    explicit constexpr ScaledInt32(int32_t raw) : raw_(raw) {}

    int32_t raw_ = 0;
};

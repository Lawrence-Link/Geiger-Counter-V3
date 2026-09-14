#pragma once

#include <stddef.h>
#include <stdint.h>

#include "core/NumericRange.h"

class FixedBufferWriter {
public:
    // buffer is non-owning and must remain valid for the writer's lifetime.
    /**
     * @brief Creates a writer over a caller-owned fixed buffer.
     * @param buffer Destination buffer, retained without taking ownership.
     * @param capacity Buffer capacity in bytes.
     */
    FixedBufferWriter(char* buffer, size_t capacity);

    /** @param text Null-terminated text to append; nullptr is ignored. */
    bool append(const char* text);
    /** @param character Character to append. @return false when it does not fit. */
    bool appendCharacter(char character);
    /** @param value Signed value to append. @param minimumDigits Minimum digit count. */
    bool appendInteger(int32_t value, uint8_t minimumDigits = 1U);
    /** @param value Unsigned value to append. @param minimumDigits Minimum digit count. */
    bool appendUnsigned(uint32_t value, uint8_t minimumDigits = 1U);
    /** @brief Terminates the output and reports whether the writer is valid. */
    bool finish();

    /** @return Whether all writes so far fit in the buffer. */
    bool valid() const { return valid_; }
    /** @return Number of output characters excluding the null terminator. */
    size_t size() const { return position_; }

private:
    /** @brief Reserves space for count characters plus a null terminator. */
    bool reserve(size_t count);
    /** @brief Marks the writer invalid and clears any partial output. */
    bool fail();

    char* buffer_ = nullptr;
    size_t capacity_ = 0U;
    size_t position_ = 0U;
    bool valid_ = false;
};

struct IntegerFormat {
    uint8_t minimumDigits = 1U;
    const char* suffix = nullptr;
};

struct ScaledIntegerFormat {
    uint32_t scale = 1U;
    uint8_t fractionalDigits = 0U;
    uint8_t minimumIntegerDigits = 1U;
    const char* suffix = nullptr;
};

struct PercentageFormat {
    const NumericRange* range = nullptr;
    const char* suffix = "%";
};

class NumericFormatter {
public:
    using FormatFunction = bool (*)(
        const void* context,
        int32_t value,
        char* buffer,
        size_t bufferSize);

    /** @brief Creates an empty formatter with no formatting function. */
    constexpr NumericFormatter() = default;

    // context is non-owning and must outlive every format() call made through
    // the returned formatter. The same rule applies to suffix/range pointers
    // stored inside IntegerFormat, ScaledIntegerFormat and PercentageFormat.
    /**
     * @brief Creates a formatter backed by a caller-supplied function.
     * @param context Non-owning context passed to the function.
     * @param function Formatting function to invoke.
     * @note The context must outlive every format() call.
     */
    static constexpr NumericFormatter custom(
        const void* context,
        FormatFunction function) {
        return NumericFormatter(context, function);
    }

    /** @brief Creates a formatter using default integer formatting. */
    static constexpr NumericFormatter integer() {
        return NumericFormatter(nullptr, &formatInteger);
    }

    /**
     * @brief Creates an integer formatter with explicit minimum digits and suffix.
     * @param format Non-owning formatting options.
     */
    static constexpr NumericFormatter integer(const IntegerFormat& format) {
        return NumericFormatter(&format, &formatInteger);
    }

    /**
     * @brief Creates a formatter for a scaled integer with decimal output.
     * @param format Non-owning scaling and suffix options.
     */
    static constexpr NumericFormatter scaled(const ScaledIntegerFormat& format) {
        return NumericFormatter(&format, &formatScaled);
    }

    /**
     * @brief Creates a formatter that maps a numeric range to a percentage.
     * @param format Non-owning range and suffix options.
     */
    static constexpr NumericFormatter percentage(const PercentageFormat& format) {
        return NumericFormatter(&format, &formatPercentage);
    }

    /**
     * @brief Formats a value into a caller-owned fixed buffer.
     * @param value Value to format.
     * @param buffer Destination buffer.
     * @param bufferSize Buffer capacity in bytes.
     * @return true when formatting succeeds and the output fits.
     */
    bool format(int32_t value, char* buffer, size_t bufferSize) const;
    /** @return Whether a formatting function is bound. */
    constexpr bool valid() const { return function_ != nullptr; }
    /** @return The non-owning formatting context pointer. */
    constexpr const void* context() const { return context_; }
    /** @return The bound formatting function, or nullptr when empty. */
    constexpr FormatFunction function() const { return function_; }

private:
    /** @brief Stores the non-owning context and formatting function. */
    constexpr NumericFormatter(const void* context, FormatFunction function)
        : context_(context), function_(function) {}

    /** @brief Formats an integer using IntegerFormat options. */
    static bool formatInteger(
        const void* context, int32_t value, char* buffer, size_t bufferSize);
    /** @brief Formats a scaled integer using nearest decimal rounding. */
    static bool formatScaled(
        const void* context, int32_t value, char* buffer, size_t bufferSize);
    /** @brief Formats a range value as a percentage with its configured suffix. */
    static bool formatPercentage(
        const void* context, int32_t value, char* buffer, size_t bufferSize);

    const void* context_ = nullptr;
    FormatFunction function_ = nullptr;
};

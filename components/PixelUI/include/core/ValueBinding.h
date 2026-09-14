#pragma once

#include <stddef.h>
#include <stdint.h>

#include "core/ScaledInteger.h"

namespace PixelUIValue {

/**
 * @brief Formats a scaled integer with an optional suffix.
 * @param buffer Destination buffer.
 * @param bufferSize Size of @p buffer in bytes.
 * @param raw Stored integer value expressed in units of 1 / @p scale.
 * @param scale Positive scale that relates @p raw to the displayed value.
 * @param fractionalDigits Number of digits to emit after the decimal point.
 * @param suffix Optional null-terminated suffix appended to the value.
 * @return true when the complete value fits and the format is valid.
 */
bool formatScaledInteger(
    char* buffer,
    size_t bufferSize,
    int32_t raw,
    uint32_t scale,
    uint8_t fractionalDigits,
    const char* suffix = nullptr);

class Binding {
public:
    using FormatFunction = bool (*)(
        const void* object,
        const char* suffix,
        char* buffer,
        size_t bufferSize);

    /** @brief Creates an empty binding that cannot format a value. */
    constexpr Binding() = default;

    /**
     * @brief Creates a binding backed by a custom formatting function.
     * @param object Non-owning object passed to @p formatter.
     * @param formatter Function that formats @p object.
     * @param suffix Optional non-owning suffix passed to @p formatter.
     * @return Binding that references the supplied object, formatter, and suffix.
     * @note @p object and @p suffix must outlive every call to format().
     */
    static constexpr Binding custom(
        const void* object,
        FormatFunction formatter,
        const char* suffix = nullptr) {
        return Binding(object, formatter, suffix);
    }

    /**
     * @brief Creates a binding for a signed integer.
     * @param value Non-owning reference to the value to format.
     * @param suffix Optional non-owning suffix appended to the value.
     * @return Binding that reads @p value when format() is called.
     * @note @p value and @p suffix must outlive every call to format().
     */
    static constexpr Binding integer(
        const int32_t& value,
        const char* suffix = nullptr) {
        return Binding(&value, &formatInteger, suffix);
    }

    template <uint8_t FractionalDigits, int32_t Scale>
    /**
     * @brief Creates a decimal binding for a compile-time scaled integer.
     * @tparam FractionalDigits Number of digits to display after the decimal point.
     * @tparam Scale Positive scale used by the stored integer.
     * @param value Non-owning reference to the scaled value.
     * @param suffix Optional non-owning suffix appended to the value.
     * @return Binding that reads @p value when format() is called.
     * @note @p value and @p suffix must outlive every call to format().
     */
    static constexpr Binding decimal(
        const ScaledInt32<Scale>& value,
        const char* suffix = nullptr) {
        static_assert(FractionalDigits <= 9U,
                      "ValueBinding supports at most 9 fractional digits");
        return Binding(
            &value,
            &formatDecimal<FractionalDigits, Scale>,
            suffix);
    }

    /**
     * @brief Formats the currently bound value.
     * @param buffer Destination buffer.
     * @param bufferSize Size of @p buffer in bytes.
     * @return true when a formatter is bound and the complete output is written.
     * @note A writable non-empty buffer is cleared when formatting fails.
     */
    bool format(char* buffer, size_t bufferSize) const {
        if (buffer != nullptr && bufferSize != 0U) buffer[0] = '\0';
        if (formatter_ == nullptr) {
            return false;
        }
        const bool formatted =
            formatter_(object_, suffix_, buffer, bufferSize);
        if (!formatted && buffer != nullptr && bufferSize != 0U) {
            buffer[0] = '\0';
        }
        return formatted;
    }

    /** @return The non-owning object pointer supplied to the formatter. */
    constexpr const void* object() const { return object_; }

    /** @return The bound formatting function, or nullptr for an empty binding. */
    constexpr FormatFunction formatter() const { return formatter_; }

    /** @return The optional non-owning suffix pointer. */
    constexpr const char* suffix() const { return suffix_; }

private:
    /**
     * @brief Stores the non-owning parts of a binding.
     * @param object Object passed to the formatter.
     * @param formatter Function used to format the object.
     * @param suffix Optional suffix passed to the formatter.
     */
    constexpr Binding(
        const void* object,
        FormatFunction formatter,
        const char* suffix)
        : object_(object), formatter_(formatter), suffix_(suffix) {}

    /** @brief Adapts a bound int32_t to the scaled-integer formatter. */
    static bool formatInteger(
        const void* object,
        const char* suffix,
        char* buffer,
        size_t bufferSize) {
        if (object == nullptr) return false;
        return formatScaledInteger(
            buffer,
            bufferSize,
            *static_cast<const int32_t*>(object),
            1U,
            0U,
            suffix);
    }

    template <uint8_t FractionalDigits, int32_t Scale>
    /** @brief Adapts a bound ScaledInt32 to the scaled-integer formatter. */
    static bool formatDecimal(
        const void* object,
        const char* suffix,
        char* buffer,
        size_t bufferSize) {
        if (object == nullptr) return false;
        return formatScaledInteger(
            buffer,
            bufferSize,
            static_cast<const ScaledInt32<Scale>*>(object)->raw(),
            static_cast<uint32_t>(Scale),
            FractionalDigits,
            suffix);
    }

    const void* object_ = nullptr;
    FormatFunction formatter_ = nullptr;
    const char* suffix_ = nullptr;
};

} // namespace PixelUIValue

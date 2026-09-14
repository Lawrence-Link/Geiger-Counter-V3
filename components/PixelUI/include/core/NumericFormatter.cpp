#include "core/NumericFormatter.h"

namespace {

/** @brief Calculates 10 raised to a digit count no greater than nine. */
uint32_t powerOfTen(uint8_t exponent) {
    uint32_t result = 1U;
    while (exponent-- != 0U) result *= 10U;
    return result;
}

} // namespace

FixedBufferWriter::FixedBufferWriter(char* buffer, size_t capacity)
    : buffer_(buffer), capacity_(capacity), valid_(buffer != nullptr && capacity != 0U) {
    if (valid_) buffer_[0] = '\0';
}

bool FixedBufferWriter::fail() {
    valid_ = false;
    position_ = 0U;
    if (buffer_ != nullptr && capacity_ != 0U) buffer_[0] = '\0';
    return false;
}

bool FixedBufferWriter::reserve(size_t count) {
    if (!valid_ || count >= capacity_ || position_ > capacity_ - count - 1U) {
        return fail();
    }
    return true;
}

bool FixedBufferWriter::append(const char* text) {
    if (text == nullptr) return true;
    size_t length = 0U;
    while (text[length] != '\0') ++length;
    if (!reserve(length)) return false;
    for (size_t i = 0U; i < length; ++i) buffer_[position_++] = text[i];
    buffer_[position_] = '\0';
    return true;
}

bool FixedBufferWriter::appendCharacter(char character) {
    if (!reserve(1U)) return false;
    buffer_[position_++] = character;
    buffer_[position_] = '\0';
    return true;
}

bool FixedBufferWriter::appendUnsigned(uint32_t value, uint8_t minimumDigits) {
    char reversed[10];
    size_t digitCount = 0U;
    do {
        reversed[digitCount++] = static_cast<char>('0' + value % 10U);
        value /= 10U;
    } while (value != 0U);

    const size_t requestedDigits = minimumDigits > digitCount
        ? minimumDigits
        : digitCount;
    if (!reserve(requestedDigits)) return false;
    for (size_t i = digitCount; i < requestedDigits; ++i) {
        buffer_[position_++] = '0';
    }
    while (digitCount != 0U) buffer_[position_++] = reversed[--digitCount];
    buffer_[position_] = '\0';
    return true;
}

bool FixedBufferWriter::appendInteger(int32_t value, uint8_t minimumDigits) {
    const bool negative = value < 0;
    const uint32_t magnitude = negative
        ? 0U - static_cast<uint32_t>(value)
        : static_cast<uint32_t>(value);
    if (negative && !appendCharacter('-')) return false;
    return appendUnsigned(magnitude, minimumDigits);
}

bool FixedBufferWriter::finish() {
    if (!valid_) return fail();
    buffer_[position_] = '\0';
    return true;
}

bool NumericFormatter::format(
    int32_t value, char* buffer, size_t bufferSize) const {
    if (buffer != nullptr && bufferSize != 0U) buffer[0] = '\0';
    if (function_ == nullptr || buffer == nullptr || bufferSize == 0U) return false;
    const bool formatted = function_(context_, value, buffer, bufferSize);
    if (!formatted) buffer[0] = '\0';
    return formatted;
}

bool NumericFormatter::formatInteger(
    const void* context, int32_t value, char* buffer, size_t bufferSize) {
    const IntegerFormat defaults{};
    const IntegerFormat& format = context == nullptr
        ? defaults
        : *static_cast<const IntegerFormat*>(context);
    FixedBufferWriter writer(buffer, bufferSize);
    return writer.appendInteger(value, format.minimumDigits) &&
           writer.append(format.suffix) && writer.finish();
}

bool NumericFormatter::formatScaled(
    const void* context, int32_t value, char* buffer, size_t bufferSize) {
    if (context == nullptr) return false;
    const ScaledIntegerFormat& format =
        *static_cast<const ScaledIntegerFormat*>(context);
    if (format.scale == 0U || format.fractionalDigits > 9U) return false;

    const bool negative = value < 0;
    const uint32_t magnitude = negative
        ? 0U - static_cast<uint32_t>(value)
        : static_cast<uint32_t>(value);
    uint32_t whole = magnitude / format.scale;
    const uint32_t remainder = magnitude % format.scale;
    const uint32_t fractionalScale = powerOfTen(format.fractionalDigits);
    uint32_t fractional = static_cast<uint32_t>(
        (static_cast<uint64_t>(remainder) * fractionalScale + format.scale / 2U) /
        format.scale);
    if (fractional == fractionalScale) {
        ++whole;
        fractional = 0U;
    }

    FixedBufferWriter writer(buffer, bufferSize);
    if (negative && !writer.appendCharacter('-')) return false;
    if (!writer.appendUnsigned(whole, format.minimumIntegerDigits)) return false;
    if (format.fractionalDigits != 0U) {
        if (!writer.appendCharacter('.') ||
            !writer.appendUnsigned(fractional, format.fractionalDigits)) return false;
    }
    return writer.append(format.suffix) && writer.finish();
}

bool NumericFormatter::formatPercentage(
    const void* context, int32_t value, char* buffer, size_t bufferSize) {
    if (context == nullptr) return false;
    const PercentageFormat& format =
        *static_cast<const PercentageFormat*>(context);
    if (format.range == nullptr) return false;

    FixedBufferWriter writer(buffer, bufferSize);
    return writer.appendUnsigned(normalizeToExtent(*format.range, value, 100U)) &&
           writer.append(format.suffix) && writer.finish();
}

#include "core/ValueBinding.h"
#include "core/NumericFormatter.h"

namespace PixelUIValue {

bool formatScaledInteger(
    char* buffer,
    size_t bufferSize,
    int32_t raw,
    uint32_t scale,
    uint8_t fractionalDigits,
    const char* suffix) {
    const ScaledIntegerFormat format{
        scale, fractionalDigits, 1U, suffix};
    return NumericFormatter::scaled(format).format(raw, buffer, bufferSize);
}

} // namespace PixelUIValue

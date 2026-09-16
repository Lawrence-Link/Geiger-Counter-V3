#pragma once

#include <cstdint>

class Canvas;

namespace HelloAnimation {

struct Bounds {
    int32_t x;
    int32_t y;
    int32_t width;
    int32_t height;
};

uint32_t durationMs();
void draw(Canvas& canvas, uint32_t elapsedMs, const Bounds& bounds);

}  // namespace HelloAnimation

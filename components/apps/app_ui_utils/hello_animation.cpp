/*
 * Copyright (C) 2025 Lawrence Link
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "app_ui_utils/hello_animation.h"

#include "PixelUI.h"

#include <cstddef>

namespace {
constexpr uint32_t kHelloFirstStrokeMs = 400U;
constexpr uint32_t kHelloSecondStrokeDelayMs = 300U;
constexpr uint32_t kHelloSecondStrokeMs = 1600U;
constexpr uint32_t kHelloDurationMs =
    kHelloSecondStrokeDelayMs + kHelloSecondStrokeMs;
constexpr int32_t kDesignX = 22;
constexpr int32_t kDesignY = 14;
constexpr int32_t kDesignWidth = 84;
constexpr int32_t kDesignHeight = 37;
constexpr int32_t kScaleOne = 1024;
constexpr int32_t kHelloCenterY = 32;
constexpr int32_t kHelloHeightNumerator = 4;
constexpr int32_t kHelloHeightDenominator = 7;

struct Point {
    int16_t x;
    int16_t y;
};

struct ScaleTransform {
    int32_t x;
    int32_t y;
    int32_t scale;
    int32_t strokeWidth;
};

struct CubicSegment {
    Point p0;
    Point p1;
    Point p2;
    Point p3;
    uint8_t samples;
};

// Apple-style "hello" path, scaled from the 638 x 200 SVG into the
// x=22, y=14, w=84, h=37 area of the 128 x 64 display.
// Sample counts approximate each segment's arc length so that writing speed
// remains visually uniform instead of accelerating on longer Bezier curves.
constexpr CubicSegment kHelloStroke1[] = {
    {{24, 43}, {28, 41}, {31, 37}, {34, 32}, 14},
    {{34, 32}, {37, 28}, {38, 23}, {38, 20}, 11},
    {{38, 20}, {38, 18}, {37, 16}, {36, 16}, 4},
    {{36, 16}, {34, 16}, {33, 18}, {32, 22}, 7},
    {{32, 22}, {32, 26}, {31, 31}, {30, 47}, 19},
};

constexpr CubicSegment kHelloStroke2[] = {
    {{30, 46}, {31, 38}, {33, 32}, {37, 32}, 15},
    {{37, 32}, {39, 32}, {40, 34}, {40, 37}, 6},
    {{40, 37}, {39, 39}, {39, 41}, {39, 43}, 5},
    {{39, 43}, {39, 45}, {40, 48}, {42, 48}, 6},
    {{42, 48}, {46, 48}, {51, 44}, {53, 40}, 13},
    {{53, 40}, {54, 38}, {54, 37}, {54, 35}, 4},
    {{54, 35}, {54, 33}, {53, 31}, {51, 31}, 5},
    {{51, 31}, {49, 31}, {47, 35}, {47, 39}, 9},
    {{47, 39}, {47, 44}, {49, 48}, {53, 48}, 11},
    {{53, 48}, {59, 48}, {66, 38}, {69, 28}, 24},
    {{69, 28}, {69, 25}, {70, 22}, {70, 20}, 6},
    {{70, 20}, {70, 18}, {69, 16}, {68, 16}, 4},
    {{68, 16}, {66, 16}, {65, 18}, {64, 20}, 6},
    {{64, 20}, {63, 23}, {63, 27}, {62, 32}, 10},
    {{62, 32}, {61, 43}, {63, 48}, {67, 48}, 15},
    {{67, 48}, {73, 48}, {78, 38}, {81, 28}, 23},
    {{81, 28}, {82, 25}, {82, 22}, {82, 20}, 6},
    {{82, 20}, {82, 18}, {82, 16}, {80, 16}, 4},
    {{80, 16}, {79, 16}, {78, 18}, {77, 20}, 5},
    {{77, 20}, {76, 23}, {75, 27}, {75, 32}, 10},
    {{75, 32}, {74, 43}, {76, 48}, {79, 48}, 14},
    {{79, 48}, {83, 48}, {85, 43}, {86, 39}, 11},
    {{86, 39}, {88, 34}, {89, 31}, {92, 31}, 10},
    {{92, 31}, {95, 31}, {97, 34}, {97, 38}, 9},
    {{97, 38}, {97, 44}, {94, 48}, {91, 48}, 11},
    {{91, 48}, {88, 48}, {86, 45}, {86, 40}, 9},
    {{86, 40}, {87, 35}, {89, 31}, {92, 31}, 11},
    {{92, 31}, {94, 31}, {95, 32}, {96, 33}, 4},
    {{96, 33}, {100, 36}, {102, 34}, {103, 31}, 9},
};

template <size_t N>
constexpr uint16_t sampleCount(const CubicSegment (&segments)[N]) {
    uint16_t count = 0U;
    for (const CubicSegment& segment : segments) count += segment.samples;
    return count;
}

constexpr uint16_t kHelloStroke1Samples = sampleCount(kHelloStroke1);
constexpr uint16_t kHelloStroke2Samples = sampleCount(kHelloStroke2);

uint16_t easeInOut(uint32_t elapsed, uint32_t duration) {
    if (elapsed >= duration) return 1024U;
    const uint32_t t = elapsed * 1024U / duration;
    return static_cast<uint16_t>(t * t * (3072U - 2U * t) / (1024U * 1024U));
}

int32_t interpolate(int32_t from, int32_t to, uint16_t t) {
    return from + ((to - from) * static_cast<int32_t>(t) + 128) / 256;
}

Point cubicPoint(const CubicSegment& segment, uint16_t t) {
    const int32_t x01 = interpolate(segment.p0.x, segment.p1.x, t);
    const int32_t x12 = interpolate(segment.p1.x, segment.p2.x, t);
    const int32_t x23 = interpolate(segment.p2.x, segment.p3.x, t);
    const int32_t x012 = interpolate(x01, x12, t);
    const int32_t x123 = interpolate(x12, x23, t);

    const int32_t y01 = interpolate(segment.p0.y, segment.p1.y, t);
    const int32_t y12 = interpolate(segment.p1.y, segment.p2.y, t);
    const int32_t y23 = interpolate(segment.p2.y, segment.p3.y, t);
    const int32_t y012 = interpolate(y01, y12, t);
    const int32_t y123 = interpolate(y12, y23, t);

    return {
        static_cast<int16_t>(interpolate(x012, x123, t)),
        static_cast<int16_t>(interpolate(y012, y123, t)),
    };
}

Point compressHelloHeight(Point point) {
    const int32_t centeredY = point.y - kHelloCenterY;
    point.y = static_cast<int16_t>(
        kHelloCenterY + centeredY * kHelloHeightNumerator /
                            kHelloHeightDenominator);
    return point;
}

ScaleTransform makeTransform(const HelloAnimation::Bounds& bounds) {
    const int32_t scaleX = bounds.width * kScaleOne / kDesignWidth;
    const int32_t scaleY = bounds.height * kScaleOne / kDesignHeight;
    const int32_t scale = scaleX < scaleY ? scaleX : scaleY;
    const int32_t drawnWidth = kDesignWidth * scale / kScaleOne;
    const int32_t drawnHeight = kDesignHeight * scale / kScaleOne;
    const int32_t scaledStrokeWidth = 2;
        // (3 * scale + kScaleOne / 2) / kScaleOne;

    return {
        bounds.x + (bounds.width - drawnWidth) / 2,
        bounds.y + (bounds.height - drawnHeight) / 2,
        scale,
        scaledStrokeWidth > 0 ? scaledStrokeWidth : 1,
    };
}

Point scalePoint(Point point, const ScaleTransform& transform) {
    point = compressHelloHeight(point);
    return {
        static_cast<int16_t>(
            transform.x +
            (point.x - kDesignX) * transform.scale / kScaleOne),
        static_cast<int16_t>(
            transform.y +
            (point.y - kDesignY) * transform.scale / kScaleOne),
    };
}

int32_t strokeFirstOffset(int32_t width) {
    return -(width / 2);
}

void drawStrokePoint(Canvas& canvas, Point point, int32_t width) {
    canvas.drawVLine(point.x, point.y + strokeFirstOffset(width), width);
}

void drawThickLine(Canvas& canvas, Point from, Point to, int32_t width) {
    const int32_t firstOffset = strokeFirstOffset(width);
    for (int32_t offset = firstOffset;
         offset < firstOffset + width; ++offset) {
        canvas.drawLine(from.x, from.y + offset, to.x, to.y + offset);
    }
}

template <size_t N>
void drawStroke(Canvas& canvas, const CubicSegment (&segments)[N],
                uint16_t visibleSamples, const ScaleTransform& transform) {
    if (visibleSamples == 0U) return;

    Point last = scalePoint(segments[0].p0, transform);
    drawStrokePoint(canvas, last, transform.strokeWidth);
    for (const CubicSegment& segment : segments) {
        const uint16_t count = visibleSamples < segment.samples
                                   ? visibleSamples
                                   : segment.samples;
        for (uint16_t i = 1U; i <= count; ++i) {
            const uint16_t t = static_cast<uint16_t>(
                (i * 256U + segment.samples / 2U) / segment.samples);
            const Point next = scalePoint(cubicPoint(segment, t), transform);
            drawThickLine(canvas, last, next, transform.strokeWidth);
            last = next;
        }
        if (visibleSamples <= segment.samples) break;
        visibleSamples -= segment.samples;
    }
    drawStrokePoint(canvas, last, transform.strokeWidth);
}
}  // namespace

uint32_t HelloAnimation::durationMs() {
    return kHelloDurationMs;
}

void HelloAnimation::draw(Canvas& canvas, uint32_t elapsed,
                          const Bounds& bounds) {
    if (bounds.width <= 0 || bounds.height <= 0) return;

    const ScaleTransform transform = makeTransform(bounds);
    if (transform.scale <= 0) return;

    canvas.setDrawColor(1);

    const uint16_t firstProgress = easeInOut(elapsed, kHelloFirstStrokeMs);
    drawStroke(canvas, kHelloStroke1,
               kHelloStroke1Samples * firstProgress / 1024U, transform);

    if (elapsed >= kHelloSecondStrokeDelayMs) {
        const uint16_t secondProgress = easeInOut(
            elapsed - kHelloSecondStrokeDelayMs, kHelloSecondStrokeMs);
        drawStroke(canvas, kHelloStroke2,
                   kHelloStroke2Samples * secondProgress / 1024U, transform);
    }
}

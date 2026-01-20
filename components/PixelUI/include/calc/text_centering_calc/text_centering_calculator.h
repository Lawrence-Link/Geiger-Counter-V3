#pragma once

#include "PixelUI.h"

struct Rect {
    int16_t x;
    int16_t y;
    int16_t w;
    int16_t h;
};

struct TextPos {
    int16_t x;
    int16_t y;   // baseline
};

enum class TextAlignX {
    Left,
    Center,
    Right
};

enum class TextAlignY {
    Top,
    Center,
    Bottom,
    Baseline
};

class TextAlignHelper {
public:

    // calculate X position for centered text
    static int16_t calcCenteredX(
        u8g2_t* u8g2,
        int16_t area_x,
        int16_t area_w,
        const char* utf8
    ) {
        int16_t text_width = u8g2_GetUTF8Width(u8g2, utf8);
        return area_x + (area_w - text_width) / 2;
    }

    static int16_t calcAlignedX(
        u8g2_t* u8g2,
        int16_t area_x,
        int16_t area_w,
        const char* utf8,
        TextAlignX align
    ) {
        int16_t text_width = u8g2_GetUTF8Width(u8g2, utf8);

        switch (align) {
        case TextAlignX::Left:
            return area_x;

        case TextAlignX::Center:
            return area_x + (area_w - text_width) / 2;

        case TextAlignX::Right:
            return area_x + area_w - text_width;

        default:
            return area_x;
        }
    }

    static int16_t calcAlignedBaselineY(
        u8g2_t* u8g2,
        int16_t area_y,
        int16_t area_h,
        TextAlignY align
    ) {
        int16_t ascent  = u8g2_GetAscent(u8g2);
        int16_t descent = u8g2_GetDescent(u8g2);
        int16_t text_height = ascent - descent;

        switch (align) {
        case TextAlignY::Top:
            return area_y + ascent;

        case TextAlignY::Center:
            return area_y + (area_h - text_height) / 2 + ascent;

        case TextAlignY::Bottom:
            return area_y + area_h + descent;

        case TextAlignY::Baseline:
            return area_y;

        default:
            return area_y + ascent;
        }
    }

    static TextPos calcTextPos(
        u8g2_t* u8g2,
        const Rect& area,
        const char* utf8,
        TextAlignX align_x,
        TextAlignY align_y
    ) {
        TextPos pos;
        pos.x = calcAlignedX(
            u8g2,
            area.x,
            area.w,
            utf8,
            align_x
        );

        pos.y = calcAlignedBaselineY(
            u8g2,
            area.y,
            area.h,
            align_y
        );

        return pos;
    }

    static TextPos calcCenteredText(
        u8g2_t* u8g2,
        const Rect& area,
        const char* utf8
    ) {
        return calcTextPos(
            u8g2,
            area,
            utf8,
            TextAlignX::Center,
            TextAlignY::Center
        );
    }
};

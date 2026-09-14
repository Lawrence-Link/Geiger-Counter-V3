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

enum class TextOverflowPlacement {
    PreserveAlignment,
    PinToLeadingEdge
};

class TextAlignHelper {
public:
    /**
     * @brief Calculates a horizontal offset for text within an area.
     * @param area_width Width of the available area.
     * @param text_width Width of the rendered text.
     * @param align Horizontal alignment to apply.
     * @param overflow Placement policy when the text is wider than the area.
     * @return Offset from the area's left edge; it may be negative when
     *         preserving centered or right alignment for overflowing text.
     */
    static int32_t calcAlignedOffset(
        int32_t area_width,
        int32_t text_width,
        TextAlignX align,
        TextOverflowPlacement overflow =
            TextOverflowPlacement::PreserveAlignment
    ) {
        if (overflow == TextOverflowPlacement::PinToLeadingEdge &&
            text_width > area_width) {
            return 0;
        }

        switch (align) {
        case TextAlignX::Left:
            return 0;
        case TextAlignX::Center:
            return (area_width - text_width) / 2;
        case TextAlignX::Right:
            return area_width - text_width;
        default:
            return 0;
        }
    }

    /**
     * @brief Calculates the left coordinate for horizontally centered text.
     * @param u8g2 Graphics context used to measure the UTF-8 text.
     * @param area_x Left coordinate of the available area.
     * @param area_w Width of the available area.
     * @param utf8 Null-terminated UTF-8 text to measure.
     * @return Left coordinate at which the text should be drawn.
     */
    static int16_t calcCenteredX(
        u8g2_t* u8g2,
        int16_t area_x,
        int16_t area_w,
        const char* utf8
    ) {
        int16_t text_width = u8g2_GetUTF8Width(u8g2, utf8);
        return static_cast<int16_t>(
            area_x + calcAlignedOffset(
                area_w, text_width, TextAlignX::Center));
    }

    /**
     * @brief Calculates the left coordinate for horizontally aligned text.
     * @param u8g2 Graphics context used to measure the UTF-8 text.
     * @param area_x Left coordinate of the available area.
     * @param area_w Width of the available area.
     * @param utf8 Null-terminated UTF-8 text to measure.
     * @param align Horizontal alignment to apply.
     * @return Left coordinate at which the text should be drawn.
     */
    static int16_t calcAlignedX(
        u8g2_t* u8g2,
        int16_t area_x,
        int16_t area_w,
        const char* utf8,
        TextAlignX align
    ) {
        int16_t text_width = u8g2_GetUTF8Width(u8g2, utf8);

        return static_cast<int16_t>(
            area_x + calcAlignedOffset(area_w, text_width, align));
    }

    /**
     * @brief Calculates a text baseline for the requested vertical alignment.
     * @param u8g2 Graphics context that supplies the current font metrics.
     * @param area_y Top coordinate of the available area, or the baseline when
     *        @p align is TextAlignY::Baseline.
     * @param area_h Height of the available area.
     * @param align Vertical alignment to apply.
     * @return Baseline coordinate at which the text should be drawn.
     */
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

    /**
     * @brief Calculates the draw position for text aligned inside a rectangle.
     * @param u8g2 Graphics context used for text metrics.
     * @param area Rectangle in which to align the text.
     * @param utf8 Null-terminated UTF-8 text to measure.
     * @param align_x Horizontal alignment to apply.
     * @param align_y Vertical alignment to apply.
     * @return Text origin containing the left coordinate and baseline.
     */
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

    /**
     * @brief Calculates the draw position for text centered in a rectangle.
     * @param u8g2 Graphics context used for text metrics.
     * @param area Rectangle in which to center the text.
     * @param utf8 Null-terminated UTF-8 text to measure.
     * @return Text origin containing the left coordinate and baseline.
     */
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

/*
 * Copyright (C) 2025 Lawrence Link
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the conditions in LICENSE are met.
 */

#pragma once

#include "U8g2lib.h"
#include "core/CommonTypes.h"
#include <stdint.h>

/**
 * @brief Viewport state for a canvas that can be taller than the display.
 *
 * Coordinates stored by applications are canvas coordinates. The camera
 * offset is subtracted only by Canvas draw calls; direct U8G2 drawing remains
 * in fixed screen coordinates.
 */
class CanvasCamera {
public:
    /** @brief Creates a camera for the given viewport dimensions. */
    CanvasCamera(int32_t viewportWidth, int32_t viewportHeight)
        : viewportWidth_(viewportWidth), viewportHeight_(viewportHeight) {}

    /** @param enabled Whether camera offsets affect Canvas draw calls. */
    void setEnabled(bool enabled);
    /** @return Whether camera offsets are currently applied. */
    bool isEnabled() const { return enabled_; }

    /** @return Horizontal camera offset. */
    int32_t x() const { return x_; }
    /** @return Vertical offset, or zero while the camera is disabled. */
    int32_t y() const { return enabled_ ? y_ : 0; }
    /** @param x New horizontal camera offset. */
    void setX(int32_t x) { x_ = x; }
    /** @return Stored vertical offset even when the camera is disabled. */
    int32_t storedY() const { return y_; }

    /** @param y Requested vertical offset, clamped to the content extent. */
    bool setY(int32_t y);
    /** @param deltaY Relative vertical scroll amount. @return Whether the offset changed. */
    bool scrollBy(int32_t deltaY);
    /** @brief Scrolls enough to bring an inclusive content interval into view. */
    bool ensureVisible(int32_t top, int32_t bottom);
    /** @return Whether an UP or DOWN navigation event changed the camera. */
    bool handleInput(InputEvent event);

    /** @param height Total content height used to clamp vertical scrolling. */
    void setContentHeight(int32_t height);
    /** @return Total content height used by the camera. */
    int32_t contentHeight() const { return contentHeight_; }
    /** @return Largest valid vertical camera offset. */
    int32_t maxY() const;
    /** @return Camera viewport width in pixels. */
    int32_t viewportWidth() const { return viewportWidth_; }
    /** @return Camera viewport height in pixels. */
    int32_t viewportHeight() const { return viewportHeight_; }

private:
    friend class Canvas;

    /** @brief Updates the physical viewport after display initialization. */
    void setViewportSize(int32_t width, int32_t height);
    /** @return Vertical offset clamped to the valid content extent. */
    int32_t clampY(int32_t y) const;

    int32_t viewportWidth_ = 0;
    int32_t viewportHeight_ = 0;
    int32_t x_ = 0;
    int32_t y_ = 0;
    int32_t contentHeight_ = 0;
    bool enabled_ = false;
};

/**
 * @brief Camera-aware drawing facade for the coordinate-bearing U8G2 APIs.
 *
 * Draw on this object when content should move with the application camera.
 * Use PixelUI::getU8G2() for fixed overlays and low-level display operations.
 */
class Canvas {
public:
    /** @brief Creates a camera-aware drawing facade over a U8G2 display. */
    Canvas(U8G2& display, int32_t displayWidth, int32_t displayHeight)
        : display_(display), camera_(displayWidth, displayHeight),
          displayWidth_(displayWidth), displayHeight_(displayHeight) {}

    /** @return Mutable camera controlling Canvas coordinate translation. */
    CanvasCamera& camera() { return camera_; }
    /** @return Read-only camera controlling Canvas coordinate translation. */
    const CanvasCamera& camera() const { return camera_; }
    /** @return Underlying display for fixed screen-coordinate operations. */
    U8G2& rawDisplay() { return display_; }

    /** @brief Begins a frame and resets per-frame observed content state. */
    void beginFrame();
    /** @brief Ends a frame and reports whether content height changed. */
    bool endFrame();
    /** @param height Declared content height in canvas coordinates. */
    void setContentHeight(int32_t height);

    /** @return Physical display width in pixels. */
    int32_t getDisplayWidth() const { return displayWidth_; }
    /** @return Physical display height in pixels. */
    int32_t getDisplayHeight() const { return displayHeight_; }
    /** @return Physical display width in pixels. */
    int32_t getWidth() const { return displayWidth_; }

    /** @param color U8G2 draw color or operation mode. */
    void setDrawColor(uint8_t color) { display_.setDrawColor(color); }
    /** @return Current U8G2 draw color or operation mode. */
    uint8_t getDrawColor() { return display_.getDrawColor(); }
    /** @param transparent U8G2 bitmap transparency mode. */
    void setBitmapMode(uint8_t transparent) { display_.setBitmapMode(transparent); }

    /** @param font U8G2 font data. */
    void setFont(const uint8_t* font) { display_.setFont(font); }
    /** @param transparent U8G2 font transparency mode. */
    void setFontMode(uint8_t transparent) { display_.setFontMode(transparent); }
    /** @param direction U8G2 font drawing direction. */
    void setFontDirection(uint8_t direction) { display_.setFontDirection(direction); }
    /** @brief Sets U8G2 text positioning relative to the baseline. */
    void setFontPosBaseline() { display_.setFontPosBaseline(); }
    /** @brief Sets U8G2 text positioning relative to the bottom edge. */
    void setFontPosBottom() { display_.setFontPosBottom(); }
    /** @brief Sets U8G2 text positioning relative to the top edge. */
    void setFontPosTop() { display_.setFontPosTop(); }
    /** @brief Sets U8G2 text positioning relative to the center line. */
    void setFontPosCenter() { display_.setFontPosCenter(); }
    /** @brief Selects U8G2 text reference height metrics. */
    void setFontRefHeightText() { display_.setFontRefHeightText(); }
    /** @brief Selects U8G2 extended-text reference height metrics. */
    void setFontRefHeightExtendedText() { display_.setFontRefHeightExtendedText(); }
    /** @brief Selects U8G2 full-font reference height metrics. */
    void setFontRefHeightAll() { display_.setFontRefHeightAll(); }
    /** @return Current font ascent in pixels. */
    int8_t getAscent() { return display_.getAscent(); }
    /** @return Current font descent in pixels. */
    int8_t getDescent() { return display_.getDescent(); }
    /** @return Current font ascent according to U8G2 metrics. */
    int8_t getFontAscent() { return display_.getFontAscent(); }
    /** @return Current font descent according to U8G2 metrics. */
    int8_t getFontDescent() { return display_.getFontDescent(); }
    /** @return Width of an ASCII string in the current font. */
    u8g2_uint_t getStrWidth(const char* text) { return display_.getStrWidth(text); }
    /** @return Width of a UTF-8 string in the current font. */
    u8g2_uint_t getUTF8Width(const char* text) { return display_.getUTF8Width(text); }
    /** @return Horizontal offset of a glyph in the current font. */
    int8_t getXOffsetGlyph(uint16_t encoding) { return display_.getXOffsetGlyph(encoding); }
    /** @return Horizontal offset of the first UTF-8 glyph. */
    int8_t getXOffsetUTF8(const char* text) { return display_.getXOffsetUTF8(text); }

    /** @brief Draws one camera-translated pixel. */
    void drawPixel(int32_t x, int32_t y);
    /** @brief Draws a horizontal camera-translated line. */
    void drawHLine(int32_t x, int32_t y, int32_t width);
    /** @brief Draws a vertical camera-translated line. */
    void drawVLine(int32_t x, int32_t y, int32_t height);
    /** @brief Draws a horizontal or vertical camera-translated line. */
    void drawHVLine(int32_t x, int32_t y, int32_t length, uint8_t direction);
    /** @brief Draws a camera-translated line between two points. */
    void drawLine(int32_t x1, int32_t y1, int32_t x2, int32_t y2);
    /** @brief Draws a camera-translated rectangular frame. */
    void drawFrame(int32_t x, int32_t y, int32_t width, int32_t height);
    /** @brief Draws a camera-translated rounded rectangular frame. */
    void drawRFrame(int32_t x, int32_t y, int32_t width, int32_t height, int32_t radius);
    /** @brief Draws a camera-translated filled rectangle. */
    void drawBox(int32_t x, int32_t y, int32_t width, int32_t height);
    /** @brief Draws a camera-translated filled rounded rectangle. */
    void drawRBox(int32_t x, int32_t y, int32_t width, int32_t height, int32_t radius);
    /** @brief Draws a camera-translated circle. */
    void drawCircle(int32_t x, int32_t y, int32_t radius, uint8_t option = U8G2_DRAW_ALL);
    /** @brief Draws a camera-translated filled circle. */
    void drawDisc(int32_t x, int32_t y, int32_t radius, uint8_t option = U8G2_DRAW_ALL);
    /** @brief Draws a camera-translated ellipse. */
    void drawEllipse(int32_t x, int32_t y, int32_t rx, int32_t ry, uint8_t option = U8G2_DRAW_ALL);
    /** @brief Draws a camera-translated filled ellipse. */
    void drawFilledEllipse(int32_t x, int32_t y, int32_t rx, int32_t ry, uint8_t option = U8G2_DRAW_ALL);
    /** @brief Draws a camera-translated circular arc. */
    void drawArc(int32_t x, int32_t y, int32_t radius, uint8_t start, uint8_t end);
    /** @brief Draws a camera-translated triangle. */
    void drawTriangle(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2);
    /** @brief Draws a camera-translated bitmap. */
    void drawBitmap(int32_t x, int32_t y, int32_t byteWidth, int32_t height, const uint8_t* bitmap);
    /** @brief Draws a camera-translated XBM bitmap. */
    void drawXBM(int32_t x, int32_t y, int32_t width, int32_t height, const uint8_t* bitmap);
    /** @brief Draws a camera-translated XBMP bitmap. */
    void drawXBMP(int32_t x, int32_t y, int32_t width, int32_t height, const uint8_t* bitmap);
    /** @brief Draws a camera-translated glyph. */
    u8g2_uint_t drawGlyph(int32_t x, int32_t y, uint16_t encoding);
    /** @brief Draws a camera-translated doubled glyph. */
    u8g2_uint_t drawGlyphX2(int32_t x, int32_t y, uint16_t encoding);
    /** @brief Draws a camera-translated ASCII string. */
    u8g2_uint_t drawStr(int32_t x, int32_t y, const char* text);
    /** @brief Draws a camera-translated doubled ASCII string. */
    u8g2_uint_t drawStrX2(int32_t x, int32_t y, const char* text);
    /** @brief Draws a camera-translated UTF-8 string. */
    u8g2_uint_t drawUTF8(int32_t x, int32_t y, const char* text);
    /** @brief Draws a camera-translated doubled UTF-8 string. */
    u8g2_uint_t drawUTF8X2(int32_t x, int32_t y, const char* text);
    /** @brief Draws a camera-translated UTF-8 string with external kerning. */
    u8g2_uint_t drawExtUTF8(int32_t x, int32_t y, uint8_t toLeft,
                           const uint16_t* kerningTable, const char* text);
    /** @brief Draws a camera-translated UTF-8 button label. */
    void drawButtonUTF8(int32_t x, int32_t y, int32_t flags, int32_t width,
                        int32_t horizontalPadding, int32_t verticalPadding,
                        const char* text);

#ifdef U8G2_WITH_CLIP_WINDOW_SUPPORT
    /** @brief Sets a camera-translated clipping rectangle. */
    void setClipWindow(int32_t x0, int32_t y0, int32_t x1, int32_t y1);
    /** @brief Restores the full display clipping rectangle. */
    void setMaxClipWindow() { display_.setMaxClipWindow(); }
#endif

private:
    friend class PixelUI;

    /** @brief Updates display-dependent dimensions after U8G2 initialization. */
    void setDisplaySize(int32_t width, int32_t height);
    /** @return Screen X coordinate after camera translation. */
    u8g2_uint_t screenX(int32_t x) const;
    /** @return Screen Y coordinate after camera translation. */
    u8g2_uint_t screenY(int32_t y) const;
    /** @brief Records the lowest content coordinate observed this frame. */
    void observeBottom(int32_t bottom);
    /** @brief Records a rectangle's contribution to observed content height. */
    void observeRect(int32_t y, int32_t height);
    /** @brief Records text extents using the current font metrics. */
    void observeText(int32_t baseline, bool doubled = false);

    U8G2& display_;
    CanvasCamera camera_;
    int32_t displayWidth_ = 0;
    int32_t displayHeight_ = 0;
    int32_t observedHeight_ = 0;
    int32_t declaredHeight_ = 0;
};

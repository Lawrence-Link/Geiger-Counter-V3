/*
 * Copyright (C) 2025 Lawrence Link
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include "../IWidget.h"

class TextButton : public IWidget {
private:
    VoidCallback m_callback;          // Callback when selected

    PixelUI& m_ui;

    int32_t m_x = 0, m_y = 0;                  // Position of the button
    int32_t m_w = 0, m_h = 0;                  // Size of the button                       // Reference to the UI manager

    const char* src = nullptr;                 // Button text
    const uint8_t* m_font = PIXELUI_FONT_TEXT;
    // Animation values
    int32_t anim_w = 0, anim_h = 0;           // Animated width/height
    int32_t anim_x = 0, anim_y = 0;           // Animated position

/** @brief drawSelf. */
    void drawSelf(const WidgetRenderContext& context) override;
/** @brief display. */
    Canvas& display() override;

public:
/** @brief TextButton. */
    TextButton(PixelUI& ui, uint16_t x, uint16_t y, uint16_t w, uint16_t h, char const* text = "")
        : TextButton(ui, x, y, w, h, text, PIXELUI_FONT_TEXT)
    {
    }
/** @brief TextButton. */
    TextButton(PixelUI& ui, uint16_t x, uint16_t y, uint16_t w, uint16_t h,
               char const* text, const uint8_t* font)
        : m_ui(ui), m_x(x), m_y(y), m_w(w), m_h(h), src(text), m_font(font)
    {
        setFocusable(true);
        setFocusInsets({1, 1, 1, 1});
        setWidgetBounds({m_x, m_y, m_w, m_h});
    }
/** @brief ~TextButton. */
    ~TextButton() override = default;

    // IWidget overrides
/** @brief onLoad. */
    void onLoad() override;
/** @brief onLoad. */
    void onLoad(LoadTransition transition);
    // Compatibility wrapper for callers that already use the old name.
/** @brief onLoadNoAnim. */
    void onLoadNoAnim();
/** @brief onOffload. */
    void onOffload() override;
/** @brief onSelect. */
    bool onSelect() override;

    // Setters
/** @brief setCallback. */
    void setCallback(VoidCallback cb) { m_callback = cb; }
/** @brief setPosition. */
    void setPosition(uint16_t x, uint16_t y) {
        m_x = x; m_y = y; 
        setWidgetBounds({m_x, m_y, m_w, m_h});
    }
/** @brief setSize. */
    void setSize(uint16_t w, uint16_t h) {
        m_w = w; m_h = h; 
        setWidgetBounds({m_x, m_y, m_w, m_h});
    }
/** @brief setText. */
    void setText(const char* text) { src = text; }
};

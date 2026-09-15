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

#include "widgets/text_button/text_button.h"
#include "PixelUI.h"

/**
 * @brief Initialize the widget with animation from the center.
 */
void TextButton::onLoad() {
    onLoad(LoadTransition::Animated);
}

/** @brief TextButton::onLoad. */
void TextButton::onLoad(LoadTransition transition) {
    // Calculate the center coordinates
    int32_t center_x = m_x + m_w / 2;
    int32_t center_y = m_y + m_h / 2;

    anim_x = center_x;
    anim_y = center_y;

    if (transition == LoadTransition::Immediate) {
        anim_w = m_w;
        anim_h = m_h;
        m_ui.markDirty();
        return;
    }

    // Animate width and height expansion from the center
    anim_w = 0;
    anim_h = 0;
    if (!m_ui.animate(anim_w, anim_h,
                      m_w, m_h,
                      400, EasingType::EASE_OUT_CUBIC,
                      PROTECTION::PROTECTED)) {
        anim_w = m_w;
        anim_h = m_h;
        m_ui.markDirty();
    }
}

/** @brief TextButton::onLoadNoAnim. */
void TextButton::onLoadNoAnim() {
    onLoad(LoadTransition::Immediate);
}

/**
 * @brief Clean up widget resources when offloaded.
 */
void TextButton::onOffload() { }

/**
 * @brief Render the text button.
 */
Canvas& TextButton::display() { return m_ui.getCanvas(); }

/** @brief TextButton::drawSelf. */
void TextButton::drawSelf(const WidgetRenderContext& context) {
    if (!src) return;

    Canvas& u8g2 = m_ui.getCanvas();

    // Compute animated drawing position (centered)
    int32_t draw_x = context.originX + m_x + (m_w - anim_w) / 2;
    int32_t draw_y = context.originY + m_y + (m_h - anim_h) / 2;

    // Clip drawing area to widget bounds
    setClipWindow(context, {draw_x - context.originX, draw_y - context.originY, anim_w, anim_h});

    // Draw rounded frame with radius 2
    u8g2.drawRFrame(draw_x, draw_y, anim_w, anim_h, 2);

    u8g2.setFont(m_font);

    // Get font metrics
    int8_t font_ascent = u8g2.getAscent();   // Height above baseline
    int8_t font_descent = u8g2.getDescent(); // Depth below baseline
    int8_t font_height = font_ascent - font_descent;

    // Calculate text width
    int32_t text_width = u8g2.getUTF8Width((const char*)src);

    // Compute centered text position
    int32_t text_x = draw_x + (anim_w - text_width) / 2;
    int32_t text_y = draw_y + (anim_h - font_height) / 2 + font_ascent;

    // Draw the button text
    u8g2.drawUTF8(text_x + m_textOffsetX,
                  text_y + 2 + m_textOffsetY,
                  (const char*)src);

    // Reset clip window
    restoreClipWindow(context);
}

/**
 * @brief Handle selection of the button.
 * @return False, but callback executed if set.
 */
bool TextButton::onSelect() {
    if (m_callback) {
        m_callback();
    }
    return false;
}

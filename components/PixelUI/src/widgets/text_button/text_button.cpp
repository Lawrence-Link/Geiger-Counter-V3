/*
 * Copyright (C) 2025 Lawrence Link
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "widgets/text_button/text_button.h"

/**
 * @brief Constructor for TextButton widget.
 * @param ui Reference to PixelUI instance for rendering and animation.
 */
TextButton::TextButton(PixelUI& ui, uint16_t x, uint16_t y, uint16_t w, uint16_t h) : 
    m_ui(ui),
    m_x(x),
    m_y(y),
    m_w(w),
    m_h(h)
{

}

/**
 * @brief Initialize the widget with animation from the center.
 */
void TextButton::onLoad() {
    // Calculate the center coordinates
    int32_t center_x = m_x + m_w / 2;
    int32_t center_y = m_y + m_h / 2;

    anim_x = center_x;
    anim_y = center_y;

    // Set focus box slightly inside the widget
    setFocusBox(FocusBox(m_x + 1, m_y + 1, m_w - 2, m_h - 2));

    // Animate width and height expansion from the center
    m_ui.animate(anim_w, anim_h,
                 m_w, m_h,
                 400, EasingType::EASE_OUT_CUBIC,
                 PROTECTION::PROTECTED);
}

/**
 * @brief Clean up widget resources when offloaded.
 */
void TextButton::onOffload() { }

/**
 * @brief Render the text button.
 */
void TextButton::draw() {
    if (!src) return;

    U8G2& u8g2 = m_ui.getU8G2();

    // Compute animated drawing position (centered)
    int32_t draw_x = m_x + (m_w - anim_w) / 2;
    int32_t draw_y = m_y + (m_h - anim_h) / 2;

    // Clip drawing area to widget bounds
    u8g2.setClipWindow(draw_x, draw_y, draw_x + anim_w, draw_y + anim_h);

    // Draw rounded frame with radius 2
    u8g2.drawRFrame(draw_x, draw_y, anim_w, anim_h, 2);

    u8g2.setFont(u8g2_font_wqy12_t_gb2312);

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
    u8g2.drawUTF8(text_x, text_y + 2, (const char*)src);

    // Reset clip window
    u8g2.setMaxClipWindow();
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

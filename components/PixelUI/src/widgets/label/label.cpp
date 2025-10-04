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

#include "widgets/label/label.h"

Label::Label(PixelUI& ui, uint16_t x, uint16_t y, const char* content, POS pos) : m_ui(ui), m_x(x), m_y(y), src(content), load_pos(pos) 
{
    
}

void Label::onLoad() {
    // calculate center coordinate.
    U8G2& u8g2 = m_ui.getU8G2();
    u8g2.setFont(u8g2_font_wqy12_t_gb2312);
    int8_t font_height = u8g2.getFontAscent() + u8g2.getFontDescent();
    switch (load_pos) {
        case POS::TOP:    {
            anim_x = m_x;
            anim_y = m_y - font_height; 
            m_ui.animate(anim_y, m_y, 300, EasingType::EASE_OUT_CUBIC, PROTECTION::NOT_PROTECTED);
        } break;
        case POS::BOTTOM: {
            anim_x = m_x;
            anim_y = m_y + font_height; 
            m_ui.animate(anim_y, m_y, 300, EasingType::EASE_OUT_CUBIC, PROTECTION::NOT_PROTECTED);
        } break;
        case POS::LEFT:   {
            anim_x = m_x - u8g2.getUTF8Width(src);
            anim_y = m_y; 
            m_ui.animate(anim_y, m_y, 300, EasingType::EASE_OUT_CUBIC, PROTECTION::NOT_PROTECTED);
        } break;
        case POS::RIGHT:  {
            anim_x = m_x + u8g2.getUTF8Width(src);
            anim_y = m_y; 
            m_ui.animate(anim_y, m_y, 300, EasingType::EASE_OUT_CUBIC, PROTECTION::NOT_PROTECTED);
        } break;
    }

    setFocusBox(FocusBox(m_x + 1, m_y + 1, m_w - 2, m_h - 2));
}

void Label::onOffload() {}

void Label::draw() {
    if (!src) return;
    U8G2& u8g2 = m_ui.getU8G2();

    u8g2.setFont(u8g2_font_wqy12_t_gb2312);    
    // Height data of the font
    int8_t font_ascent = u8g2.getAscent();   // Parts above the baseline
    int8_t font_descent = u8g2.getDescent(); // Parts below the baseline
    int8_t font_height = font_ascent - font_descent; // Total height
    int32_t text_width = u8g2.getUTF8Width((const char*)src);

    u8g2.setClipWindow(m_x, m_y - font_height, m_x+text_width, m_y + 1);
    
    u8g2.drawUTF8(anim_x, anim_y, (const char*)src);
    
    u8g2.setMaxClipWindow();
}

bool Label::onSelect() {
    if (m_callback) {
        m_callback();
    }
    return false;
}
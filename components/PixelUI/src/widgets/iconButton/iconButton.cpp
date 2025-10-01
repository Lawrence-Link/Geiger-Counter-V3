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

#include "widgets/iconButton/iconButton.h"

IconButton::IconButton(PixelUI& ui) : m_ui(ui) {

}

void IconButton::onLoad() {
    anim_x = m_x;
    anim_y = m_y - 10;
    m_ui.animate(anim_y, m_y, 100, EasingType::EASE_OUT_CUBIC, PROTECTION::PROTECTED);
}

void IconButton::onOffload() {}

void IconButton::draw() {
    if (!src) {
        return;
    }
    U8G2& u8g2 = m_ui.getU8G2();
    u8g2.drawXBM(anim_x, anim_y, m_w, m_h, src);
}

bool IconButton::onSelect() {
    if (m_callback) {
        m_callback();
    }
    return false;
}
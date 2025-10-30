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

#include "widgets/icon_button/icon_button.h"

/**
 * @brief Constructor for IconButton widget.
 * @param ui Reference to the PixelUI instance for rendering and animation.
 */
IconButton::IconButton(PixelUI& ui, uint16_t x, uint16_t y, uint16_t w, uint16_t h) : 
    m_ui(ui),
    pos_x(x),
    pos_y(y),
    m_w(w),
    m_h(h)
{
    
}

/**
 * @brief Initialize the widget when loaded. Sets initial animation offsets.
 */
void IconButton::onLoad() {
    // Set initial animated position slightly above the target y
    anim_x = pos_x;
    anim_y = pos_y - 10;

    // Animate Y position to slide down into final position
    m_ui.animate(anim_y, pos_y, 100, EasingType::EASE_OUT_CUBIC, PROTECTION::PROTECTED);
}

/**
 * @brief Clean up resources when widget is offloaded.
 */
void IconButton::onOffload() {
    // No dynamic resources allocated, nothing to clean
}

/**
 * @brief Render the icon button on the screen.
 */
void IconButton::draw() {
    if (!src) {
        return; // No image source, nothing to draw
    }

    U8G2& u8g2 = m_ui.getU8G2();

    // Draw XBM bitmap at current animated position
    u8g2.drawXBM(anim_x, anim_y, m_w, m_h, src);
}

/**
 * @brief Handle button selection (click).
 * @return False, indicating event propagation is not stopped.
 */
bool IconButton::onSelect() {
    // Trigger user-defined callback if available
    if (m_callback) {
        m_callback();
    }
    return false;
}

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
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "widgets/brace/brace.h"

/**
 * @brief Construct a new Brace object
 * 
 * @param ui Reference to the PixelUI instance
 */
Brace::Brace(PixelUI& ui) : m_ui(ui) {
    onLoad(); /**< Start the loading animations */
}

/**
 * @brief Initialize animations for brace expansion
 */
void Brace::onLoad() {
    /**< Animate width from 0 to margin_w_ over 550ms */
    m_ui.animate(anim_w, margin_w_, 550, EasingType::EASE_OUT_CUBIC, PROTECTION::PROTECTED);

    /**< Animate height from 0 to margin_h_ over 600ms */
    m_ui.animate(anim_h, margin_h_, 600, EasingType::EASE_OUT_CUBIC, PROTECTION::PROTECTED);

    FocusBox fbox;
    fbox.x = coord_x_ - margin_w_ / 2 + 1;
    fbox.y = coord_y_ - margin_h_ / 2 + 1;
    fbox.w = margin_w_ - 1;
    fbox.h = margin_h_ - 1;
    setFocusBox(fbox);
}

/**
 * @brief Called when the widget is removed/offloaded
 */
void Brace::onOffload() {
    // No special cleanup required
}

/**
 * @brief Draw the brace with corners and optional content inside
 */
void Brace::draw() {
    U8G2& u8g2 = m_ui.getU8G2();

    int half_width = anim_w / 2;
    int half_height = anim_h / 2;

    int clip_x = coord_x_ - half_width;
    int clip_y = coord_y_ - half_height;
    int clip_width = anim_w;
    int clip_height = anim_h;

    /**< Set the clipping window to restrict drawing inside the brace */
    u8g2.setClipWindow(
        clip_x, 
        clip_y, 
        clip_x + clip_width,   /**< bottom-right X */
        clip_y + clip_height   /**< bottom-right Y */
    );

    /**< Draw the user-provided content inside the brace, if any */
    if (contentWithinBrace) {
        contentWithinBrace();
    }

    /**< Reset clipping to full screen */
    u8g2.setMaxClipWindow();

    /**< Draw corner lines for the brace */
    // Top-left corner
    u8g2.drawLine(coord_x_ - half_width, coord_y_ - half_height, coord_x_ - half_width + 4, coord_y_ - half_height);
    u8g2.drawLine(coord_x_ - half_width, coord_y_ - half_height, coord_x_ - half_width, coord_y_ - half_height + 4);

    // Top-right corner
    u8g2.drawLine(coord_x_ + half_width, coord_y_ - half_height, coord_x_ + half_width - 4, coord_y_ - half_height);
    u8g2.drawLine(coord_x_ + half_width, coord_y_ - half_height, coord_x_ + half_width, coord_y_ - half_height + 4);

    // Bottom-left corner
    u8g2.drawLine(coord_x_ - half_width, coord_y_ + half_height, coord_x_ - half_width + 4, coord_y_ + half_height);
    u8g2.drawLine(coord_x_ - half_width, coord_y_ + half_height, coord_x_ - half_width, coord_y_ + half_height - 4);

    // Bottom-right corner
    u8g2.drawLine(coord_x_ + half_width, coord_y_ + half_height, coord_x_ + half_width - 4, coord_y_ + half_height);
    u8g2.drawLine(coord_x_ + half_width, coord_y_ + half_height, coord_x_ + half_width, coord_y_ + half_height - 4);
}

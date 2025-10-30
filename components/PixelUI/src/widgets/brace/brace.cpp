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
 * * @param ui Reference to the PixelUI instance
 * * Note: pos_x_ and pos_y_ now represent the top-left corner.
 */
Brace::Brace(PixelUI& ui, uint16_t pos_x, uint16_t pos_y, uint16_t size_w, uint16_t size_h) : 
    m_ui(ui), 
    pos_x_(pos_x),
    pos_y_(pos_y),
    size_w_(size_w),
    size_h_(size_h) 
{
    int32_t start_anim_x = (size_w_ / 2);
    int32_t start_anim_y = (size_h_ / 2);
    
    anim_w = 0; 
    anim_h = 0;

    anim_x = start_anim_x;
    anim_y = start_anim_y;
}

/**
 * @brief Initialize animations for brace expansion and set the focus box
 */
void Brace::onLoad() {
    int32_t start_anim_x = (size_w_ / 2);
    int32_t start_anim_y = (size_h_ / 2);
    
    anim_w = 0; 
    anim_h = 0;
    
    anim_x = start_anim_x;
    anim_y = start_anim_y;

    m_ui.animate(anim_w, size_w_, 550, EasingType::EASE_OUT_CUBIC, PROTECTION::PROTECTED);
    m_ui.animate(anim_h, size_h_, 600, EasingType::EASE_OUT_CUBIC, PROTECTION::PROTECTED);
    
    m_ui.animate(anim_x, 0, 550, EasingType::EASE_OUT_CUBIC, PROTECTION::PROTECTED);
    m_ui.animate(anim_y, 0, 600, EasingType::EASE_OUT_CUBIC, PROTECTION::PROTECTED);

    FocusBox fbox;
    fbox.x = pos_x_ + 1; // Top-left X + 1
    fbox.y = pos_y_ + 1; // Top-left Y + 1
    fbox.w = size_w_ - 1;
    fbox.h = size_h_ - 1;
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

    int tl_x = pos_x_ + anim_x;
    int tl_y = pos_y_ + anim_y;
    int current_w = anim_w;
    int current_h = anim_h;

    /**< Set the clipping window to restrict drawing inside the brace, based on top-left */
    u8g2.setClipWindow(
        tl_x, 
        tl_y, 
        tl_x + current_w,   /**< bottom-right X */
        tl_y + current_h    /**< bottom-right Y */
    );

    /**< Draw the user-provided content inside the brace, if any */
    if (contentWithinBrace) {
        contentWithinBrace();
    }

    /**< Reset clipping to full screen */
    u8g2.setMaxClipWindow();

    /**< Draw corner lines for the brace */
    
    // Top-left corner: (tl_x, tl_y)
    u8g2.drawLine(tl_x, tl_y, tl_x + 4, tl_y);
    u8g2.drawLine(tl_x, tl_y, tl_x, tl_y + 4);

    // Top-right corner: (tl_x + current_w, tl_y)
    u8g2.drawLine(tl_x + current_w, tl_y, tl_x + current_w - 4, tl_y);
    u8g2.drawLine(tl_x + current_w, tl_y, tl_x + current_w, tl_y + 4);

    // Bottom-left corner: (tl_x, tl_y + current_h)
    u8g2.drawLine(tl_x, tl_y + current_h, tl_x + 4, tl_y + current_h);
    u8g2.drawLine(tl_x, tl_y + current_h, tl_x, tl_y + current_h - 4);

    // Bottom-right corner: (tl_x + current_w, tl_y + current_h)
    u8g2.drawLine(tl_x + current_w, tl_y + current_h, tl_x + current_w - 4, tl_y + current_h);
    u8g2.drawLine(tl_x + current_w, tl_y + current_h, tl_x + current_w, tl_y + current_h - 4);
}
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

#pragma once

#include "../IWidget.h"

class TextButton : public IWidget {
private:
    const char* src = nullptr;                 // Button text
    std::function<void()> m_callback;          // Callback when selected

    PixelUI& m_ui;

    int32_t m_x = 0, m_y = 0;                  // Position of the button
    int32_t m_w = 0, m_h = 0;                  // Size of the button                       // Reference to the UI manager

    // Animation values
    int32_t anim_w = 0, anim_h = 0;           // Animated width/height
    int32_t anim_x = 0, anim_y = 0;           // Animated position

public:
    TextButton(PixelUI& ui, uint16_t x, uint16_t y, uint16_t w, uint16_t h);
    ~TextButton() override = default;

    // IWidget overrides
    void onLoad() override;
    void onOffload() override;
    bool onSelect() override;
    void draw() override;

    // Setters
    void setCallback(std::function<void()> cb) { m_callback = cb; }
    void setPosition(uint16_t x, uint16_t y) { m_x = x; m_y = y; }
    void setSize(uint16_t w, uint16_t h) { m_w = w; m_h = h; }
    void setText(const char* text) { src = text; }
};
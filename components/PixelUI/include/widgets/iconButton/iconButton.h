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

class IconButton : public IWidget {
private:
    const unsigned char* src;
    std::function<void()> m_callback;
    int32_t m_x = 0, m_y = 0, m_w = 0, m_h = 0;
    PixelUI& m_ui;

    int32_t anim_x = 0, anim_y = 0;
public:
    IconButton(PixelUI& ui);
    ~IconButton() = default;
    void onLoad() override;
    void onOffload() override;
    bool onSelect() override ;
    void draw() override;

    void setCallback(std::function<void()> cb) {m_callback = cb;}
    void setCoordinate(uint16_t x, uint16_t y) {m_x = x; m_y = y;};
    void setMargin(uint16_t w, uint16_t h) {m_w = w; m_h = h;}
    void setSource(const unsigned char* source) {src = source;};
};
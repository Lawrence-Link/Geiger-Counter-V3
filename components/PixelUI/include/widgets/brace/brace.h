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

#pragma once

#include "../IWidget.h"

class Brace : public IWidget {

public:
    Brace(PixelUI& ui, uint16_t pos_x, uint16_t pos_y, uint16_t size_w, uint16_t size_h);
    ~Brace() = default;

    void onLoad() override;
    void onOffload() override;
    void draw() override;
    bool onSelect() override { if (m_callback) m_callback(); return false;}

    void setCallback(std::function<void()> cb) { m_callback = cb; };
    void setSize(uint16_t mar_w, uint16_t mar_h) {size_h_ = mar_w; size_h_ = mar_h;}
    void setPosition(int16_t pos_x, int16_t pos_y) {pos_x_ = pos_x; pos_y_=pos_y;}
    void setDrawContentFunction(std::function<void()> func) { contentWithinBrace = func; }
    
private:
    PixelUI& m_ui;
    int16_t pos_x_ = 0, pos_y_ = 0;
    int16_t size_w_ = 0, size_h_ = 0;

    std::function<void()> contentWithinBrace;
    std::function<void()> m_callback = nullptr;

    int32_t anim_w = 0;
    int32_t anim_h = 0;
    int32_t anim_x = 0;
    int32_t anim_y = 0;
};
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

enum class EXPAND_BASE {
    TOP_LEFT,
    TOP_RIGHT,
    BOTTOM_LEFT,
    BOTTOM_RIGHT
};

class Histogram : public IWidget {
public:
    Histogram(PixelUI& ui);
    ~Histogram() = default;

    void onLoad() override;
    void onOffload() override;
    void draw() override;
    bool onSelect() override;
    bool handleEvent(InputEvent event) override;

    void setSize(uint16_t mar_w, uint16_t mar_h) { margin_w_ = mar_w; margin_h_ = mar_h; }
    void setCoordinate(uint16_t coord_x, uint16_t coord_y) { coord_x_ = coord_x; coord_y_ = coord_y; }
    void setExpand(EXPAND_BASE base, uint16_t w, uint16_t h) {base_ = base; exp_w = w; exp_h = h;}

    bool isExpanded() const { return is_expanded; }

    void addData(float value);
    float getMaxValue() const;
    float getAverageValue() const;
    float getMinValue() const;
    void clearData();

private:
    uint16_t coord_x_ = 0, coord_y_ = 0;
    uint16_t margin_w_ = 0, margin_h_ = 0;
    uint16_t exp_w = 0, exp_h = 0;
    EXPAND_BASE base_;

    // Internal data buffer for real-time data streaming
    float* m_data_buffer = nullptr;
    int m_buffer_size = 0;
    int m_write_index = 0;
    int m_data_count = 0;
    
    // Statistics tracking
    float m_max_value = 0.0f;
    float m_min_value = 0.0f;
    float m_sum_value = 0.0f;

    PixelUI& m_ui;

    float* m_data_ptr = nullptr;     // Pointer to the external circular buffer
    uint16_t m_data_size = 0;        // Total size of the circular buffer
    uint16_t m_head_index = 0;       // Current head index of the circular buffer

    // animation related variables:
    int32_t anim_w = 0;
    int32_t anim_h = 0;
    int32_t anim_x = 0;
    int32_t anim_y = 0;
    void expandWidget();
    void contractWidget();
    void calculateExpandPosition(int32_t& target_x, int32_t& target_y);
    void initializeDataBuffer();
    void updateStatistics(float new_value, float old_value, bool replacing_data);
    void recalculateExtremes();
    void drawHistogramData(int center_x, int center_y, int half_width, int half_height, U8G2& u8g2);

    bool is_expanded = false;
};
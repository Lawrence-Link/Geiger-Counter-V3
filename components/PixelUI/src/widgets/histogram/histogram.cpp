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
#include "widgets/histogram/histogram.h"
#include <cmath>
#include <algorithm>

Histogram::Histogram(PixelUI& ui) : m_ui(ui) {
    onLoad();
}

void Histogram::onLoad() {
    // Initialize animation coordinates to 0 (relative to original position)
    anim_x = 0;
    anim_y = 0;
    
    // Initialize size animation
    m_ui.animate(anim_w, margin_w_, 550, EasingType::EASE_OUT_QUAD, PROTECTION::PROTECTED);
    m_ui.animate(anim_h, margin_h_, 600, EasingType::EASE_OUT_QUAD, PROTECTION::PROTECTED);
    
    // Initialize internal data structures
    initializeDataBuffer();
}

void Histogram::initializeDataBuffer() {
    // Initialize buffer with expanded width size
    m_buffer_size = exp_w > 0 ? exp_w : 200; // Default to 200 if exp_w not set
    m_data_buffer = new float[m_buffer_size];
    
    // Initialize buffer with zeros
    for (int i = 0; i < m_buffer_size; ++i) {
        m_data_buffer[i] = 0.0f;
    }
    
    // Initialize statistics
    m_write_index = 0;
    m_data_count = 0;
    m_max_value = 0.0f;
    m_sum_value = 0.0f;
    m_min_value = std::numeric_limits<float>::max();
}

void Histogram::onOffload() {
    // Clean up buffer
    if (m_data_buffer != nullptr) {
        delete[] m_data_buffer;
        m_data_buffer = nullptr;
    }
}

void Histogram::addData(float value) {
    if (m_data_buffer == nullptr) {
        initializeDataBuffer();
    }
    
    // Store the old value at current position for statistics update
    float old_value = m_data_buffer[m_write_index];
    bool replacing_valid_data = (m_data_count >= m_buffer_size);
    
    // Add new value to buffer
    m_data_buffer[m_write_index] = value;
    
    // Update write index (ring buffer)
    m_write_index = (m_write_index + 1) % m_buffer_size;
    
    // Update data count
    if (m_data_count < m_buffer_size) {
        m_data_count++;
    }
    
    // Update statistics efficiently
    updateStatistics(value, old_value, replacing_valid_data);
}

void Histogram::updateStatistics(float new_value, float old_value, bool replacing_data) {
    if (!replacing_data) {
        // Adding new data (buffer not full yet)
        m_sum_value += new_value;
        m_max_value = std::max(m_max_value, new_value);
        m_min_value = std::min(m_min_value, new_value);
    } else {
        // Replacing old data (buffer is full)
        m_sum_value = m_sum_value - old_value + new_value;
        
        // For max/min, we need to check if we're removing the extreme value
        if (old_value == m_max_value || old_value == m_min_value) {
            // Need to recalculate max/min by scanning buffer
            recalculateExtremes();
        } else {
            // Just update with new value
            m_max_value = std::max(m_max_value, new_value);
            m_min_value = std::min(m_min_value, new_value);
        }
    }
}

void Histogram::recalculateExtremes() {
    if (m_data_count == 0) {
        m_max_value = 0.0f;
        m_min_value = 0.0f;
        return;
    }
    
    m_max_value = m_data_buffer[0];
    m_min_value = m_data_buffer[0];
    
    for (int i = 1; i < m_data_count; ++i) {
        m_max_value = std::max(m_max_value, m_data_buffer[i]);
        m_min_value = std::min(m_min_value, m_data_buffer[i]);
    }
}

float Histogram::getMaxValue() const {
    return m_max_value;
}

float Histogram::getAverageValue() const {
    if (m_data_count == 0) {
        return 0.0f;
    }
    return m_sum_value / m_data_count;
}

float Histogram::getMinValue() const {
    return m_min_value;
}

void Histogram::clearData() {
    if (m_data_buffer != nullptr) {
        for (int i = 0; i < m_buffer_size; ++i) {
            m_data_buffer[i] = 0.0f;
        }
    }
    
    m_write_index = 0;
    m_data_count = 0;
    m_max_value = 0.0f;
    m_sum_value = 0.0f;
    m_min_value = std::numeric_limits<float>::max();
}

bool Histogram::handleEvent(InputEvent event) {
    if (event == InputEvent::SELECT) {
        is_expanded = false;
        contractWidget();
        return true;
    }
    return false;
}

bool Histogram::onSelect(){
    m_ui.clearUnprotectedAnimations();
    if (!is_expanded) {
        // Expand animation
        expandWidget();
        is_expanded = true;
    } else {
        // Shrink animation
        contractWidget();
        is_expanded = false;
    }
    return true;
}

void Histogram::expandWidget() {
    int32_t target_x, target_y;
    calculateExpandPosition(target_x, target_y);
    
    // Animate to expanded size and position
    m_ui.animate(anim_w, exp_w, 400, EasingType::EASE_OUT_QUAD);
    m_ui.animate(anim_h, exp_h, 350, EasingType::EASE_OUT_QUAD);
    m_ui.animate(anim_x, target_x, 400, EasingType::EASE_OUT_QUAD);
    m_ui.animate(anim_y, target_y, 350, EasingType::EASE_OUT_QUAD);
}

void Histogram::contractWidget() {
    // Animate back to original size and position
    m_ui.animate(anim_w, margin_w_, 350, EasingType::EASE_OUT_QUAD, PROTECTION::PROTECTED);
    m_ui.animate(anim_h, margin_h_, 400, EasingType::EASE_OUT_QUAD, PROTECTION::PROTECTED);
    m_ui.animate(anim_x, 0, 350, EasingType::EASE_OUT_QUAD, PROTECTION::PROTECTED);
    m_ui.animate(anim_y, 0, 400, EasingType::EASE_OUT_QUAD, PROTECTION::PROTECTED);
}

void Histogram::calculateExpandPosition(int32_t& target_x, int32_t& target_y) {
    // Calculate the offset of the center position after expansion
    int32_t width_diff = exp_w - margin_w_;
    int32_t height_diff = exp_h - margin_h_;
    
    switch (base_) {
        case EXPAND_BASE::TOP_LEFT:
            target_x = width_diff / 2;
            target_y = height_diff / 2;
            break;
            
        case EXPAND_BASE::TOP_RIGHT:
            target_x = -width_diff / 2;
            target_y = height_diff / 2;
            break;
            
        case EXPAND_BASE::BOTTOM_LEFT:
            target_x = width_diff / 2;
            target_y = -height_diff / 2;
            break;
            
        case EXPAND_BASE::BOTTOM_RIGHT:
            target_x = -width_diff / 2;
            target_y = -height_diff / 2;
            break;
    }
}

void Histogram::draw() {
    U8G2& u8g2 = m_ui.getU8G2();
    int current_x = coord_x_ + anim_x;
    int current_y = coord_y_ + anim_y;
    int half_width = anim_w / 2;
    int half_height = anim_h / 2;
    
    // Clear drawing area (background box)
    u8g2.setDrawColor(0);
    u8g2.drawBox(current_x - half_width + 2, current_y - half_height, 2 * half_width - 4, 2 * half_height);
    u8g2.setDrawColor(1);
    
    // Draw border corners
    // Top left corner
    u8g2.drawLine(current_x - half_width, current_y - half_height, 
                  current_x - half_width + 4, current_y - half_height);
    u8g2.drawLine(current_x - half_width, current_y - half_height, 
                  current_x - half_width, current_y - half_height + 4);
    // Top right corner
    u8g2.drawLine(current_x + half_width, current_y - half_height, 
                  current_x + half_width - 4, current_y - half_height);
    u8g2.drawLine(current_x + half_width, current_y - half_height, 
                  current_x + half_width, current_y + half_height + 4);
    // Bottom left corner
    u8g2.drawLine(current_x - half_width, current_y + half_height, 
                  current_x - half_width + 4, current_y + half_height);
    u8g2.drawLine(current_x - half_width, current_y + half_height, 
                  current_x - half_width, current_y + half_height - 4);
    // Bottom right corner
    u8g2.drawLine(current_x + half_width, current_y + half_height, 
                  current_x + half_width - 4, current_y + half_height);
    u8g2.drawLine(current_x + half_width, current_y + half_height, 
                  current_x + half_width, current_y + half_height - 4);
    
    // Draw border - using animated coordinates
    // Left vertical line with thickness of 2
    u8g2.drawLine(current_x - half_width, current_y - half_height, 
                  current_x - half_width, current_y + half_height);
    u8g2.drawLine(current_x - half_width + 1, current_y - half_height, 
                  current_x - half_width + 1, current_y + half_height);
    // Right vertical line with thickness of 2
    u8g2.drawLine(current_x + half_width, current_y - half_height, 
                  current_x + half_width, current_y + half_height);
    u8g2.drawLine(current_x + half_width - 1, current_y - half_height, 
                  current_x + half_width - 1, current_y + half_height);
    
    // Draw histogram data from internal buffer
    drawHistogramData(current_x, current_y, half_width, half_height, u8g2);
    
    // Draw label
    u8g2.setFont(u8g2_font_4x6_tr);
    u8g2.drawStr(current_x + half_width - 19, current_y - half_height + 7, "Hist");
}

void Histogram::drawHistogramData(int center_x, int center_y, int half_width, int half_height, U8G2& u8g2) {
    if (m_data_buffer == nullptr || m_data_count == 0 || m_max_value <= 0.0f) {
        return;
    }
    
    // Determine how many points to draw based on current widget width
    int points_to_draw = std::min(static_cast<int>(anim_w), static_cast<int>(m_data_count));
    if (points_to_draw <= 0) {
        return;
    }
    
    // Calculate normalization factor for auto-scaling
    float scale_factor = static_cast<float>(anim_h - 4) / m_max_value; // Leave 2px margin top/bottom
    
    // Draw from right to left (newest data on the right)
    for (int i = 0; i < points_to_draw; ++i) {
        // Calculate data index (newest data first, going backwards)
        int buffer_offset = (i + 1); // How many steps back from current write position
        int data_index = (m_write_index - buffer_offset + m_buffer_size) % m_buffer_size;
        
        // Skip if this index doesn't have valid data yet
        if (buffer_offset > m_data_count) {
            continue;
        }
        
        float value = m_data_buffer[data_index];
        
        // Normalize the value
        int bar_height = static_cast<int>(value * scale_factor);
        
        // Calculate x position (from right to left)
        int x_pos = center_x + half_width - 2 - i; // Start from right edge, move left
        
        // Make sure we don't draw outside the widget bounds
        if (x_pos < center_x - half_width + 2) {
            break;
        }
        
        // Calculate y positions
        int y_bottom = center_y + half_height - 2; // Bottom of the chart area
        int y_top = y_bottom - bar_height;
        
        // Draw the bar
        if (bar_height > 0) {
            u8g2.drawLine(x_pos, y_bottom, x_pos, y_top);
        }
    }
}
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

#include "widgets/curve_chart/curve_chart.h"
#include <cmath>
#include <algorithm>
#include <limits>

/**
 * @brief Constructor for CurveChart widget.
 * @param ui Reference to the PixelUI instance.
 * @param pos_x X coordinate of the widget's top-left corner (new anchor).
 * @param pos_y Y coordinate of the widget's top-left corner (new anchor).
 */
CurveChart::CurveChart(PixelUI& ui, uint16_t pos_x, uint16_t pos_y, uint16_t size_w, uint16_t size_h) : 
    m_ui(ui), 
    pos_x_(pos_x), 
    pos_y_(pos_y),
    size_w_(size_w),
    size_h_(size_h)
{
    // pos_x_ and pos_y_ now represent the top-left anchor point.
    int32_t start_anim_x = (size_w_ / 2);
    int32_t start_anim_y = (size_h_ / 2);

    anim_w = 0; 
    anim_h = 0;

    anim_x = start_anim_x;
    anim_y = start_anim_y;
}

void CurveChart::onLoad() {
    int32_t start_anim_x = (size_w_ / 2);
    int32_t start_anim_y = (size_h_ / 2);

    anim_w = 0; 
    anim_h = 0;

    anim_x = start_anim_x;
    anim_y = start_anim_y;

    m_ui.animate(anim_w, size_w_, 550, EasingType::EASE_OUT_QUAD, PROTECTION::PROTECTED);
    m_ui.animate(anim_h, size_h_, 600, EasingType::EASE_OUT_QUAD, PROTECTION::PROTECTED);

    m_ui.animate(anim_x, 0, 550, EasingType::EASE_OUT_QUAD, PROTECTION::PROTECTED);
    m_ui.animate(anim_y, 0, 600, EasingType::EASE_OUT_QUAD, PROTECTION::PROTECTED);
    
    // ---------------------------------
    FocusBox fbox;
    fbox.x = pos_x_ + 1; // Top-Left X + 1
    fbox.y = pos_y_ + 1; // Top-Left Y + 1
    fbox.w = size_w_ - 1;
    fbox.h = size_h_ - 1;
    
    setFocusBox(fbox);

    // Initialize internal data structures
    initializeDataBuffer();
}

void CurveChart::initializeDataBuffer() {
    // Initialize buffer with expanded width size
    m_buffer_size = exp_w > 0 ? exp_w : 200; // Default to 200 if exp_w not set

    m_data_buffer = std::make_unique<float[]>(m_buffer_size); // acquisition for buffer memory
    
    // Initialize buffer with zeros
    for (int i = 0; i < m_buffer_size; ++i) {
        m_data_buffer[i] = 0.0f;
    }
    
    // Initialize statistics
    m_write_index = 0;
    m_data_count = 0;
    m_max_value = 0.0f;
    m_sum_value = 0.0f;
    // Initialize min value to the maximum possible float to ensure the first added value sets the correct minimum
    m_min_value = std::numeric_limits<float>::max(); 
}

void CurveChart::onOffload() {

}

void CurveChart::addData(float value) {
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
    
void CurveChart::updateStatistics(float new_value, float old_value, bool replacing_data) {
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

void CurveChart::recalculateExtremes() {
    if (m_data_count == 0) {
        m_max_value = 0.0f;
        // Reset m_min_value for future incoming data
        m_min_value = std::numeric_limits<float>::max(); 
        return;
    }
    
    // Check if the buffer is full; if not, only scan up to m_data_count
    int scan_limit = m_data_count; 
    
    // Start with the first data point. Since m_data_buffer is a ring buffer, 
    // the first valid point may not be at index 0 if m_data_count == m_buffer_size
    // However, since we only call this when the buffer is full or an extreme is removed, 
    // we can safely scan the whole allocated size if it's full, or up to data_count.
    // Simpler and safer: use m_data_count for limit. The first valid point 
    // relative to the oldest one is at m_write_index.

    // To scan only valid data points:
    float current_max = -std::numeric_limits<float>::max();
    float current_min = std::numeric_limits<float>::max();
    
    // The safest way to scan the currently valid data in a circular buffer:
    for (int i = 0; i < scan_limit; ++i) {
        // Calculate the index starting from the oldest data point
        int data_index = (m_write_index - m_data_count + i + m_buffer_size) % m_buffer_size;
        float value = m_data_buffer[data_index];
        current_max = std::max(current_max, value);
        current_min = std::min(current_min, value);
    }
    
    m_max_value = current_max;
    m_min_value = current_min;
}

float CurveChart::getMaxValue() const {
    return m_max_value;
}

float CurveChart::getAverageValue() const {
    if (m_data_count == 0) {
        return 0.0f;
    }
    return m_sum_value / m_data_count;
}

float CurveChart::getMinValue() const {
    // If the buffer isn't full and no data has been added, m_min_value is MAX_FLOAT. Return 0.
    if (m_data_count == 0 || m_min_value == std::numeric_limits<float>::max()) {
        return 0.0f;
    }
    return m_min_value;
}

void CurveChart::clearData() {
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

bool CurveChart::handleEvent(InputEvent event) {
    if (event == InputEvent::SELECT) {
        is_expanded = false;
        contractWidget();
        return true;
    }
    return false;
}

bool CurveChart::onSelect(){
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

void CurveChart::expandWidget() {
    int32_t target_x, target_y;
    calculateExpandPosition(target_x, target_y);
    
    // Animate to expanded size and position
    m_ui.animate(anim_w, exp_w, 400, EasingType::EASE_OUT_QUAD);
    m_ui.animate(anim_h, exp_h, 350, EasingType::EASE_OUT_QUAD);
    // Animate the Top-Left position to the calculated target offsets
    m_ui.animate(anim_x, target_x, 400, EasingType::EASE_OUT_QUAD);
    m_ui.animate(anim_y, target_y, 350, EasingType::EASE_OUT_QUAD);
}

void CurveChart::contractWidget() {
    // Animate back to original size and position
    m_ui.animate(anim_w, size_w_, 350, EasingType::EASE_OUT_QUAD, PROTECTION::PROTECTED);
    m_ui.animate(anim_h, size_h_, 400, EasingType::EASE_OUT_QUAD, PROTECTION::PROTECTED);
    // Animate the Top-Left position back to (0, 0) offset
    m_ui.animate(anim_x, 0, 350, EasingType::EASE_OUT_QUAD, PROTECTION::PROTECTED);
    m_ui.animate(anim_y, 0, 400, EasingType::EASE_OUT_QUAD, PROTECTION::PROTECTED);
}

/**
 * @brief Calculate target animation offsets for expansion based on the base corner.
 * The offsets (anim_x, anim_y) are applied to the top-left corner (pos_x_, pos_y_).
 * @param target_x Output target x offset.
 * @param target_y Output target y offset.
 */
void CurveChart::calculateExpandPosition(int32_t& target_x, int32_t& target_y) {
    int32_t width_diff = exp_w - size_w_;
    int32_t height_diff = exp_h - size_h_;
    
    switch (base_) {
        case EXPAND_BASE::TOP_LEFT:
            // Top-Left corner (pos_x_ + anim_x) should stay fixed at pos_x_
            target_x = 0;
            target_y = 0;
            break;
            
        case EXPAND_BASE::TOP_RIGHT:
            // Top-Right corner should stay fixed: tl_x + width = fixed
            target_x = -width_diff;
            target_y = 0;
            break;
            
        case EXPAND_BASE::BOTTOM_LEFT:
            // Bottom-Left corner should stay fixed: tl_y + height = fixed
            target_x = 0;
            target_y = -height_diff;
            break;
            
        case EXPAND_BASE::BOTTOM_RIGHT:
            // Bottom-Right corner should stay fixed
            target_x = -width_diff;
            target_y = -height_diff;
            break;
    }
}

void CurveChart::draw() {
    U8G2& u8g2 = m_ui.getU8G2();
    // tl_x and tl_y are the animated top-left corner coordinates
    int tl_x = pos_x_ + anim_x; 
    int tl_y = pos_y_ + anim_y; 
    int current_w = anim_w;
    int current_h = anim_h;
    
    // Clear drawing area (background box)
    u8g2.setDrawColor(0);
    // Draw box relative to top-left corner
    u8g2.drawBox(tl_x + 2, tl_y, current_w - 4, current_h);
    u8g2.setDrawColor(1);
    
    // --- Draw border corners ---
    // Top-Left: (tl_x, tl_y)
    u8g2.drawLine(tl_x, tl_y, tl_x + 4, tl_y);
    u8g2.drawLine(tl_x, tl_y, tl_x, tl_y + 4);
    
    // Top-Right: (tl_x + current_w, tl_y)
    u8g2.drawLine(tl_x + current_w, tl_y, tl_x + current_w - 4, tl_y);
    u8g2.drawLine(tl_x + current_w, tl_y, tl_x + current_w, tl_y + 4); 

    // Bottom-Left: (tl_x, tl_y + current_h)
    u8g2.drawLine(tl_x, tl_y + current_h, tl_x + 4, tl_y + current_h);
    u8g2.drawLine(tl_x, tl_y + current_h, tl_x, tl_y + current_h - 4);
    
    // Bottom-Right: (tl_x + current_w, tl_y + current_h)
    u8g2.drawLine(tl_x + current_w, tl_y + current_h, tl_x + current_w - 4, tl_y + current_h);
    u8g2.drawLine(tl_x + current_w, tl_y + current_h, tl_x + current_w, tl_y + current_h - 4);

    // Draw border - using animated coordinates (thickness of 2)
    // Left vertical line
    u8g2.drawLine(tl_x, tl_y, tl_x, tl_y + current_h);
    u8g2.drawLine(tl_x + 1, tl_y, tl_x + 1, tl_y + current_h);
    // Right vertical line
    u8g2.drawLine(tl_x + current_w, tl_y, tl_x + current_w, tl_y + current_h);
    u8g2.drawLine(tl_x + current_w - 1, tl_y, tl_x + current_w - 1, tl_y + current_h);
    
    // Draw curve data from internal buffer, passing top-left coordinates, current width, and height
    drawCuveData(tl_x, tl_y, current_w, current_h, u8g2);
    
    // Draw label
    u8g2.setFont(u8g2_font_4x6_tr);
    // Positioned relative to the top-right corner
    u8g2.drawStr(tl_x + current_w - 23, tl_y + 7, "Curve"); 
}

/**
 * @brief Draws the curve data by connecting points with lines.
 * Data is plotted from right (newest) to left (oldest).
 * @param tl_x Widget top-left x coordinate.
 * @param tl_y Widget top-left y coordinate.
 * @param width Current width of the widget.
 * @param height Current height of the widget.
 * @param u8g2 Reference to U8G2 for drawing.
 */
void CurveChart::drawCuveData(int tl_x, int tl_y, int width, int height, U8G2& u8g2) {
    // Return early if there's no data or the buffer is invalid
    if (m_data_buffer == nullptr || m_data_count == 0) {
        return;
    }
    
    // Chart dimensions, excluding the 2px border margin on all sides
    // tl_x + 2 to tl_x + width - 2 (width - 4)
    // tl_y + 2 to tl_y + height - 2 (height - 4)
    const int chart_w = width - 4;
    const int chart_h = height - 4;

    // Determine how many horizontal steps to draw (limited by widget width or data count)
    int points_to_draw = std::min(chart_w, static_cast<int>(m_data_count));
    if (points_to_draw <= 1) { 
        // Handle single point case (optional)
        if (points_to_draw == 1) {
            int data_index = (m_write_index - 1 + m_buffer_size) % m_buffer_size;
            float value = m_data_buffer[data_index];
            
            float range = m_max_value - m_min_value;
            if (range < 1e-6) range = 1.0f; // Prevent div by zero
            float scale_factor = static_cast<float>(chart_h) / range;
            
            const int y_bottom = tl_y + height - 2; // Bottom of chart area
            
            float normalized_value = value - m_min_value;
            int y_offset = static_cast<int>(normalized_value * scale_factor);
            
            int current_y = y_bottom - y_offset; 
            int current_x = tl_x + width - 3; // Position for the single point (right side of chart area)
            
            u8g2.drawPixel(current_x, current_y);
        }
        return;
    }
    
    // Calculate normalization factor for auto-scaling
    float range = m_max_value - m_min_value;
    
    if (range < 1e-6) {
        range = 1.0f; 
    }
    
    // Scale factor maps the data range to the chart height
    float scale_factor = static_cast<float>(chart_h) / range;
    
    // Drawing area Y boundary (Bottom of the chart area)
    const int y_bottom = tl_y + height - 2;
    const int y_top = tl_y + 2;

    int prev_x = -1;
    int prev_y = -1;
    
    // Loop from newest data (i=0, rightmost pixel) backwards to the oldest data (i=points_to_draw-1, leftmost pixel)
    for (int i = 0; i < points_to_draw; ++i) {
        
        // 1. Calculate data index
        int buffer_offset = i + 1; 
        int data_index = (m_write_index - buffer_offset + m_buffer_size) % m_buffer_size;
        
        if (buffer_offset > m_data_count) {
            continue;
        }
        
        float value = m_data_buffer[data_index];
        
        // 2. Calculate coordinates
        // X position: Right Edge of chart area (tl_x + width - 2), move left by 'i' pixels
        int current_x = tl_x + width - 2 - i; 
        
        // Y position: Normalize value relative to min_value, then scale, and position relative to y_bottom
        float normalized_value = value - m_min_value;
        int y_offset = static_cast<int>(normalized_value * scale_factor);
        
        // Y coordinate (0 is top, y_bottom is bottom)
        int current_y = y_bottom - y_offset; 
        
        // Ensure Y stays within the vertical bounds of the chart area 
        current_y = std::max(y_top, current_y);
        current_y = std::min(y_bottom, current_y);

        // 3. Draw line segment
        if (prev_x != -1) {
            // Draw line from the previously plotted point (i-1, newer, on the right) 
            // to the current point (i, older, on the left)
            u8g2.drawLine(current_x, current_y, prev_x, prev_y);
        }
        
        // 4. Update previous coordinates for the next iteration (which is the next older point)
        prev_x = current_x;
        prev_y = current_y;
    }
}

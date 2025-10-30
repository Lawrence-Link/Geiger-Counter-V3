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
#include <limits> // Added for std::numeric_limits

/**
 * @brief Constructor for Histogram widget.
 * @param ui Reference to the PixelUI instance for rendering and animation.
 * @param pos_x X coordinate of the widget's top-left corner.
 * @param pos_y Y coordinate of the widget's top-left corner.
 */
Histogram::Histogram(PixelUI& ui, uint16_t pos_x, uint16_t pos_y, uint16_t size_w, uint16_t size_h) : 
    m_ui(ui), 
    pos_x_(pos_x), 
    pos_y_(pos_y),
    size_w_(size_w),
    size_h_(size_h)
{
    // pos_x_ and pos_y_ now represent the top-left anchor point.
    int32_t start_anim_x = (size_w_ / 2);
    int32_t start_anim_y = (size_h_ / 2);

    anim_w = 4; 
    anim_h = 4;

    anim_x = start_anim_x;
    anim_y = start_anim_y;
}

/**
 * @brief Initialize widget when loaded. Sets up animations, focus, and internal buffer.
 */
void Histogram::onLoad() {
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

    // Initialize the internal data buffer
    initializeDataBuffer();
}

/**
 * @brief Initialize the circular data buffer and statistics.
 */
void Histogram::initializeDataBuffer() {
    // Allocate buffer based on expanded width or default size
    m_buffer_size = exp_w > 0 ? exp_w : 200;
    
    // Check if buffer already exists before allocating
    // if (m_data_buffer != nullptr) {
    //     delete[] m_data_buffer;
    // }
    m_data_buffer = std::make_unique<float[]> (m_buffer_size);

    // Clear buffer
    for (int i = 0; i < m_buffer_size; ++i) {
        m_data_buffer[i] = 0.0f;
    }

    // Reset statistics
    m_write_index = 0;
    m_data_count = 0;
    m_max_value = 0.0f;
    m_sum_value = 0.0f;
    m_min_value = std::numeric_limits<float>::max();
}

/**
 * @brief Free resources when widget is offloaded.
 */
void Histogram::onOffload() {
    
}

/**
 * @brief Add a new data point to the histogram buffer.
 * @param value The new float value to add.
 */
void Histogram::addData(float value) {
    if (m_data_buffer == nullptr) {
        initializeDataBuffer();
    }

    // Save old value for statistics update
    float old_value = m_data_buffer[m_write_index];
    bool replacing_valid_data = (m_data_count >= m_buffer_size);

    // Write new value to circular buffer
    m_data_buffer[m_write_index] = value;

    // Update write index with wrap-around
    m_write_index = (m_write_index + 1) % m_buffer_size;

    // Update data count if buffer not full
    if (m_data_count < m_buffer_size) {
        m_data_count++;
    }

    // Update running statistics
    updateStatistics(value, old_value, replacing_valid_data);
}

/**
 * @brief Update max, min, and sum statistics efficiently.
 * @param new_value The new value being added.
 * @param old_value The old value being replaced (if any).
 * @param replacing_data True if replacing an existing value in a full buffer.
 */
void Histogram::updateStatistics(float new_value, float old_value, bool replacing_data) {
    if (!replacing_data) {
        // Buffer not full, simple update
        m_sum_value += new_value;
        m_max_value = std::max(m_max_value, new_value);
        // Only update min if a valid (non-max) value is added or if min is still at its maximum possible value
        if (m_min_value == std::numeric_limits<float>::max() || new_value < m_min_value) {
            m_min_value = new_value;
        }
    } else {
        // Replace existing value
        m_sum_value = m_sum_value - old_value + new_value;

        // Recalculate extremes if old value was max or min (recalculating min if old_value was the placeholder max is safer)
        if (old_value == m_max_value || old_value == m_min_value || m_min_value == std::numeric_limits<float>::max()) {
            recalculateExtremes();
        } else {
            m_max_value = std::max(m_max_value, new_value);
            m_min_value = std::min(m_min_value, new_value);
        }
    }
}

/**
 * @brief Recalculate the max and min values by scanning the buffer.
 */
void Histogram::recalculateExtremes() {
    if (m_data_count == 0) {
        m_max_value = 0.0f;
        m_min_value = std::numeric_limits<float>::max(); // Reset min to max
        m_sum_value = 0.0f;
        return;
    }

    m_max_value = m_data_buffer[0];
    m_min_value = m_data_buffer[0];

    for (int i = 1; i < m_data_count; ++i) {
        m_max_value = std::max(m_max_value, m_data_buffer[i]);
        m_min_value = std::min(m_min_value, m_data_buffer[i]);
    }
}

/**
 * @brief Get the maximum value in the histogram.
 * @return Maximum float value in buffer.
 */
float Histogram::getMaxValue() const {
    return m_max_value;
}

/**
 * @brief Get the average value of the histogram.
 * @return Average float value or 0 if buffer empty.
 */
float Histogram::getAverageValue() const {
    if (m_data_count == 0) {
        return 0.0f;
    }
    return m_sum_value / m_data_count;
}

/**
 * @brief Get the minimum value in the histogram.
 * @return Minimum float value in buffer (or 0.0f if empty).
 */
float Histogram::getMinValue() const {
    if (m_data_count == 0) return 0.0f;
    return m_min_value;
}

/**
 * @brief Clear all data in the histogram buffer and reset statistics.
 */
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

/**
 * @brief Handle user input events.
 * @param event Input event from UI system.
 * @return True if the event was handled.
 */
bool Histogram::handleEvent(InputEvent event) {
    if (event == InputEvent::SELECT) {
        is_expanded = false;
        contractWidget();
        return true;
    }
    return false;
}

/**
 * @brief Handle widget selection toggle.
 * @return True after processing selection.
 */
bool Histogram::onSelect() {
    m_ui.clearUnprotectedAnimations();
    if (!is_expanded) {
        expandWidget();
        is_expanded = true;
    } else {
        contractWidget();
        is_expanded = false;
    }
    return true;
}

/**
 * @brief Animate widget expansion to target size and position.
 */
void Histogram::expandWidget() {
    int32_t target_x, target_y;
    calculateExpandPosition(target_x, target_y);

    m_ui.animate(anim_w, exp_w, 400, EasingType::EASE_OUT_QUAD);
    m_ui.animate(anim_h, exp_h, 350, EasingType::EASE_OUT_QUAD);
    // Animate the Top-Left position to the calculated target offsets
    m_ui.animate(anim_x, target_x, 400, EasingType::EASE_OUT_QUAD);
    m_ui.animate(anim_y, target_y, 350, EasingType::EASE_OUT_QUAD);
}

/**
 * @brief Animate widget contraction back to original size and position.
 */
void Histogram::contractWidget() {
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
void Histogram::calculateExpandPosition(int32_t& target_x, int32_t& target_y) {
    int32_t width_diff = exp_w - size_w_;
    int32_t height_diff = exp_h - size_h_;

    switch (base_) {
        case EXPAND_BASE::TOP_LEFT:
            // Top-Left corner (pos_x_ + anim_x) should stay fixed at pos_x_
            target_x = 0;
            target_y = 0;
            break;
        case EXPAND_BASE::TOP_RIGHT:
            // Top-Right corner (pos_x_ + anim_x + anim_w) should stay fixed
            target_x = -width_diff;
            target_y = 0;
            break;
        case EXPAND_BASE::BOTTOM_LEFT:
            // Bottom-Left corner (pos_y_ + anim_y + anim_h) should stay fixed
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

/**
 * @brief Render the histogram widget on the screen.
 */
void Histogram::draw() {
    U8G2& u8g2 = m_ui.getU8G2();
    
    // tl_x and tl_y are the animated top-left corner coordinates
    int tl_x = pos_x_ + anim_x; 
    int tl_y = pos_y_ + anim_y; 
    int current_w = anim_w;
    int current_h = anim_h;

    // Clear background area (relative to top-left)
    u8g2.setDrawColor(0);
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

    // Draw vertical borders
    u8g2.drawLine(tl_x, tl_y, tl_x, tl_y + current_h);
    u8g2.drawLine(tl_x + 1, tl_y, tl_x + 1, tl_y + current_h);
    u8g2.drawLine(tl_x + current_w, tl_y, tl_x + current_w, tl_y + current_h);
    u8g2.drawLine(tl_x + current_w - 1, tl_y, tl_x + current_w - 1, tl_y + current_h);

    // Draw histogram bars, passing top-left coordinates, current width, and height
    drawHistogramData(tl_x, tl_y, current_w, current_h, u8g2);

    // Draw label (positioned relative to the top-right corner)
    u8g2.setFont(u8g2_font_4x6_tr);
    u8g2.drawStr(tl_x + current_w - 19, tl_y + 7, "Hist");
}

/**
 * @brief Draw the bars representing histogram data.
 * @param tl_x Widget top-left x coordinate.
 * @param tl_y Widget top-left y coordinate.
 * @param width Current width of the widget.
 * @param height Current height of the widget.
 * @param u8g2 Reference to U8G2 for drawing.
 */
void Histogram::drawHistogramData(int tl_x, int tl_y, int width, int height, U8G2& u8g2) {
    if (m_data_buffer == nullptr || m_data_count == 0 || m_max_value <= 0.0f) {
        return;
    }

    // Determine number of points to draw based on widget width
    int points_to_draw = std::min(static_cast<int>(width), static_cast<int>(m_data_count));
    if (points_to_draw <= 0) return;

    // Compute scale factor for normalizing bar heights (height - 4 to account for border/padding)
    float scale_factor = static_cast<float>(height - 4) / m_max_value;

    // Draw bars from right (newest) to left (oldest)
    for (int i = 0; i < points_to_draw; ++i) {
        int buffer_offset = i + 1;
        int data_index = (m_write_index - buffer_offset + m_buffer_size) % m_buffer_size;

        if (buffer_offset > m_data_count) continue;

        float value = m_data_buffer[data_index];
        int bar_height = static_cast<int>(value * scale_factor);

        // X position: Right Edge - 2 pixels of padding - i
        int x_pos = tl_x + width - 2 - i;
        
        // Break if we run off the left side of the widget content area (tl_x + 2)
        if (x_pos < tl_x + 2) break;

        // Y Bottom: Bottom Edge - 1 pixel of padding
        int y_bottom = tl_y + height - 1;
        // Y Top: Bottom - bar_height
        int y_top = y_bottom - bar_height;

        if (bar_height > 0) {
            u8g2.drawLine(x_pos, y_bottom, x_pos, y_top);
        }
    }
}

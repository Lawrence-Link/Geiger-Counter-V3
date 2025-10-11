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

/**
 * @brief Constructor for Histogram widget.
 * @param ui Reference to the PixelUI instance for rendering and animation.
 * @param coord_x X coordinate of the widget center.
 * @param coord_y Y coordinate of the widget center.
 */
Histogram::Histogram(PixelUI& ui, uint16_t coord_x, uint16_t coord_y) 
    : m_ui(ui), coord_x_(coord_x), coord_y_(coord_y) 
{
    // Constructor does not automatically initialize animation or data buffer
}

/**
 * @brief Initialize widget when loaded. Sets up animations, focus, and internal buffer.
 */
void Histogram::onLoad() {
    // Reset animation offsets
    anim_x = 0;
    anim_y = 0;

    // Animate size from zero to margin size
    m_ui.animate(anim_w, margin_w_, 550, EasingType::EASE_OUT_QUAD, PROTECTION::PROTECTED);
    m_ui.animate(anim_h, margin_h_, 600, EasingType::EASE_OUT_QUAD, PROTECTION::PROTECTED);

    // Set focus box slightly smaller than margin for visual highlighting
    FocusBox fbox;
    fbox.x = coord_x_ - margin_w_ / 2 + 1;
    fbox.y = coord_y_ - margin_h_ / 2 + 1;
    fbox.w = margin_w_ - 1;
    fbox.h = margin_h_ - 1;
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
    m_data_buffer = new float[m_buffer_size];

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
    if (m_data_buffer != nullptr) {
        delete[] m_data_buffer;
        m_data_buffer = nullptr;
    }
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
        m_min_value = std::min(m_min_value, new_value);
    } else {
        // Replace existing value
        m_sum_value = m_sum_value - old_value + new_value;

        // Recalculate extremes if old value was max or min
        if (old_value == m_max_value || old_value == m_min_value) {
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
 * @return Minimum float value in buffer.
 */
float Histogram::getMinValue() const {
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
    m_ui.animate(anim_x, target_x, 400, EasingType::EASE_OUT_QUAD);
    m_ui.animate(anim_y, target_y, 350, EasingType::EASE_OUT_QUAD);
}

/**
 * @brief Animate widget contraction back to original size and position.
 */
void Histogram::contractWidget() {
    m_ui.animate(anim_w, margin_w_, 350, EasingType::EASE_OUT_QUAD, PROTECTION::PROTECTED);
    m_ui.animate(anim_h, margin_h_, 400, EasingType::EASE_OUT_QUAD, PROTECTION::PROTECTED);
    m_ui.animate(anim_x, 0, 350, EasingType::EASE_OUT_QUAD, PROTECTION::PROTECTED);
    m_ui.animate(anim_y, 0, 400, EasingType::EASE_OUT_QUAD, PROTECTION::PROTECTED);
}

/**
 * @brief Calculate target animation offsets for expansion based on the base corner.
 * @param target_x Output target x offset.
 * @param target_y Output target y offset.
 */
void Histogram::calculateExpandPosition(int32_t& target_x, int32_t& target_y) {
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

/**
 * @brief Render the histogram widget on the screen.
 */
void Histogram::draw() {
    U8G2& u8g2 = m_ui.getU8G2();
    int current_x = coord_x_ + anim_x;
    int current_y = coord_y_ + anim_y;
    int half_width = anim_w / 2;
    int half_height = anim_h / 2;

    // Clear background area
    u8g2.setDrawColor(0);
    u8g2.drawBox(current_x - half_width + 2, current_y - half_height, 2 * half_width - 4, 2 * half_height);
    u8g2.setDrawColor(1);

    // Draw border corners
    u8g2.drawLine(current_x - half_width, current_y - half_height, current_x - half_width + 4, current_y - half_height);
    u8g2.drawLine(current_x - half_width, current_y - half_height, current_x - half_width, current_y - half_height + 4);
    u8g2.drawLine(current_x + half_width, current_y - half_height, current_x + half_width - 4, current_y - half_height);
    u8g2.drawLine(current_x + half_width, current_y - half_height, current_x + half_width, current_y + half_height + 4);
    u8g2.drawLine(current_x - half_width, current_y + half_height, current_x - half_width + 4, current_y + half_height);
    u8g2.drawLine(current_x - half_width, current_y + half_height, current_x - half_width, current_y + half_height - 4);
    u8g2.drawLine(current_x + half_width, current_y + half_height, current_x + half_width - 4, current_y + half_height);
    u8g2.drawLine(current_x + half_width, current_y + half_height, current_x + half_width, current_y + half_height - 4);

    // Draw vertical borders
    u8g2.drawLine(current_x - half_width, current_y - half_height, current_x - half_width, current_y + half_height);
    u8g2.drawLine(current_x - half_width + 1, current_y - half_height, current_x - half_width + 1, current_y + half_height);
    u8g2.drawLine(current_x + half_width, current_y - half_height, current_x + half_width, current_y + half_height);
    u8g2.drawLine(current_x + half_width - 1, current_y - half_height, current_x + half_width - 1, current_y + half_height);

    // Draw histogram bars
    drawHistogramData(current_x, current_y, half_width, half_height, u8g2);

    // Draw label
    u8g2.setFont(u8g2_font_4x6_tr);
    u8g2.drawStr(current_x + half_width - 19, current_y - half_height + 7, "Hist");
}

/**
 * @brief Draw the bars representing histogram data.
 * @param center_x Widget center x coordinate.
 * @param center_y Widget center y coordinate.
 * @param half_width Half of widget width.
 * @param half_height Half of widget height.
 * @param u8g2 Reference to U8G2 for drawing.
 */
void Histogram::drawHistogramData(int center_x, int center_y, int half_width, int half_height, U8G2& u8g2) {
    if (m_data_buffer == nullptr || m_data_count == 0 || m_max_value <= 0.0f) {
        return;
    }

    // Determine number of points to draw based on widget width
    int points_to_draw = std::min(static_cast<int>(anim_w), static_cast<int>(m_data_count));
    if (points_to_draw <= 0) return;

    // Compute scale factor for normalizing bar heights
    float scale_factor = static_cast<float>(anim_h - 4) / m_max_value;

    // Draw bars from right (newest) to left (oldest)
    for (int i = 0; i < points_to_draw; ++i) {
        int buffer_offset = i + 1;
        int data_index = (m_write_index - buffer_offset + m_buffer_size) % m_buffer_size;

        if (buffer_offset > m_data_count) continue;

        float value = m_data_buffer[data_index];
        int bar_height = static_cast<int>(value * scale_factor);

        int x_pos = center_x + half_width - 2 - i;
        if (x_pos < center_x - half_width + 2) break;

        int y_bottom = center_y + half_height - 2;
        int y_top = y_bottom - bar_height;

        if (bar_height > 0) {
            u8g2.drawLine(x_pos, y_bottom, x_pos, y_top);
        }
    }
}

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

#include "widgets/num_scroll/num_scroll.h"
#include <cstdio>
#include "config.h"
#include <etl/string.h>

/**
 * @brief Constructor for NumScroll widget.
 * @param ui Reference to PixelUI instance for rendering and animation.
 */
NumScroll::NumScroll(PixelUI& ui) : m_ui(ui) {
    setFocusable(true);
}

/**
 * @brief Initialize widget with animation.
 */
void NumScroll::onLoad() {
    m_anim_offset = 0;
    m_is_active = false;

    anim_w = 0;
    anim_h = 0;

    // Set focus box slightly inside widget boundaries
    FocusBox box;
    box.x = m_x + 1;
    box.y = m_y + 1;
    box.w = m_margin_w - 2;
    box.h = m_margin_h - 2;
    setFocusBox(box);

    // Animate size from 0 to margin size
    m_ui.animate(anim_w, anim_h,
                 m_margin_w, m_margin_h,
                 200, EasingType::EASE_OUT_CUBIC,
                 PROTECTION::PROTECTED);
}

/**
 * @brief Initialize widget without animation (immediate size).
 */
void NumScroll::onLoadNoAnim() {
    m_anim_offset = 0;
    m_is_active = false;

    anim_w = 0;
    anim_h = 0;

    FocusBox box;
    box.x = m_x + 1;
    box.y = m_y + 1;
    box.w = m_margin_w - 2;
    box.h = m_margin_h - 2;
    setFocusBox(box);

    // Set final size immediately
    anim_w = m_margin_w;
    anim_h = m_margin_h;
}

/**
 * @brief Clean up widget resources when offloaded.
 */
void NumScroll::onOffload() {
    m_ui.clearUnprotectedAnimations();
    m_is_active = false;
}

/**
 * @brief Handle selection of the widget.
 * @return True if widget requests input control, false otherwise.
 */
bool NumScroll::onSelect() {
    // Toggle active state
    if (!m_is_active) {
        m_ui.markDirty();
        return true; // Request input control
    } else {
        m_ui.markDirty();
        return false; // Return input control
    }
}

/**
 * @brief Handle user input events.
 * @param event Input event to process.
 * @return True if event consumed, false otherwise.
 */
bool NumScroll::handleEvent(InputEvent event) {
    if (!m_is_active) {
        return false;
    }

    // Update last interaction timestamp
    updateInteractionTime(m_ui.getCurrentTime());

    switch (event) {
        case InputEvent::LEFT:
            decrementValue();
            return false;

        case InputEvent::RIGHT:
            incrementValue();
            return false;

        case InputEvent::SELECT:
            m_ui.markDirty();
            return true;

        default:
            return false;
    }
}

/**
 * @brief Render the scrollable number widget.
 */
void NumScroll::draw() {
    U8G2& u8g2 = m_ui.getU8G2();

    // Compute animated drawing area (centered)
    int32_t draw_x = m_x + (m_margin_w - anim_w) / 2;
    int32_t draw_y = m_y + (m_margin_h - anim_h) / 2;
    int center_x = draw_x + anim_w / 2;
    int center_y = draw_y + anim_h / 2;

    // Clear background
    u8g2.setDrawColor(0);
    u8g2.drawBox(draw_x + 2, draw_y + 2, anim_w - 4, anim_h - 4);
    u8g2.setDrawColor(1);

    // Draw border, thicker if active
    if (m_is_active) {
        u8g2.drawFrame(draw_x, draw_y, anim_w, anim_h);
        u8g2.drawFrame(draw_x + 1, draw_y + 1, anim_w - 2, anim_h - 2);
    } else {
        u8g2.drawFrame(draw_x, draw_y, anim_w, anim_h);
    }

    // Clip drawing area inside the widget
    u8g2.setClipWindow(draw_x + 3, draw_y + 3,
                       draw_x + anim_w - 3,
                       draw_y + anim_h - 3);

    u8g2.setFont(u8g2_font_tenfatguys_tn);

    const int digit_height = 16;
    char buffer[16];

    // Draw current value with one previous and one next value for scroll effect
    for (int i = -1; i <= 1; i++) {
        int32_t value = m_current_value + i;
        if (value < m_min_value) value = m_min_value;
        if (value > m_max_value) value = m_max_value;

        // Format value for fixed digits if applicable
        formatValue(value, buffer, sizeof(buffer));

        int text_width = u8g2.getStrWidth(buffer);
        int y_pos = center_y + (i * digit_height) + m_anim_offset;

        // Center text horizontally
        u8g2.drawStr(center_x - text_width / 2, y_pos + 5, buffer);
    }

    u8g2.setMaxClipWindow();
    u8g2.setDrawColor(1);
}

/**
 * @brief Set the range of allowed numeric values.
 * @param min_val Minimum allowed value.
 * @param max_val Maximum allowed value.
 */
void NumScroll::setRange(int32_t min_val, int32_t max_val) {
    if (min_val <= max_val) {
        m_min_value = min_val;
        m_max_value = max_val;

        if (m_current_value < m_min_value) {
            m_current_value = m_min_value;
        }
        if (m_current_value > m_max_value) {
            m_current_value = m_max_value;
        }

        m_ui.markDirty();
    }
}

/**
 * @brief Set the current value and animate to it if changed.
 * @param val New value to set.
 */
void NumScroll::setValue(int32_t val) {
    if (val < m_min_value) val = m_min_value;
    if (val > m_max_value) val = m_max_value;

    if (val != m_current_value) {
        animateToValue(val);
    }
}

/**
 * @brief Increment the value by one step if not exceeding max.
 */
void NumScroll::incrementValue() {
    if (m_current_value < m_max_value) {
        animateToValue(m_current_value + 1);
    }
}

/**
 * @brief Decrement the value by one step if not below min.
 */
void NumScroll::decrementValue() {
    if (m_current_value > m_min_value) {
        animateToValue(m_current_value - 1);
    }
}

/**
 * @brief Animate the scroll effect to a new value.
 * @param new_value Target numeric value.
 */
void NumScroll::animateToValue(int32_t new_value) {
    int32_t diff = new_value - m_current_value;
    if (diff == 0) return;

    m_current_value = new_value;

    const int digit_height = 16;
    int32_t start_offset = (diff > 0) ? digit_height : -digit_height;

    // Start animation offset and animate to zero
    m_anim_offset = start_offset;
    m_ui.animate(m_anim_offset, 0, 200, EasingType::EASE_OUT_QUAD);

    m_ui.markDirty();
}

/**
 * @brief Format numeric value into buffer with optional leading zeros.
 * @param value Numeric value to format.
 * @param buffer Destination buffer.
 * @param buf_size Size of buffer.
 */
void NumScroll::formatValue(int32_t value, char* buffer, size_t buf_size) const {
    if (m_fixed_digits > 0) {
        char fmt[8];
        snprintf(fmt, sizeof(fmt), "%%0%dd", m_fixed_digits); // e.g. "%03d"
        snprintf(buffer, buf_size, fmt, (int)value);
    } else {
        snprintf(buffer, buf_size, "%d", (int)value);
    }
}

/*
 * Copyright (C) 2025 Lawrence Link
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "widgets/num_scroll/num_scroll.h"
#include <cstdio>
#include "config.h"
#include <etl/string.h>

NumScroll::NumScroll(PixelUI& ui) : m_ui(ui) {
    setFocusable(true);
}

void NumScroll::onLoad() {
    m_anim_offset = 0;
    m_is_active = false;

    // 更新焦点框，使用 margin 作为宽高
    FocusBox box;
    box.x = m_x;
    box.y = m_y;
    box.w = m_margin_w;
    box.h = m_margin_h;
    setFocusBox(box);
}

void NumScroll::onOffload() {
    m_ui.clearUnprotectedAnimations();
    m_is_active = false;
}

bool NumScroll::onSelect() {
    // 切换激活状态
    if (!m_is_active) {
        m_ui.markDirty();
        return true; // 请求输入控制
    } else {
        m_ui.markDirty();
        return false; // 返回控制
    }
}

bool NumScroll::handleEvent(InputEvent event) {
    if (!m_is_active) {
        return false;
    }

    // 更新交互时间
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

void NumScroll::draw() {
    U8G2& u8g2 = m_ui.getU8G2();

    int center_x = m_x + m_margin_w / 2;
    int center_y = m_y + m_margin_h / 2;

    u8g2.setDrawColor(0);
    u8g2.drawBox(m_x + 2, m_y + 2, m_margin_w - 4, m_margin_h - 4);
    u8g2.setDrawColor(1);

    if (m_is_active) {
        u8g2.drawFrame(m_x, m_y, m_margin_w, m_margin_h);
        u8g2.drawFrame(m_x + 1, m_y + 1, m_margin_w - 2, m_margin_h - 2);
    } else {
        u8g2.drawFrame(m_x, m_y, m_margin_w, m_margin_h);
    }

    u8g2.setClipWindow(m_x + 3, m_y + 3,
                       m_x + m_margin_w - 3,
                       m_y + m_margin_h - 3);

    u8g2.setFont(u8g2_font_tenfatguys_tn);

    const int digit_height = 16;
    char buffer[16];

    for (int i = -1; i <= 1; i++) {
        int32_t value = m_current_value + i;
        if (value < m_min_value) value = m_min_value;
        if (value > m_max_value) value = m_max_value;

        // 格式化输出
        formatValue(value, buffer, sizeof(buffer));

        int text_width = u8g2.getStrWidth(buffer);
        int y_pos = center_y + (i * digit_height) + m_anim_offset;

        // 居中绘制
        u8g2.drawStr(center_x - text_width / 2, y_pos + 5, buffer);
    }

    u8g2.setMaxClipWindow();
    u8g2.setDrawColor(1);
}

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

void NumScroll::setValue(int32_t val) {
    if (val < m_min_value) val = m_min_value;
    if (val > m_max_value) val = m_max_value;

    if (val != m_current_value) {
        animateToValue(val);
    }
}

void NumScroll::incrementValue() {
    if (m_current_value < m_max_value) {
        animateToValue(m_current_value + 1);
    }
}

void NumScroll::decrementValue() {
    if (m_current_value > m_min_value) {
        animateToValue(m_current_value - 1);
    }
}

void NumScroll::animateToValue(int32_t new_value) {
    int32_t diff = new_value - m_current_value;
    if (diff == 0) return;

    m_current_value = new_value;

    const int digit_height = 16;
    int32_t start_offset = (diff > 0) ? digit_height : -digit_height;

    m_anim_offset = start_offset;
    m_ui.animate(m_anim_offset, 0, 200, EasingType::EASE_OUT_QUAD);

    m_ui.markDirty();
}

void NumScroll::formatValue(int32_t value, char* buffer, size_t buf_size) const {
    if (m_fixed_digits > 0) {
        char fmt[8];
        snprintf(fmt, sizeof(fmt), "%%0%dd", m_fixed_digits); // e.g. "%03d"
        snprintf(buffer, buf_size, fmt, (int)value);
    } else {
        snprintf(buffer, buf_size, "%d", (int)value);
    }
}

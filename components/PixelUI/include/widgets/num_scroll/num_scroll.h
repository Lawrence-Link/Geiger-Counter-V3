/*
 * Copyright (C) 2025 Lawrence Link
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include "../IWidget.h"
#include "PixelUI.h"

/**
 * @class NumScroll
 * @brief A scrollable numeric input widget with vertical scrolling animation
 * 
 * This widget displays a number in a square box and allows users to increment
 * or decrement the value using LEFT/RIGHT input events. The scrolling effect
 * is achieved through vertical animation of digit sprites.
 */
class NumScroll : public IWidget {
public:
    NumScroll(PixelUI& ui);
    ~NumScroll() = default;

    void onLoad() override;
    void onLoadNoAnim();
    void onOffload() override;
    void draw() override;
    bool handleEvent(InputEvent event) override;
    bool onSelect() override;
    uint32_t getTimeout() const override { return TIMEOUT_MS; }

    void setPosition(uint16_t x, uint16_t y) { m_x = x; m_y = y; }

    void setRange(int32_t min_val, int32_t max_val);
    void setValue(int32_t val);
    int32_t getValue() const { return m_current_value; }

    void setFixedIntDigits(uint8_t digits) { m_fixed_digits = digits; }
    void setSize(uint16_t w, uint16_t h) { m_margin_w = w; m_margin_h = h; }

private:
    private:
    PixelUI& m_ui;

    uint16_t m_x = 0;
    uint16_t m_y = 0;

    uint16_t m_margin_w = 24;
    uint16_t m_margin_h = 16;

    int32_t m_current_value = 0;
    int32_t m_min_value = 0;
    int32_t m_max_value = 99;
    uint8_t m_fixed_digits = 0; 

    int32_t m_anim_offset = 0;
    
    int32_t anim_w = 0;
    int32_t anim_h = 0;

    static constexpr uint32_t TIMEOUT_MS = 5000;

    void incrementValue();
    void decrementValue();
    void animateToValue(int32_t new_value);

    void formatValue(int32_t value, char* buffer, size_t buf_size) const;
};

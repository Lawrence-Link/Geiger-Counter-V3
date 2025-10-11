/*
 * Copyright (C) 2025 Lawrence Link
 */

#pragma once

#include "PopupBase.h"
#include <stddef.h>
#include <functional>
#include "widgets/num_scroll/num_scroll.h"
#include "focus/focus.h"
/**
 * @class PopupProgress
 * @brief A popup displaying a value window
 */
class PopupValue4Digits : public PopupBase {
private:
    int32_t& _value;
    const char* _title;
    NumScroll num_thousands, num_hundreds, num_tens, num_ones;
    FocusManager m_focusMan;
    std::function<void(int32_t value)> m_cb;

public:
    PopupValue4Digits(PixelUI& ui, uint16_t width, uint16_t height, 
                  int32_t& value,
                  const char* title = "", uint16_t duration = 3000, uint8_t priority = 0, std::function<void(int32_t value)> cb_function = nullptr);
    ~PopupValue4Digits() {
        m_ui.markDirty();
    };

    void drawContent(int16_t centerX, int16_t centerY, int16_t currentWidth, int16_t currentHeight) override;
    bool handleInput(InputEvent event) override;
};
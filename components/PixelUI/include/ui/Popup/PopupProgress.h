/*
 * Copyright (C) 2025 Lawrence Link
 */

#pragma once

#include "PopupBase.h"
#include <stddef.h>
#include <functional>

/**
 * @class PopupProgress
 * @brief A popup displaying a progress bar.
 */
class PopupProgress : public PopupBase {
private:
    int32_t& _value;
    int32_t _minValue, _maxValue;
    const char* _title;
    
    void formatValue(char* buffer, size_t bufferSize) const;
    void formatValueAsPercentage(char* buffer, size_t bufferSize) const;
    
    std::function<void(int32_t value)> m_cb;

public:
    PopupProgress(PixelUI& ui, uint16_t width, uint16_t height, 
                  int32_t& value, int32_t minValue, int32_t maxValue,
                  const char* title = "", uint16_t duration = 3000, uint8_t priority = 0, std::function<void(int32_t value)> cb_function = nullptr);
    ~PopupProgress() = default;

    void drawContent(int16_t centerX, int16_t centerY, int16_t currentWidth, int16_t currentHeight) override;
    bool handleInput(InputEvent event) override;
};
/*
 * Copyright (C) 2025 Lawrence Link
 */

#include "ui/Popup/PopupProgress.h"
#include "PixelUI.h"
#include <cstdio>

PopupProgress::PopupProgress(PixelUI& ui, uint16_t width, uint16_t height, 
                            int32_t& value, int32_t minValue, int32_t maxValue,
                            const char* title, uint16_t duration, uint8_t priority, std::function<void(int32_t value)> cb_function)
    : PopupBase(ui, width, height, priority, duration), 
      _value(value), _minValue(minValue), _maxValue(maxValue), _title(title), m_cb(cb_function)
{
}

void PopupProgress::formatValue(char* buffer, size_t bufferSize) const {
    if (buffer && bufferSize > 0) {
        snprintf(buffer, bufferSize, "%ld/%ld", (long)_value, (long)_maxValue);
    }
}

void PopupProgress::formatValueAsPercentage(char* buffer, size_t bufferSize) const {
    if (!buffer || bufferSize == 0) return;
    
    if (_maxValue > _minValue) {
        float progress = (float)(_value - _minValue) / (float)(_maxValue - _minValue);
        progress = (progress < 0.0f) ? 0.0f : ((progress > 1.0f) ? 1.0f : progress);

        int percentage = (int)(progress * 100.0f + 0.5f);
        
        // This line is technically optional due to the clamping earlier, 
        // but it's good defensive programming for the final output.
        if (percentage > 100) percentage = 100;

        snprintf(buffer, bufferSize, "%d%%", percentage);
    } else {
        snprintf(buffer, bufferSize, "0%%");
    }
}

void PopupProgress::drawContent(int16_t centerX, int16_t centerY, int16_t currentWidth, int16_t currentHeight) {
    U8G2& u8g2 = m_ui.getU8G2();
    
    // title
    if (_title && strlen(_title) > 0) {
        u8g2.setFont(u8g2_font_wqy12_t_gb2312);
        int16_t titleWidth = u8g2.getUTF8Width(_title);
        u8g2.drawUTF8(centerX - titleWidth / 2, centerY - 7, _title);
    }
    
    // draw the progress bar
    int16_t barWidth = currentWidth - 20;
    int16_t barHeight = 8;
    int16_t barX = centerX - barWidth / 2;
    int16_t barY = centerY-3;
    
    // frame of the progress bar
    u8g2.drawFrame(barX, barY, barWidth, barHeight);
    
    // calculate the progress
    if (_maxValue > _minValue) {
        float progress = (float)(_value - _minValue) / (float)(_maxValue - _minValue);
        progress = (progress < 0.0f) ? 0.0f : ((progress > 1.0f) ? 1.0f : progress);
        int16_t fillWidth = (int16_t)(progress * (barWidth - 2));
        
        // fill the progress
        if (fillWidth > 0) {
            u8g2.drawBox(barX + 1, barY + 1, fillWidth, barHeight - 2);
        }
    }
    
    // show percentage
    char percentBuffer[16];
    formatValueAsPercentage(percentBuffer, sizeof(percentBuffer));
    int16_t percentWidth = u8g2.getStrWidth(percentBuffer);
    u8g2.drawStr(centerX - percentWidth / 2, centerY + 17, percentBuffer);
}

bool PopupProgress::handleInput(InputEvent event) {
    // when closing, comsume the event without acting anything.
    if (_state == PopupState::CLOSING) {
        return true;
    }

    switch (event) {
        case InputEvent::RIGHT:
            if (_value < _maxValue) {
                _value++;
                if (m_cb) m_cb(_value);
                // reset the auto-shutoff timer
                _startTime = m_ui.getCurrentTime();
                m_ui.markDirty();
            }
            return true;

        case InputEvent::LEFT:
            if (_value > _minValue) {
                _value--;
                if (m_cb) m_cb(_value);
                // reset the auto-shutoff timer
                _startTime = m_ui.getCurrentTime();
                m_ui.markDirty();
            }
            return true;

        case InputEvent::SELECT:
            startClosingAnimation();
            return true;

        default:
            return PopupBase::handleInput(event);
    }
}
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

#include "ui/Popup/PopupProgress.h"
#include "PixelUI.h"
#include <cstdio>

/**
 * @brief Construct a progress popup
 * @param ui Reference to the PixelUI instance
 * @param width Popup width in pixels
 * @param height Popup height in pixels
 * @param value Reference to the integer value being tracked
 * @param minValue Minimum allowable value
 * @param maxValue Maximum allowable value
 * @param title Optional title string
 * @param duration Display duration in ms
 * @param priority Popup priority for stacking order
 * @param cb_function Optional callback invoked when value changes
 *
 * Initializes a progress popup and links it to a value reference.
 */
PopupProgress::PopupProgress(PixelUI& ui, uint16_t width, uint16_t height, 
                            int32_t& value, int32_t minValue, int32_t maxValue,
                            const char* title, uint16_t duration, uint8_t priority, std::function<void(int32_t value)> cb_function)
    : PopupBase(ui, width, height, priority, duration), 
      _value(value), _minValue(minValue), _maxValue(maxValue), _title(title), m_cb(cb_function)
{
}

/**
 * @brief Format the value as "current/max" string
 * @param buffer Destination buffer
 * @param bufferSize Size of the buffer
 */
void PopupProgress::formatValue(char* buffer, size_t bufferSize) const {
    if (buffer && bufferSize > 0) {
        snprintf(buffer, bufferSize, "%ld/%ld", (long)_value, (long)_maxValue);
    }
}

/**
 * @brief Format the value as a percentage string
 * @param buffer Destination buffer
 * @param bufferSize Size of the buffer
 *
 * Clamps the value between min and max and outputs "0-100%".
 */
void PopupProgress::formatValueAsPercentage(char* buffer, size_t bufferSize) const {
    if (!buffer || bufferSize == 0) return;
    
    if (_maxValue > _minValue) {
        float progress = (float)(_value - _minValue) / (float)(_maxValue - _minValue);
        progress = (progress < 0.0f) ? 0.0f : ((progress > 1.0f) ? 1.0f : progress);

        int percentage = (int)(progress * 100.0f + 0.5f);

        if (percentage > 100) percentage = 100;

        snprintf(buffer, bufferSize, "%d%%", percentage);
    } else {
        snprintf(buffer, bufferSize, "0%%");
    }
}

/**
 * @brief Draw the popup content (title, progress bar, percentage)
 * @param centerX X-coordinate of popup center
 * @param centerY Y-coordinate of popup center
 * @param currentWidth Current width of popup box
 * @param currentHeight Current height of popup box
 *
 * Uses U8G2 to render a horizontal progress bar and optional title.
 * Fills the progress proportionally and displays numeric percentage below.
 */
void PopupProgress::drawContent(int16_t centerX, int16_t centerY, int16_t currentWidth, int16_t currentHeight) {
    U8G2& u8g2 = m_ui.getU8G2();
    
    // draw title centered above the bar
    if (_title && strlen(_title) > 0) {
        u8g2.setFont(u8g2_font_wqy12_t_gb2312);
        int16_t titleWidth = u8g2.getUTF8Width(_title);
        u8g2.drawUTF8(centerX - titleWidth / 2, centerY - 7, _title);
    }
    
    // configure progress bar dimensions
    int16_t barWidth = currentWidth - 20;
    int16_t barHeight = 8;
    int16_t barX = centerX - barWidth / 2;
    int16_t barY = centerY - 3;
    
    // draw the bar frame
    u8g2.drawFrame(barX, barY, barWidth, barHeight);
    
    // calculate and fill the progress
    if (_maxValue > _minValue) {
        float progress = (float)(_value - _minValue) / (float)(_maxValue - _minValue);
        progress = (progress < 0.0f) ? 0.0f : ((progress > 1.0f) ? 1.0f : progress);
        int16_t fillWidth = (int16_t)(progress * (barWidth - 2));
        
        if (fillWidth > 0) {
            u8g2.drawBox(barX + 1, barY + 1, fillWidth, barHeight - 2);
        }
    }
    
    // draw percentage text below the bar
    char percentBuffer[16];
    formatValueAsPercentage(percentBuffer, sizeof(percentBuffer));
    int16_t percentWidth = u8g2.getStrWidth(percentBuffer);
    u8g2.drawStr(centerX - percentWidth / 2, centerY + 17, percentBuffer);
}

/**
 * @brief Handle user input for the progress popup
 * @param event Input event
 * @return true if event is consumed
 *
 * RIGHT/LEFT increments or decrements the value, invoking the callback if set.
 * SELECT triggers the closing animation.
 * Resets the auto-close timer whenever value changes.
 */
bool PopupProgress::handleInput(InputEvent event) {
    if (_state == PopupState::CLOSING) {
        // consume input but do nothing while closing
        return true;
    }

    switch (event) {
        case InputEvent::RIGHT:
            if (_value < _maxValue) {
                _value++;
                if (m_cb) m_cb(_value);
                _startTime = m_ui.getCurrentTime(); // reset auto-close timer
                m_ui.markDirty();                   // mark for redraw
            }
            return true;

        case InputEvent::LEFT:
            if (_value > _minValue) {
                _value--;
                if (m_cb) m_cb(_value);
                _startTime = m_ui.getCurrentTime();
                m_ui.markDirty();
            }
            return true;

        case InputEvent::SELECT:
            startClosingAnimation(); // close popup
            return true;

        default:
            return PopupBase::handleInput(event); // fallback to base
    }
}

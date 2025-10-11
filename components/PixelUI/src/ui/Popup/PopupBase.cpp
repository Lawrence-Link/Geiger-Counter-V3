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

#include "ui/Popup/PopupBase.h"
#include "PixelUI.h"

/**
 * @brief Construct a PopupBase object
 * @param ui Reference to the PixelUI manager
 * @param width Width of the popup
 * @param height Height of the popup
 * @param priority Priority for display layering
 * @param duration Duration in milliseconds before auto-closing (0 = no auto-close)
 *
 * Initializes the popup state to APPEARING and calculates the target box size.
 */
PopupBase::PopupBase(PixelUI& ui, uint16_t width, uint16_t height, uint8_t priority, uint16_t duration)
    : m_ui(ui), _width(width), _height(height), _priority(priority), _duration(duration),
      _startTime(0), _currentBoxSize(0), _state(PopupState::APPEARING)
{
    _targetBoxSize = _width << 12;  // store as fixed-point for animation
}

/**
 * @brief Draw the popup frame and inner box.
 * @param rectX X position of the popup top-left corner
 * @param rectY Y position of the popup top-left corner
 * @param currentWidth Current width (animated)
 * @param currentHeight Current height (animated)
 *
 * Draws outer frame, inner frame, and fills the inner area.
 */
void PopupBase::drawPopupBox(int16_t rectX, int16_t rectY, int16_t currentWidth, int16_t currentHeight) {
    U8G2& u8g2 = m_ui.getU8G2();
    
    u8g2.setDrawColor(1);
    u8g2.drawFrame(rectX + BORDER_OFFSET, rectY + BORDER_OFFSET, currentWidth - 2 * BORDER_OFFSET, currentHeight - 2 * BORDER_OFFSET);
    u8g2.drawFrame(rectX, rectY, currentWidth, currentHeight);
    
    u8g2.setDrawColor(0);
    u8g2.drawBox(rectX + BORDER_WIDTH, rectY + BORDER_WIDTH, 
                 currentWidth - 2 * BORDER_WIDTH, currentHeight - 2 * BORDER_WIDTH);
    
    u8g2.setDrawColor(1);
}

/**
 * @brief Setup clipping for the popup area to avoid drawing outside bounds
 * @param rectX X position of top-left corner
 * @param rectY Y position of top-left corner
 * @param currentWidth Current width of the popup
 * @param currentHeight Current height of the popup
 */
void PopupBase::setupClipWindow(int16_t rectX, int16_t rectY, int16_t currentWidth, int16_t currentHeight) {
    U8G2& u8g2 = m_ui.getU8G2();
    u8g2.setClipWindow(rectX, rectY, rectX + currentWidth, rectY + currentHeight);
}

/**
 * @brief Reset clipping to the full screen
 */
void PopupBase::resetClipWindow() {
    U8G2& u8g2 = m_ui.getU8G2();
    u8g2.setMaxClipWindow();
    u8g2.setDrawColor(1);
}

/**
 * @brief Update the popup state based on current time.
 * @param currentTime Current system time in milliseconds
 * @return true if the popup is still active, false if it has finished closing
 *
 * Manages APPEARING -> SHOWING -> CLOSING transitions and triggers animations
 * using PixelUI's animation system.
 */
bool PopupBase::updateState(uint32_t currentTime) {
    if (_startTime == 0) {
        _startTime = currentTime;
        // Animate the box size from 0 to target with cubic easing, PROTECTED to prevent clearing
        m_ui.animate(_currentBoxSize, _targetBoxSize, 300, EasingType::EASE_OUT_CUBIC, PROTECTION::PROTECTED);
    }
    
    switch (_state) {
        case PopupState::APPEARING: {
            if (_currentBoxSize >= _targetBoxSize) {
                _currentBoxSize = _targetBoxSize;
                _state = PopupState::SHOWING;
                _startTime = currentTime;
            }
            break;
        }
        case PopupState::SHOWING: {
            if (_duration > 0 && currentTime - _startTime >= _duration) {
                startClosingAnimation();
            }
            break;
        }
        case PopupState::CLOSING: {
            m_ui.markDirty();  // request redraw
            if (_currentBoxSize <= 0) {
                _currentBoxSize = 0;
                return false;  // popup finished
            }
            break;
        }
    }
    
    return true;
}

/**
 * @brief Start closing animation
 *
 * Transitions state to CLOSING and animates _currentBoxSize to 0
 * using EASE_IN_CUBIC easing.
 */
void PopupBase::startClosingAnimation() {
    if (_state != PopupState::CLOSING) {
        _state = PopupState::CLOSING;
        m_ui.animate(_currentBoxSize, 0, 300, EasingType::EASE_IN_CUBIC);
    }
}

/**
 * @brief Update the popup
 * @param currentTime Current system time in milliseconds
 * @return true if still active
 *
 * Wrapper around updateState.
 */
bool PopupBase::update(uint32_t currentTime) {
    return updateState(currentTime);
}

/**
 * @brief Handle input events for the popup
 * @param event InputEvent triggered by user
 * @return true if handled
 *
 * Currently closes popup on any input while showing.
 */
bool PopupBase::handleInput(InputEvent event) {
    if (_state == PopupState::SHOWING) {
        startClosingAnimation();
        return true;
    }
    return true;
}

/**
 * @brief Draw the popup on screen
 *
 * Calculates current width/height from animated _currentBoxSize and centers popup.
 * Sets clipping, draws popup frame, and calls drawContent() for derived classes.
 */
void PopupBase::draw() {
    U8G2& u8g2 = m_ui.getU8G2();
    
    int16_t screenWidth = u8g2.getDisplayWidth();
    int16_t screenHeight = u8g2.getDisplayHeight();
    int16_t centerX = screenWidth / 2;
    int16_t centerY = screenHeight / 2;
    
    int16_t currentWidth = _currentBoxSize >> 12; // fixed-point -> pixel
    if (currentWidth <= 0) return;
    
    int16_t currentHeight = (_width > 0) ? (currentWidth * _height) / _width : 0;
    
    int16_t rectX = centerX - currentWidth / 2;
    int16_t rectY = centerY - currentHeight / 2;
    
    setupClipWindow(rectX, rectY, currentWidth, currentHeight);
    drawPopupBox(rectX, rectY, currentWidth, currentHeight);
    drawContent(centerX, centerY, currentWidth, currentHeight);
    resetClipWindow();
}

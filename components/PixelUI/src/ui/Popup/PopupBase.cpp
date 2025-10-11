/*
 * Copyright (C) 2025 Lawrence Link
 */

#include "ui/Popup/PopupBase.h"
#include "PixelUI.h"

PopupBase::PopupBase(PixelUI& ui, uint16_t width, uint16_t height, uint8_t priority, uint16_t duration)
    : m_ui(ui), _width(width), _height(height), _priority(priority), _duration(duration),
      _startTime(0), _currentBoxSize(0), _state(PopupState::APPEARING)
{
    _targetBoxSize = _width << 12;
}

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

void PopupBase::setupClipWindow(int16_t rectX, int16_t rectY, int16_t currentWidth, int16_t currentHeight) {
    U8G2& u8g2 = m_ui.getU8G2();
    u8g2.setClipWindow(rectX, rectY, rectX + currentWidth, rectY + currentHeight);
}

void PopupBase::resetClipWindow() {
    U8G2& u8g2 = m_ui.getU8G2();
    u8g2.setMaxClipWindow();
    u8g2.setDrawColor(1);
}

bool PopupBase::updateState(uint32_t currentTime) {
    if (_startTime == 0) {
        _startTime = currentTime;
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
            m_ui.markDirty();
            if (_currentBoxSize <= 0) {
                _currentBoxSize = 0;
                return false;
            }
            break;
        }
    }
    
    return true;
}

void PopupBase::startClosingAnimation() {
    if (_state != PopupState::CLOSING) {
        _state = PopupState::CLOSING;
        m_ui.animate(_currentBoxSize, 0, 300, EasingType::EASE_IN_CUBIC);
    }
}

bool PopupBase::update(uint32_t currentTime) {
    return updateState(currentTime);
}

bool PopupBase::handleInput(InputEvent event) {
    if (_state == PopupState::SHOWING) {
        startClosingAnimation();
        return true;
    }
    return true;
}

void PopupBase::draw() {
    U8G2& u8g2 = m_ui.getU8G2();
    
    int16_t screenWidth = u8g2.getDisplayWidth();
    int16_t screenHeight = u8g2.getDisplayHeight();
    int16_t centerX = screenWidth / 2;
    int16_t centerY = screenHeight / 2;
    
    int16_t currentWidth = _currentBoxSize >> 12;
    if (currentWidth <= 0) return;
    
    int16_t currentHeight = (_width > 0) ? (currentWidth * _height) / _width : 0;
    
    int16_t rectX = centerX - currentWidth / 2;
    int16_t rectY = centerY - currentHeight / 2;
    
    setupClipWindow(rectX, rectY, currentWidth, currentHeight);
    drawPopupBox(rectX, rectY, currentWidth, currentHeight);
    drawContent(centerX, centerY, currentWidth, currentHeight);
    resetClipWindow();
}
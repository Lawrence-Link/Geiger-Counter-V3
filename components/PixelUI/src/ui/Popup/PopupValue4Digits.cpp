/*
 * Copyright (C) 2025 Lawrence Link
 */

#include "ui/Popup/PopupValue4Digits.h"
#include "PixelUI.h"
#include <cstdio>

PopupValue4Digits::PopupValue4Digits(PixelUI& ui, uint16_t width, uint16_t height, 
                            int32_t& value,
                            const char* title, uint16_t duration, uint8_t priority, std::function<void(int32_t value)> cb_function)
    : PopupBase(ui, width, height, priority, duration), 
      _value(value), _title(title), 
      num_thousands(ui), num_hundreds(ui), num_tens(ui), num_ones(ui), m_focusMan(ui), m_cb(cb_function)
{

    U8G2& u8g2 = m_ui.getU8G2();
    
    int center_x = u8g2.getDisplayWidth() / 2;
    int center_y = u8g2.getDisplayHeight() / 2;

    num_thousands.setPosition(center_x - 2*(2+14), center_y);
    num_thousands.setRange(0,9);
    num_thousands.setSize(14, 16);
    num_thousands.setValue(0);
    num_thousands.setFixedIntDigits(1);

    num_hundreds.setPosition(center_x - 1 - 14, center_y);
    num_hundreds.setRange(0,9);
    num_hundreds.setSize(14, 16);
    num_hundreds.setValue(0);
    num_hundreds.setFixedIntDigits(1);

    num_tens.setPosition(center_x + 1, center_y);
    num_tens.setRange(0,9);
    num_tens.setSize(14, 16);
    num_tens.setValue(0);
    num_tens.setFixedIntDigits(1);

    num_ones.setPosition(center_x + (2+14+2), center_y);
    num_ones.setRange(0,9);
    num_ones.setSize(14, 16);
    num_ones.setValue(0);
    num_ones.setFixedIntDigits(1);

    // num_thousands.setPosition();
    // num_hundreds.setPosition();
    // num_tens.setPosition();
    // num_ones.setPosition();

    m_focusMan.addWidget(&num_thousands);
    m_focusMan.addWidget(&num_hundreds);
    m_focusMan.addWidget(&num_tens);
    m_focusMan.addWidget(&num_ones);

    num_thousands.onLoadNoAnim();
    num_hundreds.onLoadNoAnim();
    num_tens.onLoadNoAnim();
    num_ones.onLoadNoAnim();

}

void PopupValue4Digits::drawContent(int16_t centerX, int16_t centerY, int16_t currentWidth, int16_t currentHeight) {
    U8G2& u8g2 = m_ui.getU8G2();
    
    // title
    if (_title && strlen(_title) > 0) {
        u8g2.setFont(u8g2_font_wqy12_t_gb2312);
        int16_t titleWidth = u8g2.getUTF8Width(_title);
        u8g2.drawUTF8(centerX - titleWidth / 2, centerY - 7, _title);
    }

    num_thousands.draw();
    num_hundreds.draw();
    num_tens.draw();
    num_ones.draw();
    m_focusMan.draw();
    // show percentage
    char percentBuffer[16];
    int16_t percentWidth = u8g2.getStrWidth(percentBuffer);
    u8g2.drawStr(centerX - percentWidth / 2, centerY + 17, percentBuffer);
}

bool PopupValue4Digits::handleInput(InputEvent event) {
    // when closing, comsume the event without acting anything.
    if (_state == PopupState::CLOSING) {
        return true;
    }

    _startTime = m_ui.getCurrentTime();

    IWidget* activeWidget = m_focusMan.getActiveWidget();
        if (activeWidget) {
            // If so, pass the event to the num_scroll widget then update value
            _value = 1000 * num_thousands.getValue()
             + 100 * num_hundreds.getValue()
             + 10 * num_tens.getValue()
             + num_ones.getValue();

            if (activeWidget->handleEvent(event)) {
                // If the widget returns true, it means it has finished processing and control is handed back to the FocusManager
                m_focusMan.clearActiveWidget();
            }
            return true; // Event has been handled, return true
        }
        // No widget has taken over input, execute the original focus management logic
        if (event == InputEvent::BACK) {
            startClosingAnimation();
        } else if (event == InputEvent::RIGHT) {
            m_focusMan.moveNext();
        } else if (event == InputEvent::LEFT) {
            m_focusMan.movePrev();
        } else if (event == InputEvent::SELECT) {
            m_focusMan.selectCurrent();
        }
        return true;
}
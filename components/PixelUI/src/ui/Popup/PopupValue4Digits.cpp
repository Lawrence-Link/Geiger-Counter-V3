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

/*
 * Copyright (C) 2025 Lawrence Link
 *
 * Four-digit numeric popup implementation based on PixelUI.
 * Key features:
 * 1. Four numeric widgets are instantiated on the stack (no dynamic allocation).
 * 2. FocusManager handles left/right focus switching.
 * 3. Input events can directly modify the active digit, synchronizing with _value.
 */

#include "ui/Popup/PopupValue4Digits.h"
#include "PixelUI.h"
#include <cstdio>

// Constructor
PopupValue4Digits::PopupValue4Digits(PixelUI& ui, uint16_t width, uint16_t height, 
                                     int32_t& value,
                                     const char* title, uint16_t duration, uint8_t priority, 
                                     std::function<void(int32_t value)> cb_function)
    : PopupBase(ui, width, height, priority, duration),  // Base class constructor
      _value(value), _title(title), 
      num_thousands(ui), num_hundreds(ui), num_tens(ui), num_ones(ui), // Stack-based numeric widgets
      m_focusMan(ui),  // FocusManager handles focus switching
      m_cb(cb_function) // Optional callback function
{
    m_ui.setContinousDraw(true); // Continuous drawing to avoid flicker
    U8G2& u8g2 = m_ui.getU8G2();
    
    // Calculate screen center
    int center_x = u8g2.getDisplayWidth() / 2;
    int center_y = u8g2.getDisplayHeight() / 2;

    // ---------- Initialize positions and properties of the four digit widgets ----------
    num_thousands.setPosition(center_x - 2*(2+14), center_y); // Thousands digit
    num_thousands.setRange(0,9);
    num_thousands.setSize(14, 16);
    num_thousands.setValue(0);
    num_thousands.setFixedIntDigits(1);

    num_hundreds.setPosition(center_x - 1 - 14, center_y); // Hundreds digit
    num_hundreds.setRange(0,9);
    num_hundreds.setSize(14, 16);
    num_hundreds.setValue(0);
    num_hundreds.setFixedIntDigits(1);

    num_tens.setPosition(center_x + 1, center_y); // Tens digit
    num_tens.setRange(0,9);
    num_tens.setSize(14, 16);
    num_tens.setValue(0);
    num_tens.setFixedIntDigits(1);

    num_ones.setPosition(center_x + (2+14+2), center_y); // Ones digit
    num_ones.setRange(0,9);
    num_ones.setSize(14, 16);
    num_ones.setValue(0);
    num_ones.setFixedIntDigits(1);

    // ---------- Register widgets to the FocusManager ----------
    m_focusMan.addWidget(&num_thousands);
    m_focusMan.addWidget(&num_hundreds);
    m_focusMan.addWidget(&num_tens);
    m_focusMan.addWidget(&num_ones);

    // ---------- Load widgets immediately without animation ----------
    num_thousands.onLoadNoAnim();
    num_hundreds.onLoadNoAnim();
    num_tens.onLoadNoAnim();
    num_ones.onLoadNoAnim();
}

// Draw popup content
void PopupValue4Digits::drawContent(int16_t centerX, int16_t centerY, int16_t currentWidth, int16_t currentHeight) {
    U8G2& u8g2 = m_ui.getU8G2();
    
    // Draw title
    if (_title && strlen(_title) > 0) {
        u8g2.setFont(u8g2_font_wqy12_t_gb2312);
        int16_t titleWidth = u8g2.getUTF8Width(_title);
        u8g2.drawUTF8(centerX - titleWidth / 2, centerY - 7, _title);
    }

    // Draw the four numeric widgets
    num_thousands.draw();
    num_hundreds.draw();
    num_tens.draw();
    num_ones.draw();

    // Draw focus highlight via FocusManager
    m_focusMan.draw();

    // Optional: show percentage or unit (currently placeholder)
    char percentBuffer[16];
    int16_t percentWidth = u8g2.getStrWidth(percentBuffer);
    u8g2.drawStr(centerX - percentWidth / 2, centerY + 17, percentBuffer);
}

// Handle input events
bool PopupValue4Digits::handleInput(InputEvent event) {
    // Consume events during closing
    if (_state == PopupState::CLOSING) {
        return true;
    }

    // Reset auto-close timer
    _startTime = m_ui.getCurrentTime();

    // ---------- Priority: pass input to the currently active widget ----------
    IWidget* activeWidget = m_focusMan.getActiveWidget();
    if (activeWidget) {
        // Merge values from four digit widgets into _value
        _value = 1000 * num_thousands.getValue()
               + 100 * num_hundreds.getValue()
               + 10 * num_tens.getValue()
               + num_ones.getValue();

        // If the widget handled the event, release focus
        if (activeWidget->handleEvent(event)) {
            m_focusMan.clearActiveWidget();
        }
        return true; // Event handled
    }

    // ---------- If no widget is active, use FocusManager for navigation ----------
    if (event == InputEvent::BACK) {
        startClosingAnimation(); // Back key closes popup
    } else if (event == InputEvent::RIGHT) {
        m_focusMan.moveNext(); // Move focus to the right digit
    } else if (event == InputEvent::LEFT) {
        m_focusMan.movePrev(); // Move focus to the left digit
    } else if (event == InputEvent::SELECT) {
        m_focusMan.selectCurrent(); // Activate the currently focused digit
    }
    return true;
}

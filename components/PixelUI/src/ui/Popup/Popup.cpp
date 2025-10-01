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

#include "ui/Popup/Popup.h"
#include "PixelUI.h"
#include <cstring>
#include <algorithm>

// --- PopupBase Class Implementation ---

/**
 * @brief Constructs a base popup object.
 *
 * @param ui The main PixelUI instance.
 * @param width The width of the popup.
 * @param height The height of the popup.
 * @param priority The drawing priority, higher values are drawn on top.
 * @param duration The display duration in milliseconds before auto-closing.
 */
PopupBase::PopupBase(PixelUI& ui, uint16_t width, uint16_t height, uint8_t priority, uint16_t duration)
    : m_ui(ui), _width(width), _height(height), _priority(priority), _duration(duration),
      _startTime(0), _currentBoxSize(0), _state(PopupState::APPEARING)
{
    // The target box size is a fixed-point representation for smoother animation.
    // This value is 12 bits left-shifted.
    _targetBoxSize = _width << 12;
}

/**
 * @brief Draws the popup box frame with a double border.
 *
 * @param rectX The x-coordinate of the top-left corner.
 * @param rectY The y-coordinate of the top-left corner.
 * @param currentWidth The current width of the box.
 * @param currentHeight The current height of the box.
 */
void PopupBase::drawPopupBox(int16_t rectX, int16_t rectY, int16_t currentWidth, int16_t currentHeight) {
    U8G2& u8g2 = m_ui.getU8G2();
    
    // Draw the popup box with a double-border effect.
    u8g2.setDrawColor(1); // Set draw color to white.
    u8g2.drawFrame(rectX + BORDER_OFFSET, rectY + BORDER_OFFSET, currentWidth - 2 * BORDER_OFFSET, currentHeight - 2 * BORDER_OFFSET);
    u8g2.drawFrame(rectX, rectY, currentWidth, currentHeight);
    
    // Fill the inside of the box with black.
    u8g2.setDrawColor(0); // Set draw color to black.
    u8g2.drawBox(rectX + BORDER_WIDTH, rectY + BORDER_WIDTH, 
                 currentWidth - 2 * BORDER_WIDTH, currentHeight - 2 * BORDER_WIDTH);
    
    // Reset draw color to white for content drawing
    u8g2.setDrawColor(1);
}

/**
 * @brief Sets up a clipping window to restrict drawing to the popup's bounds.
 */
void PopupBase::setupClipWindow(int16_t rectX, int16_t rectY, int16_t currentWidth, int16_t currentHeight) {
    U8G2& u8g2 = m_ui.getU8G2();
    u8g2.setClipWindow(rectX, rectY, rectX + currentWidth, rectY + currentHeight);
}

/**
 * @brief Resets the clipping window to the full display area.
 */
void PopupBase::resetClipWindow() {
    U8G2& u8g2 = m_ui.getU8G2();
    u8g2.setMaxClipWindow();
    u8g2.setDrawColor(1); // Reset draw color to default.
}

/**
 * @brief Updates the popup's state based on time.
 * @return True if the popup is still active, false if it should be removed.
 */
bool PopupBase::updateState(uint32_t currentTime) {
    if (_startTime == 0) {
        _startTime = currentTime;
        // Start the appearing animation.
        m_ui.animate(_currentBoxSize, _targetBoxSize, 300, EasingType::EASE_OUT_CUBIC);
    }
    
    switch (_state) {
        case PopupState::APPEARING: {
            if (_currentBoxSize >= _targetBoxSize) {
                _currentBoxSize = _targetBoxSize;
                _state = PopupState::SHOWING;
                _startTime = currentTime; // Reset timer for showing phase
            }
            break;
        }
        case PopupState::SHOWING: {
            // Check if display time exceeded (auto close)
            if (_duration > 0 && currentTime - _startTime >= _duration) {
                startClosingAnimation();
            }
            break;
        }
        case PopupState::CLOSING: {
            if (_currentBoxSize <= 0) {
                _currentBoxSize = 0;
                return false; // Popup finished, remove from manager
            }
            break;
        }
    }
    
    return true; // The popup is still active.
}

/**
 * @brief Initiates the animation to close the popup.
 */
void PopupBase::startClosingAnimation() {
    if (_state != PopupState::CLOSING) {
        _state = PopupState::CLOSING;
        // Start the disappearing animation.
        m_ui.animate(_currentBoxSize, 0, 300, EasingType::EASE_IN_CUBIC);
    }
}

/**
 * @brief Main update method called by the PopupManager.
 * @return True if the popup is still active, false if it should be removed.
 */
bool PopupBase::update(uint32_t currentTime) {
    return updateState(currentTime);
}

/**
 * @brief Handles input events.
 *
 * The base class implementation starts the closing animation on any input.
 * Subclasses can override this for custom behavior.
 * @return True if the event was handled, false otherwise.
 */
bool PopupBase::handleInput(InputEvent event) {
    startClosingAnimation();
    return true;
}

/**
 * @brief The main draw method for the popup.
 *
 * It handles the animation and frame drawing, then calls the subclass's
 * drawContent method.
 */
void PopupBase::draw() {
    U8G2& u8g2 = m_ui.getU8G2();
    
    int16_t screenWidth = u8g2.getDisplayWidth();
    int16_t screenHeight = u8g2.getDisplayHeight();
    int16_t centerX = screenWidth / 2;
    int16_t centerY = screenHeight / 2;
    
    // Get the current dimensions from the animated size.
    int16_t currentWidth = _currentBoxSize >> 12;
    if (currentWidth <= 0) return;
    
    // Maintain the aspect ratio during animation.
    int16_t currentHeight = (_width > 0) ? (currentWidth * _height) / _width : 0;
    
    // Calculate the top-left corner of the popup box.
    int16_t rectX = centerX - currentWidth / 2;
    int16_t rectY = centerY - currentHeight / 2;
    
    // Use a clip window to prevent drawing outside the popup's bounds during animation.
    setupClipWindow(rectX, rectY, currentWidth, currentHeight);
    
    // Draw the popup box
    drawPopupBox(rectX, rectY, currentWidth, currentHeight);
    
    // Let subclass draw its content
    drawContent(centerX, centerY, currentWidth, currentHeight);
    
    // Reset the clip window after drawing is complete.
    resetClipWindow();
}

// --- PopupInfo Class Implementation ---

/**
 * @brief Constructs an information popup.
 *
 * @param ui The main PixelUI instance.
 * @param width The desired width of the popup.
 * @param height The desired height of the popup.
 * @param text The text to display.
 * @param title The optional title for the popup.
 * @param duration The display duration.
 * @param priority The drawing priority.
 */
PopupInfo::PopupInfo(PixelUI& ui, uint16_t width, uint16_t height, 
                     const char* text, const char* title, uint16_t duration, uint8_t priority)
    : PopupBase(ui, width, height, priority, duration), _title(title), _text(text), _lineCount(0)
{
    // Split text into lines and calculate actual height needed
    if (_text) {
        _lineCount = splitTextIntoLines(_text, _width - 2 * TEXT_MARGIN);
        _actualHeight = _lineCount * LINE_HEIGHT + 2 * TEXT_MARGIN;
        
        // Update height if calculated height is different
        if (_actualHeight > _height) {
            _height = _actualHeight;
            _targetBoxSize = _width << 12; // Recalculate target size with new height
        }
    } else {
        _actualHeight = _height;
    }
}

/**
 * @brief Splits a string into multiple lines to fit within a maximum width.
 * @return The number of lines.
 */
uint16_t PopupInfo::splitTextIntoLines(const char* text, uint16_t maxWidth) {
    if (!text) return 0;
    
    U8G2& u8g2 = m_ui.getU8G2();
    u8g2.setFont(u8g2_font_5x7_tr);
    
    const char* current = text;
    uint16_t lineIndex = 0;
    
    // ESP32-C6 optimization: add max processing length limit to prevent infinite loops
    const char* textEnd = text + strnlen(text, 512); // Process up to 512 characters
    
    while (*current && current < textEnd && lineIndex < MAX_LINES) {
        const char* lineStart = current;
        const char* lastSpace = nullptr;
        const char* lineEnd = current;
        uint16_t currentLineWidth = 0;

        // Find the best break point for this line
        while (*lineEnd && lineEnd < textEnd) {
            // Simple width estimation for performance
            currentLineWidth += 6; // Assume avg 6 pixels per char for 5x7 font

            if (currentLineWidth > maxWidth) {
                break; // Line is too long
            }

            if (*lineEnd == ' ') {
                lastSpace = lineEnd;
            }
            if (*lineEnd == '\n') {
                lineEnd++;
                break;
            }
            lineEnd++;
        }

        if (currentLineWidth > maxWidth && lastSpace > lineStart) {
            lineEnd = lastSpace + 1;
        }
        
        // Store line information
        _textLines[lineIndex].start = lineStart;
        _textLines[lineIndex].length = lineEnd - lineStart;
        
        // Move to the start of the next line
        current = lineEnd;
        while (*current == ' ' || *current == '\n') {
            current++;
        }
        
        lineIndex++;
    }
    
    return lineIndex;
}

/**
 * @brief Draws the text content inside the popup box.
 */
void PopupInfo::drawContent(int16_t centerX, int16_t centerY, int16_t currentWidth, int16_t currentHeight) {
    // Draw the wrapped text inside the popup.
    if (_text && _lineCount > 0) {
        U8G2& u8g2 = m_ui.getU8G2();
        u8g2.setFont(u8g2_font_5x7_tr);
        
        int16_t textAreaHeight = _lineCount * LINE_HEIGHT;
        int16_t textStartY = centerY - textAreaHeight / 2 + LINE_HEIGHT - 2;
        
        for (uint16_t i = 0; i < _lineCount; i++) {
            if (_textLines[i].length > 0) {
                constexpr size_t MAX_LINE_BUFFER = 64; // Limit single line to 64 chars
                char lineStr[MAX_LINE_BUFFER];
                
                size_t copyLength = std::min((size_t)_textLines[i].length, MAX_LINE_BUFFER - 1);
                strncpy(lineStr, _textLines[i].start, copyLength);
                lineStr[copyLength] = '\0';
                
                // Center the line horizontally.
                int16_t lineWidth = u8g2.getStrWidth(lineStr);
                int16_t lineX = centerX - lineWidth / 2;
                int16_t lineY = textStartY + (i * LINE_HEIGHT);
                
                // Boundary check: ensure text is within the display area
                if (lineY > 0 && lineY < u8g2.getDisplayHeight()) {
                    u8g2.drawStr(lineX, lineY, lineStr);
                }
            }
        }
    }
}

// --- PopupProgress Class Implementation ---

/**
 * @brief Constructs a progress bar popup.
 *
 * @param ui The main PixelUI instance.
 * @param width The desired width.
 * @param height The desired height.
 * @param value Reference to the value to display.
 * @param minValue The minimum value of the progress bar.
 * @param maxValue The maximum value of the progress bar.
 * @param title The optional title.
 * @param duration The display duration.
 * @param priority The drawing priority.
 */
PopupProgress::PopupProgress(PixelUI& ui, uint16_t width, uint16_t height,
                             int32_t& value, int32_t minValue, int32_t maxValue,
                             const char* title, uint16_t duration, uint8_t priority)
    : PopupBase(ui, width, height, priority, duration), _value(value), _minValue(minValue),
      _maxValue(maxValue), _title(title)
{
    // Only do necessary safety checks, don't force change user parameters
    if (_minValue >= _maxValue) {
        _maxValue = _minValue + 1; // This is a necessary safety check
    }
    
    // Keep value in valid range (this is safe)
    _value = std::max(_minValue, std::min(_value, _maxValue));
}

/**
 * @brief Handles input for the progress bar, allowing value changes.
 * @return True if the event was handled, false otherwise.
 */
bool PopupProgress::handleInput(InputEvent event) {
    if (_state == PopupState::CLOSING) {
        return true; // If already closing, ignore any input.
    }

    switch (event) {
        case InputEvent::RIGHT:
            if (_value < _maxValue) {
                _value++;
                // User interaction resets the auto-close timer.
                _startTime = m_ui.getCurrentTime();
                m_ui.markDirty(); // Mark UI for redraw to show the new value.
            }
            return true; // Consume this event, prevent it from passing to other components.

        case InputEvent::LEFT:
            if (_value > _minValue) {
                _value--;
                // User interaction resets the auto-close timer.
                _startTime = m_ui.getCurrentTime();
                m_ui.markDirty(); // Mark UI for redraw.
            }
            return true; // Consume this event.

        case InputEvent::SELECT:
            startClosingAnimation(); // Use the base class's close animation function.
            return true; // Consume this event.

        default:
            return false; // For other keys, don't handle and allow them to pass.
    }
}

/**
 * @brief Draws the progress bar and associated text content.
 */
void PopupProgress::drawContent(int16_t centerX, int16_t centerY, int16_t currentWidth, int16_t currentHeight) {
    U8G2& u8g2 = m_ui.getU8G2();
    u8g2.setFont(u8g2_font_5x7_tr);
    
    // ESP32-C6 optimization: use a small buffer on the stack
    constexpr size_t BUFFER_SIZE = 32;
    
    int16_t availableHeight = currentHeight - 8; // Inner space
    int16_t currentY = centerY - availableHeight / 2;

    // Draw title if there's space and title exists
    if (_title && strlen(_title) > 0 && availableHeight >= 9) {
        int16_t titleWidth = u8g2.getStrWidth(_title);
        u8g2.drawStr(centerX - titleWidth / 2, currentY + 7, _title);
        currentY += 11;
        availableHeight -= 11;
    }
    
    // Draw progress bar if there's space
    if (availableHeight >= 10 && currentWidth > 12) {
        int16_t barWidth = currentWidth - 12;
        int16_t barX = centerX - barWidth / 2;
        int16_t barY = currentY;
        int16_t progressBarHeight = 8;
        
        u8g2.drawFrame(barX, barY, barWidth, progressBarHeight);
        
        if (_maxValue > _minValue) {
            float progress = (float)(_value - _minValue) / (float)(_maxValue - _minValue);
            progress = std::max(0.0f, std::min(1.0f, progress)); // Clamp progress
            int16_t fillWidth = (int16_t)(progress * (barWidth - 2));
            if (fillWidth > 0) {
                u8g2.drawBox(barX + 1, barY + 1, fillWidth, progressBarHeight - 2);
            }
        }
        currentY += 12;
        availableHeight -= 12;
    }
    
    // Draw value text if there's space
    if (availableHeight >= 9) {
        char valueStr[BUFFER_SIZE];
        // Format the value as a percentage.
        formatValueAsPercentage(valueStr, sizeof(valueStr)); 
        int16_t valueWidth = u8g2.getStrWidth(valueStr);
        u8g2.drawStr(centerX - valueWidth / 2, currentY + 7, valueStr);
    }
}

// --- PopupManager Class Implementation ---

/**
 * @brief Adds a new popup to the manager, handling priority-based sorting.
 */
void PopupManager::addPopup(std::shared_ptr<IPopup> popup) {
    if (!popup) return;
    
    // Check if maximum popup limit reached
    if (_popups.size() >= _popups.max_size()) {
        // Remove lowest priority popup to make space for new popup
        if (!_popups.empty()) {
            auto minPriorityIt = _popups.begin();
            for (auto it = _popups.begin(); it != _popups.end(); ++it) {
                if ((*it)->getPriority() < (*minPriorityIt)->getPriority()) {
                    minPriorityIt = it;
                }
            }
            _popups.erase(minPriorityIt);
        }
    }
    
    // Find the correct insertion point to maintain a sorted list by priority (highest first).
    auto insertPos = _popups.begin();
    for (; insertPos != _popups.end(); ++insertPos) {
        if ((*insertPos)->getPriority() < popup->getPriority()) {
            break;
        }
    }
    _popups.insert(insertPos, popup);
}

/**
 * @brief Removes a specific popup from the manager.
 */
void PopupManager::removePopup(std::shared_ptr<IPopup> popup) {
    if (!popup) return;
    
    auto it = std::find(_popups.begin(), _popups.end(), popup);
    if (it != _popups.end()) {
        _popups.erase(it);
    }
}

/**
 * @brief Clears all popups from the manager.
 */
void PopupManager::clearPopups() {
    _popups.clear();
}

/**
 * @brief Draws all popups, from lowest to highest priority.
 */
void PopupManager::drawPopups() {
    // Draw from lowest priority to highest, so highest is on top
    for (auto it = _popups.rbegin(); it != _popups.rend(); ++it) {
        if (*it) {
            (*it)->draw();
        }
    }
}

/**
 * @brief Updates the state of all active popups.
 */
void PopupManager::updatePopups(uint32_t currentTime) {
    if (_popups.empty()) {
        return;
    }

    // Use a safer iteration method to allow for removal during iteration
    auto it = _popups.begin();
    while (it != _popups.end()) {
        if (*it && (*it)->update(currentTime)) {
            ++it;
        } else {
            it = _popups.erase(it);
        }
    }
}

/**
 * @brief Handles input for the highest priority popup.
 * @return True if the input was handled, false otherwise.
 */
bool PopupManager::handleTopPopupInput(InputEvent event) {
    if (_popups.empty()) return false;
    
    // The highest priority popup is at the front of the sorted vector
    return _popups.front()->handleInput(event);
}
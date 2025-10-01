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

#pragma once

#include <functional>
#include <cstdint>
#include "core/CommonTypes.h"
#include "etl/vector.h"
#include "config.h"

class PixelUI;

/**
 * @class IPopup
 * @brief An interface for all popup types.
 *
 * Defines the essential methods for a popup to be managed by the PopupManager.
 */
class IPopup{
public:
    IPopup() = default;
    virtual ~IPopup() {}

    /**
     * @brief Updates the popup's state.
     * @return True if the popup is still active, false otherwise.
     */
    virtual bool update(uint32_t currentTime) = 0;
    
    /**
     * @brief Draws the popup to the display.
     */
    virtual void draw() = 0;
    
    /**
     * @brief Handles input events.
     * @return True if the event was consumed, false to pass it to other handlers.
     */
    virtual bool handleInput(InputEvent event) = 0;
    
    /**
     * @brief Gets the priority of the popup for drawing and input handling order.
     * @return The priority value.
     */
    virtual uint8_t getPriority() const = 0;
    
    /**
     * @brief Gets the duration of the popup.
     * @return The duration in milliseconds.
     */
    virtual uint16_t getDuration() const = 0;
};

/**
 * @enum PopupState
 * @brief The state of a popup's animation.
 */
enum class PopupState {
    APPEARING, // Popup is expanding
    SHOWING,   // Popup is displayed statically
    CLOSING    // Popup is shrinking
};

/**
 * @class PopupBase
 * @brief Base class for all popups, providing common functionality.
 */
class PopupBase : public IPopup {
protected:
    PixelUI& m_ui;    
    uint16_t _width, _height;
    uint8_t _priority;
    uint16_t _duration;
    uint32_t _startTime;
    int32_t _currentBoxSize;
    int32_t _targetBoxSize;
    
    PopupState _state;

    // Common drawing parameters
    static const int16_t BORDER_OFFSET = 2;
    static const int16_t BORDER_WIDTH = 1;

    // Common drawing methods
    void drawPopupBox(int16_t rectX, int16_t rectY, int16_t currentWidth, int16_t currentHeight);
    void setupClipWindow(int16_t rectX, int16_t rectY, int16_t currentWidth, int16_t currentHeight);
    void resetClipWindow();
    
    // Common animation and state management
    virtual bool updateState(uint32_t currentTime);
    void startClosingAnimation();

public:
    PopupBase(PixelUI& ui, uint16_t width, uint16_t height, uint8_t priority, uint16_t duration);
    virtual ~PopupBase() = default;
    
    uint8_t getPriority() const override { return _priority; }
    uint16_t getDuration() const override { return _duration; }
    bool update(uint32_t currentTime) override;
    void draw() override;
    bool handleInput(InputEvent event) override;
    
    // Abstract method for subclasses to implement their specific content drawing
    virtual void drawContent(int16_t centerX, int16_t centerY, int16_t currentWidth, int16_t currentHeight) = 0;
};

/**
 * @class PopupInfo
 * @brief A popup displaying informational text.
 */
class PopupInfo : public PopupBase {
private:
    uint16_t _actualHeight;  // The calculated height based on text content
    const char *_title;
    const char *_text;
    
    // ESP32-C6 memory-optimized text handling
    static const uint16_t MAX_LINES = 6;  // Reduced from 8 to 6 to save memory
    static const uint16_t LINE_HEIGHT = 9; // Line height in pixels
    static const uint16_t TEXT_MARGIN = 4; // Reduced from 6 to 4 to save space
    
    struct TextLine {
        const char* start;
        uint16_t length;
    };
    
    TextLine _textLines[MAX_LINES];
    uint16_t _lineCount;

    uint16_t splitTextIntoLines(const char* text, uint16_t maxWidth);
    
public:
    PopupInfo(PixelUI& ui, uint16_t width, uint16_t height, 
              const char* text, const char* title = "", uint16_t duration = 3000, uint8_t priority = 0);
    ~PopupInfo() = default;
    
    void drawContent(int16_t centerX, int16_t centerY, int16_t currentWidth, int16_t currentHeight) override;
};

/**
 * @class PopupManager
 * @brief Manages the lifecycle, drawing, and input of multiple popups.
 */
class PopupManager {
private:
    // A vector to store popups, sorted by priority.
    etl::vector<std::shared_ptr<IPopup>, MAX_POPUP_NUM> _popups;
    PixelUI& m_ui;
    
public:
    PopupManager(PixelUI& ui) : m_ui(ui) {}
    ~PopupManager() = default;

    void addPopup(std::shared_ptr<IPopup> popup);
    void removePopup(std::shared_ptr<IPopup> popup);
    void clearPopups();
    void drawPopups();
    void updatePopups(uint32_t currentTime);
    bool handleTopPopupInput(InputEvent event);
    size_t getPopupCounts() const { return _popups.size(); }
};

/**
 * @class PopupProgress
 * @brief A popup displaying a progress bar.
 */
class PopupProgress : public PopupBase {
private:
    int32_t& _value;
    int32_t _minValue, _maxValue;
    const char* _title;
    
    /**
     * @brief Formats the value as "current/max".
     */
    void formatValue(char* buffer, size_t bufferSize) const {
        // Safe formatting
        if (buffer && bufferSize > 0) {
            snprintf(buffer, bufferSize, "%ld/%ld", (long)_value, (long)_maxValue);
        }
    }

    /**
     * @brief Formats the value as a percentage.
     */
    void formatValueAsPercentage(char* buffer, size_t bufferSize) const {
        if (!buffer || bufferSize == 0) return;
        
        if (_maxValue > _minValue) {
            float progress = (float)(_value - _minValue) / (float)(_maxValue - _minValue);
            // Ensure progress is within a valid range.
            progress = (progress < 0.0f) ? 0.0f : ((progress > 1.0f) ? 1.0f : progress);
            int percentage = (int)(progress * 100.0f);
            snprintf(buffer, bufferSize, "%d%%", percentage);
        } else {
            snprintf(buffer, bufferSize, "0%%");
        }
    }

public:
    PopupProgress(PixelUI& ui, uint16_t width, uint16_t height, 
                  int32_t& value, int32_t minValue, int32_t maxValue,
                  const char* title = "", uint16_t duration = 3000, uint8_t priority = 0);
    ~PopupProgress() = default;

    void drawContent(int16_t centerX, int16_t centerY, int16_t currentWidth, int16_t currentHeight) override;
    bool handleInput(InputEvent event) override;
};
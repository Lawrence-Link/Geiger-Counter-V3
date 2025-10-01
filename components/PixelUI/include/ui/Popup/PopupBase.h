/*
 * Copyright (C) 2025 Lawrence Link
 */

#pragma once

#include "PopupInterface.h"

class PixelUI;

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
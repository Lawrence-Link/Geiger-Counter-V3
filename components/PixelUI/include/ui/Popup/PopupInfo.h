/*
 * Copyright (C) 2025 Lawrence Link
 */

#pragma once

#include "PopupBase.h"

/**
 * @class PopupInfo
 * @brief A popup displaying informational text.
 */
class PopupInfo : public PopupBase {
private:
    uint16_t _actualHeight;
    const char *_title;
    const char *_text;
    
    static const uint16_t MAX_LINES = 6;
    static const uint16_t LINE_HEIGHT = 9;
    static const uint16_t TEXT_MARGIN = 4;
    
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
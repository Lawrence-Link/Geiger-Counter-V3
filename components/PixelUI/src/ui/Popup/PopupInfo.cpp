/*
 * Copyright (C) 2025 Lawrence Link
 */

#include "ui/Popup/PopupInfo.h"
#include "PixelUI.h"
#include <cstring>
#include <algorithm>

PopupInfo::PopupInfo(PixelUI& ui, uint16_t width, uint16_t height, 
                     const char* text, const char* title, uint16_t duration, uint8_t priority)
    : PopupBase(ui, width, height, priority, duration), _title(title), _text(text), _lineCount(0)
{
    if (_text) {
        _lineCount = splitTextIntoLines(_text, _width - 2 * TEXT_MARGIN);
        _actualHeight = _lineCount * LINE_HEIGHT + 2 * TEXT_MARGIN;
        
        if (_actualHeight > _height) {
            _height = _actualHeight;
            _targetBoxSize = _width << 12;
        }
    } else {
        _actualHeight = _height;
    }
}

uint16_t PopupInfo::splitTextIntoLines(const char* text, uint16_t maxWidth) {
    if (!text) return 0;
    
    U8G2& u8g2 = m_ui.getU8G2();
    u8g2.setFont(u8g2_font_5x7_tr);
    
    const char* current = text;
    uint16_t lineIndex = 0;
    
    const char* textEnd = text + strnlen(text, 512);
    
    while (*current && current < textEnd && lineIndex < MAX_LINES) {
        const char* lineStart = current;
        const char* lastSpace = nullptr;
        const char* lineEnd = current;
        uint16_t currentLineWidth = 0;

        while (*lineEnd && lineEnd < textEnd) {
            currentLineWidth += 6;

            if (currentLineWidth > maxWidth) {
                break;
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
        
        _textLines[lineIndex].start = lineStart;
        _textLines[lineIndex].length = lineEnd - lineStart;
        
        current = lineEnd;
        while (*current == ' ' || *current == '\n') {
            current++;
        }
        
        lineIndex++;
    }
    
    return lineIndex;
}

void PopupInfo::drawContent(int16_t centerX, int16_t centerY, int16_t currentWidth, int16_t currentHeight) {
    if (_text && _lineCount > 0) {
        U8G2& u8g2 = m_ui.getU8G2();
        u8g2.setFont(u8g2_font_5x7_tr);
        
        int16_t textAreaHeight = _lineCount * LINE_HEIGHT;
        int16_t textStartY = centerY - textAreaHeight / 2 + LINE_HEIGHT - 2;
        
        for (uint16_t i = 0; i < _lineCount; i++) {
            if (_textLines[i].length > 0) {
                constexpr size_t MAX_LINE_BUFFER = 64;
                char lineBuffer[MAX_LINE_BUFFER];
                size_t copyLength = std::min((size_t)_textLines[i].length, MAX_LINE_BUFFER - 1);
                strncpy(lineBuffer, _textLines[i].start, copyLength);
                lineBuffer[copyLength] = '\0';
                
                int16_t lineWidth = u8g2.getStrWidth(lineBuffer);
                int16_t lineX = centerX - lineWidth / 2;
                int16_t lineY = textStartY + i * LINE_HEIGHT;
                
                u8g2.drawStr(lineX, lineY, lineBuffer);
            }
        }
    }
}
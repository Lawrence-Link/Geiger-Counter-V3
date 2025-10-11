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

#include "ui/Popup/PopupInfo.h"
#include "PixelUI.h"
#include <cstring>
#include <algorithm>

/**
 * @brief Construct a PopupInfo object
 * @param ui Reference to PixelUI manager
 * @param width Popup width in pixels
 * @param height Popup height in pixels
 * @param text Pointer to the text to display
 * @param title Optional title string
 * @param duration Time in milliseconds before auto-close (0 = no auto-close)
 * @param priority Display priority
 *
 * Initializes the base popup, splits the text into lines for rendering,
 * and calculates the actual height required. Updates _height and _targetBoxSize
 * if content exceeds initial popup size.
 */
PopupInfo::PopupInfo(PixelUI& ui, uint16_t width, uint16_t height, 
                     const char* text, const char* title, uint16_t duration, uint8_t priority)
    : PopupBase(ui, width, height, priority, duration), _title(title), _text(text), _lineCount(0)
{
    if (_text) {
        // Split the text into lines based on available width
        _lineCount = splitTextIntoLines(_text, _width - 2 * TEXT_MARGIN);
        _actualHeight = _lineCount * LINE_HEIGHT + 2 * TEXT_MARGIN;

        // Adjust height if content exceeds initial height
        if (_actualHeight > _height) {
            _height = _actualHeight;
            _targetBoxSize = _width << 12;  // fixed-point for animation
        }
    } else {
        _actualHeight = _height;
    }
}

/**
 * @brief Split a text string into lines for popup rendering
 * @param text Input text
 * @param maxWidth Maximum width in pixels
 * @return Number of lines generated
 *
 * Uses a fixed font width (6px for 5x7) to determine line breaks.
 * Supports word wrapping at spaces and newlines.
 * Stores line start pointer and length in _textLines array.
 */
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

        // Iterate through characters to determine line break
        while (*lineEnd && lineEnd < textEnd) {
            currentLineWidth += 6;  // font width per char

            if (currentLineWidth > maxWidth) break;
            if (*lineEnd == ' ') lastSpace = lineEnd;
            if (*lineEnd == '\n') { lineEnd++; break; }
            lineEnd++;
        }

        // Wrap at last space if exceeded maxWidth
        if (currentLineWidth > maxWidth && lastSpace > lineStart) {
            lineEnd = lastSpace + 1;
        }

        _textLines[lineIndex].start = lineStart;
        _textLines[lineIndex].length = lineEnd - lineStart;

        // Skip spaces/newlines for next line
        current = lineEnd;
        while (*current == ' ' || *current == '\n') current++;

        lineIndex++;
    }

    return lineIndex;
}

/**
 * @brief Draw the content of the popup (text lines)
 * @param centerX X coordinate of popup center
 * @param centerY Y coordinate of popup center
 * @param currentWidth Current width of the animated popup box
 * @param currentHeight Current height of the animated popup box
 *
 * Handles text centering horizontally and vertically.
 * Copies each line to a temporary buffer before drawing with U8G2.
 * Uses fixed LINE_HEIGHT to calculate vertical positions.
 */
void PopupInfo::drawContent(int16_t centerX, int16_t centerY, int16_t currentWidth, int16_t currentHeight) {
    if (_text && _lineCount > 0) {
        U8G2& u8g2 = m_ui.getU8G2();
        u8g2.setFont(u8g2_font_5x7_tr);

        int16_t textAreaHeight = _lineCount * LINE_HEIGHT;
        int16_t textStartY = centerY - textAreaHeight / 2 + LINE_HEIGHT - 2;  // vertical centering

        for (uint16_t i = 0; i < _lineCount; i++) {
            if (_textLines[i].length > 0) {
                constexpr size_t MAX_LINE_BUFFER = 64;
                char lineBuffer[MAX_LINE_BUFFER];
                size_t copyLength = std::min((size_t)_textLines[i].length, MAX_LINE_BUFFER - 1);
                strncpy(lineBuffer, _textLines[i].start, copyLength);
                lineBuffer[copyLength] = '\0';

                int16_t lineWidth = u8g2.getStrWidth(lineBuffer);
                int16_t lineX = centerX - lineWidth / 2;  // horizontal centering
                int16_t lineY = textStartY + i * LINE_HEIGHT;

                u8g2.drawStr(lineX, lineY, lineBuffer);
            }
        }
    }
}

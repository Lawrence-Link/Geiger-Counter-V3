/*
 * Copyright (C) 2025 Lawrence Link
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "ui/Popup/PopupInfo.h"
#include "PixelUI.h"
#include <string.h>
#include <etl/algorithm.h>

/**
 * @brief Construct a PopupInfo object
 * @param ui Reference to PixelUI manager
 * @param width Popup width in pixels
 * @param height Popup height in pixels
 * @param text Pointer to the text to display
 * @param title Optional title string
 * @param duration Time in milliseconds before auto-close (0 = no auto-close)
 *
 * Initializes the base popup, splits the text into lines for rendering,
 * and calculates the actual height required.
 */
PopupInfo::PopupInfo(PixelUI& ui, uint16_t width, uint16_t height,
                     const char* text, const char* title, uint16_t duration,
                     const uint8_t* font)
    : PopupBase(ui, width, height, duration), _title(title), _text(text), _lineCount(0), m_font(font)
{
    if (_text) {
        // Split the text into lines based on available width
        _lineCount = splitTextIntoLines(_text, popupWidth() - 2 * TEXT_MARGIN);
        _actualHeight = _lineCount * LINE_HEIGHT + 2 * TEXT_MARGIN;

        // Adjust height if content exceeds initial height
        if (_actualHeight > popupHeight()) {
            setContentHeight(_actualHeight);
        }
    } else {
        _actualHeight = popupHeight();
    }
}

uint16_t PopupInfo::splitTextIntoLines(const char* text, uint16_t maxWidth) {
    if (!text) return 0;

    U8G2& u8g2 = ui().getU8G2();
    u8g2.setFont(m_font);

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
void PopupInfo::drawContent(const PopupContentBounds& bounds) {
    if (_text && _lineCount > 0) {
        U8G2& u8g2 = ui().getU8G2();
        u8g2.setFont(m_font);

        LINE_HEIGHT = u8g2.getFontAscent() - u8g2.getFontDescent();
        int16_t textAreaHeight = _lineCount * LINE_HEIGHT;
        int16_t textStartY = bounds.centerY - textAreaHeight / 2 + LINE_HEIGHT - 2;

        for (uint16_t i = 0; i < _lineCount; i++) {
            if (_textLines[i].length > 0) {
                constexpr size_t MAX_LINE_BUFFER = 64;
                char lineBuffer[MAX_LINE_BUFFER];
                size_t copyLength = etl::min((size_t)_textLines[i].length, MAX_LINE_BUFFER - 1);
                strncpy(lineBuffer, _textLines[i].start, copyLength);
                lineBuffer[copyLength] = '\0';

                int16_t lineWidth = u8g2.getUTF8Width(lineBuffer);
                int16_t lineX = bounds.centerX - lineWidth / 2;
                int16_t lineY = textStartY + i * LINE_HEIGHT;

                u8g2.drawUTF8(lineX, lineY, lineBuffer);
            }
        }
    }
}

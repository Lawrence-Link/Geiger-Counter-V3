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

#pragma once

#include "../IWidget.h"

class IconButton : public IWidget {
private:
    VoidCallback m_callback;
    
    PixelUI& m_ui;
    int32_t pos_x = 0, pos_y = 0, m_w = 0, m_h = 0;

    const unsigned char* src;

    int32_t anim_x = 0, anim_y = 0;
/** @brief drawSelf. */
    void drawSelf(const WidgetRenderContext& context) override;
/** @brief display. */
    Canvas& display() override;
public:
/** @brief IconButton. */
    IconButton(PixelUI& ui, uint16_t x, uint16_t y, uint16_t w, uint16_t h, const unsigned char* source = NULL);
/** @brief ~IconButton. */
    ~IconButton() = default;
/** @brief onLoad. */
    void onLoad() override;
/** @brief onOffload. */
    void onOffload() override;
/** @brief onSelect. */
    bool onSelect() override ;

/** @brief setCallback. */
    void setCallback(VoidCallback cb) {m_callback = cb;}
/** @brief setPosition. */
    void setPosition(uint16_t x, uint16_t y) {
        pos_x = x; pos_y = y;
        setWidgetBounds({pos_x, pos_y, m_w, m_h});
    };
/** @brief setSize. */
    void setSize(uint16_t w, uint16_t h) {
        m_w = w; m_h = h;
        setWidgetBounds({pos_x, pos_y, m_w, m_h});
    }
/** @brief setSource. */
    void setSource(const unsigned char* source) {src = source;};
};

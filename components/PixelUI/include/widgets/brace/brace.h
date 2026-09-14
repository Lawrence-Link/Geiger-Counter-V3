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

class Brace : public IWidget {

public:
/** @brief Brace. */
    Brace(PixelUI& ui, uint16_t pos_x, uint16_t pos_y, uint16_t size_w, uint16_t size_h);
/** @brief ~Brace. */
    ~Brace() = default;

/** @brief onLoad. */
    void onLoad() override;
/** @brief onLoad. */
    void onLoad(LoadTransition transition);
/** @brief onOffload. */
    void onOffload() override;
/** @brief onSelect. */
    bool onSelect() override {
        if (m_callback) m_callback();
        return false;
    }

/** @brief setCallback. */
    void setCallback(VoidCallback cb) { m_callback = cb; }
/** @brief setSize. */
    void setSize(uint16_t width, uint16_t height);
/** @brief setPosition. */
    void setPosition(int16_t pos_x, int16_t pos_y);
    
private:
    PixelUI& m_ui;
    VoidCallback m_callback = nullptr;

    int32_t anim_w = 0;
    int32_t anim_h = 0;
    int32_t anim_x = 0;
    int32_t anim_y = 0;

/** @brief updateBounds. */
    void updateBounds(const FocusBox& bounds);
/** @brief drawSelf. */
    void drawSelf(const WidgetRenderContext& context) override;
/** @brief drawOverlay. */
    void drawOverlay(const WidgetRenderContext& context) override;
/** @brief getChildrenClipBounds. */
    FocusBox getChildrenClipBounds() const override;
/** @brief display. */
    Canvas& display() override;
};

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

#include "widgets/brace/brace.h"
#include "PixelUI.h"

/**
 * @brief Construct a new Brace object
 * * @param ui Reference to the PixelUI instance
 * * Note: pos_x and pos_y represent the top-left corner.
 */
Brace::Brace(PixelUI& ui, uint16_t pos_x, uint16_t pos_y, uint16_t size_w, uint16_t size_h) : 
    m_ui(ui)
{
    setFocusInsets({1, 1, 0, 0});
    updateBounds({pos_x, pos_y, size_w, size_h});
    const int32_t start_anim_x = size_w / 2;
    const int32_t start_anim_y = size_h / 2;
    
    anim_w = 0; 
    anim_h = 0;

    anim_x = start_anim_x;
    anim_y = start_anim_y;
}

/** @brief Brace::updateBounds. */
void Brace::updateBounds(const FocusBox& bounds) {
    setWidgetBounds(bounds);
}

/** @brief Brace::setSize. */
void Brace::setSize(uint16_t width, uint16_t height) {
    const FocusBox bounds = getLocalBounds();
    updateBounds({bounds.x, bounds.y, width, height});
}

/** @brief Brace::setPosition. */
void Brace::setPosition(int16_t pos_x, int16_t pos_y) {
    const FocusBox bounds = getLocalBounds();
    updateBounds({pos_x, pos_y, bounds.w, bounds.h});
}

/**
 * @brief Initialize animations for brace expansion and set the focus box
 */
void Brace::onLoad() {
    onLoad(LoadTransition::Animated);
}

/** @brief Brace::onLoad. */
void Brace::onLoad(LoadTransition transition) {
    const FocusBox bounds = getLocalBounds();
    if (transition == LoadTransition::Immediate) {
        anim_w = bounds.w;
        anim_h = bounds.h;
        anim_x = 0;
        anim_y = 0;
        m_ui.markDirty();
        return;
    }

    const int32_t start_anim_x = bounds.w / 2;
    const int32_t start_anim_y = bounds.h / 2;
    
    anim_w = 0; 
    anim_h = 0;
    
    anim_x = start_anim_x;
    anim_y = start_anim_y;

    if (!m_ui.animate(anim_w, bounds.w, 550, EasingType::EASE_OUT_CUBIC, PROTECTION::PROTECTED)) {
        anim_w = bounds.w;
    }
    if (!m_ui.animate(anim_h, bounds.h, 600, EasingType::EASE_OUT_CUBIC, PROTECTION::PROTECTED)) {
        anim_h = bounds.h;
    }
    
    if (!m_ui.animate(anim_x, 0, 550, EasingType::EASE_OUT_CUBIC, PROTECTION::PROTECTED)) {
        anim_x = 0;
    }
    if (!m_ui.animate(anim_y, 0, 600, EasingType::EASE_OUT_CUBIC, PROTECTION::PROTECTED)) {
        anim_y = 0;
    }
    m_ui.markDirty();
}

/**
 * @brief Called when the widget is removed/offloaded
 */
void Brace::onOffload() {
    // No special cleanup required
}

/**
 * @brief Draw the brace and its child content.
 */
Canvas& Brace::display() { return m_ui.getCanvas(); }

/** @brief Brace::drawSelf. */
void Brace::drawSelf(const WidgetRenderContext&) {}

/** @brief Brace::getChildrenClipBounds. */
FocusBox Brace::getChildrenClipBounds() const {
    const FocusBox bounds = getLocalBounds();
    return {bounds.x + anim_x, bounds.y + anim_y, anim_w, anim_h};
}

/** @brief Brace::drawOverlay. */
void Brace::drawOverlay(const WidgetRenderContext& context) {
    Canvas& u8g2 = m_ui.getCanvas();
    const FocusBox bounds = getLocalBounds();

    const int32_t tl_x = context.originX + bounds.x + anim_x;
    const int32_t tl_y = context.originY + bounds.y + anim_y;
    const int32_t current_w = anim_w;
    const int32_t current_h = anim_h;
    if (current_w <= 0 || current_h <= 0) return;

    const int32_t corner_w = current_w < 4 ? current_w : 4;
    const int32_t corner_h = current_h < 4 ? current_h : 4;
    
    // Top-left corner: (tl_x, tl_y)
    u8g2.drawLine(tl_x, tl_y, tl_x + corner_w, tl_y);
    u8g2.drawLine(tl_x, tl_y, tl_x, tl_y + corner_h);

    // Top-right corner: (tl_x + current_w, tl_y)
    u8g2.drawLine(tl_x + current_w, tl_y, tl_x + current_w - corner_w, tl_y);
    u8g2.drawLine(tl_x + current_w, tl_y, tl_x + current_w, tl_y + corner_h);

    // Bottom-left corner: (tl_x, tl_y + current_h)
    u8g2.drawLine(tl_x, tl_y + current_h, tl_x + corner_w, tl_y + current_h);
    u8g2.drawLine(tl_x, tl_y + current_h, tl_x, tl_y + current_h - corner_h);

    // Bottom-right corner: (tl_x + current_w, tl_y + current_h)
    u8g2.drawLine(tl_x + current_w, tl_y + current_h, tl_x + current_w - corner_w, tl_y + current_h);
    u8g2.drawLine(tl_x + current_w, tl_y + current_h, tl_x + current_w, tl_y + current_h - corner_h);
}

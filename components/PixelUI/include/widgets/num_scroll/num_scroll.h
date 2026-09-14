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
#include "core/NumericFormatter.h"
#include "core/NumericRange.h"
#include "core/animation/animation.h"

/**
 * @class NumScroll
 * @brief A scrollable numeric input widget with vertical scrolling animation
 * 
 * This widget displays a number in a square box and allows users to increment
 * or decrement the value using LEFT/RIGHT input events. The scrolling effect
 * is achieved through vertical animation of digit sprites.
 */
class NumScroll : public IWidget {
public:
    enum class Presentation : uint8_t {
        Framed,
        Bare
    };

/** @brief NumScroll. */
    NumScroll(PixelUI& ui, uint16_t x, uint16_t y, uint16_t w, uint16_t h);
/** @brief NumScroll. */
    NumScroll(PixelUI& ui, uint16_t x, uint16_t y, uint16_t w, uint16_t h,
              const NumericRange& range, NumericFormatter formatter);
/** @brief ~NumScroll. */
    ~NumScroll() override;

/** @brief onLoad. */
    void onLoad() override;
/** @brief onLoad. */
    void onLoad(LoadTransition transition);
    // Compatibility wrapper for callers that already use the old name.
/** @brief onLoadNoAnim. */
    void onLoadNoAnim();
/** @brief onOffload. */
    void onOffload() override;
/** @brief handleEvent. */
    bool handleEvent(InputEvent event) override;
/** @brief onSelect. */
    bool onSelect() override;
/** @brief getTimeout. */
    uint32_t getTimeout() const override { return TIMEOUT_MS; }

/** @brief setPosition. */
    void setPosition(uint16_t x, uint16_t y) {
        m_x = x; m_y = y; 
        updateGeometry();
    }

/** @brief setRange. */
    void setRange(const NumericRange& range);
/** @brief setRange. */
    bool setRange(int32_t minValue, int32_t maxValue, int32_t step = 1);
    // Any non-null formatter context must outlive this NumScroll.
/** @brief setFormatter. */
    void setFormatter(NumericFormatter formatter) { formatter_ = formatter; }
/** @brief setValue. */
    void setValue(int32_t val);
/** @brief setValueImmediate. */
    void setValueImmediate(int32_t val);
/** @brief getValue. */
    int32_t getValue() const { return m_current_value; }

/** @brief setPresentation. */
    void setPresentation(Presentation presentation) {
        presentation_ = presentation;
        updateGeometry();
    }
/** @brief setSize. */
    void setSize(uint16_t w, uint16_t h) {
        m_w = w; m_h = h; 
        updateGeometry();
    }

private:
    PixelUI& m_ui;

    uint16_t m_x = 0;
    uint16_t m_y = 0;

    uint16_t m_w = 24;
    uint16_t m_h = 16;

    int32_t m_current_value = 0;
    NumericRange range_{};
    NumericFormatter formatter_ = NumericFormatter::integer();
    Presentation presentation_ = Presentation::Framed;

    int32_t m_anim_offset = 0;
    
    int32_t anim_w = 0;
    int32_t anim_h = 0;
    AnimationHandle valueAnimation_ = INVALID_ANIMATION_HANDLE;
    AnimationHandle sizeAnimation_ = INVALID_ANIMATION_HANDLE;

    static constexpr uint32_t TIMEOUT_MS = 5000;

/** @brief drawSelf. */
    void drawSelf(const WidgetRenderContext& context) override;
/** @brief display. */
    Canvas& display() override;

/** @brief incrementValue. */
    void incrementValue();
/** @brief decrementValue. */
    void decrementValue();
/** @brief animateToValue. */
    void animateToValue(int32_t new_value);
/** @brief updateGeometry. */
    void updateGeometry();
/** @brief cancelOwnAnimations. */
    void cancelOwnAnimations();

/** @brief formatValue. */
    bool formatValue(int32_t value, char* buffer, size_t bufferSize) const;
};

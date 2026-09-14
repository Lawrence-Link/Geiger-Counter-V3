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

#include "config.h"
#include "../IWidget.h"
#include "calc/TextAlignHelper/TextAlignHelper.h"
#if PIXELUI_USE_LABEL_SCROLL
#include "core/scheduling/UiDeadlineScheduler.h"
#endif

enum class POS {
    TOP, BOTTOM, LEFT, RIGHT
};

class Label : public IWidget
#if PIXELUI_USE_LABEL_SCROLL
            , private UiDeadlineSource
#endif
{
public:
    enum class Overflow : uint8_t {
        Clip,
        AutoScroll
    };

private:
    PixelUI& m_ui;
int32_t m_x = 0, m_y = 0, m_w = 0, m_h = 0;
    const char* src = nullptr;
    int32_t anim_x = 0, anim_y = 0;
    int32_t text_width_ = 0;
    int32_t font_height_ = 0;
    int32_t load_distance_ = 0;
    VoidCallback m_callback;
    POS load_pos;
    uint8_t const* m_font;
    Overflow overflow_ = Overflow::Clip;
    TextAlignX text_alignment_ = TextAlignX::Left;
    bool loaded_ = false;

#if PIXELUI_USE_LABEL_SCROLL
    enum class ScrollPhase : uint8_t {
        StartPause,
        ScrollingToEnd,
        EndPause,
        ScrollingToStart
    };

    int32_t scroll_offset_ = 0;
    uint32_t next_scroll_deadline_ = 0;
    ScrollPhase scroll_phase_ = ScrollPhase::StartPause;
    bool scroll_running_ = false;

/** @brief nextWakeupMs. */
    uint32_t nextWakeupMs(uint32_t currentTime) const override;
/** @brief update. */
    bool update(uint32_t currentTime) override;
/** @brief restartAutoScroll. */
    void restartAutoScroll(uint32_t additionalDelayMs = 0U);
/** @brief stopAutoScroll. */
    void stopAutoScroll();
#endif

/** @brief refreshMetrics. */
    void refreshMetrics();
/** @brief drawSelf. */
    void drawSelf(const WidgetRenderContext& context) override;
/** @brief display. */
    Canvas& display() override;
public:
/** @brief Label. */
    Label(PixelUI& ui, uint16_t x, uint16_t y, const char* content, POS pos = POS::BOTTOM, const uint8_t* font = PIXELUI_FONT_TEXT);
/** @brief Label. */
    Label(PixelUI& ui, uint16_t x, uint16_t y, uint16_t viewportWidth,
          const char* content, POS pos = POS::BOTTOM,
          const uint8_t* font = PIXELUI_FONT_TEXT);
/** @brief ~Label. */
    ~Label() override = default;
/** @brief onLoad. */
    void onLoad() override;
/** @brief onLoad. */
    void onLoad(LoadTransition transition);
    // Compatibility wrapper for callers that already use the old name.
/** @brief onLoadImmediately. */
    void onLoadImmediately();
/** @brief onOffload. */
    void onOffload() override;
/** @brief onSelect. */
    bool onSelect() override ;
/** @brief setLoadPos. */
    void setLoadPos(POS pos) {load_pos = pos;};
/** @brief setCallback. */
    void setCallback(VoidCallback cb) {m_callback = cb;}
/** @brief setPosition. */
    void setPosition(uint16_t x, uint16_t y) {
        m_x = x; m_y = y;
        setWidgetBounds({m_x, m_y, m_w, m_h});
    };
/** @brief setSize. */
    void setSize(uint16_t w, uint16_t h);
/** @brief setViewportWidth. */
    void setViewportWidth(uint16_t width);
/** @brief clearViewportWidth. */
    void clearViewportWidth() { setViewportWidth(0U); }
/** @brief getViewportWidth. */
    uint16_t getViewportWidth() const { return static_cast<uint16_t>(m_w); }
/** @brief setOverflow. */
    void setOverflow(Overflow overflow);
/** @brief getOverflow. */
    Overflow getOverflow() const { return overflow_; }
/** @brief setTextAlignment. */
    void setTextAlignment(TextAlignX alignment);
/** @brief getTextAlignment. */
    TextAlignX getTextAlignment() const { return text_alignment_; }
/** @brief setText. */
    void setText(const char* source);

private:
/** @brief load. */
    void load(bool animate);
};

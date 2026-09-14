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

#include "widgets/label/label.h"
#include "PixelUI.h"
#if PIXELUI_USE_LABEL_SCROLL
#include "core/TimeUtils.h"
#endif

namespace {

constexpr uint32_t LOAD_ANIMATION_DURATION_MS = 300U;

#if PIXELUI_USE_LABEL_SCROLL
constexpr uint32_t SCROLL_START_PAUSE_MS = 800U;
constexpr uint32_t SCROLL_END_PAUSE_MS = 500U;
constexpr uint32_t SCROLL_PIXEL_INTERVAL_MS = 50U;
#endif

} // namespace

/**
 * @brief Constructor for Label widget.
 * @param ui Reference to the PixelUI instance for rendering and animation.
 * @param x X coordinate of label reference point.
 * @param y Y coordinate of label reference point.
 * @param content Text content of the label.
 * @param pos Label alignment position relative to (x, y).
 */
Label::Label(PixelUI& ui, uint16_t x, uint16_t y, const char* content, POS pos, const uint8_t* font)
    : Label(ui, x, y, 0U, content, pos, font)
{
}

Label::Label(PixelUI& ui, uint16_t x, uint16_t y, uint16_t viewportWidth,
             const char* content, POS pos, const uint8_t* font)
#if PIXELUI_USE_LABEL_SCROLL
    : UiDeadlineSource(ui.m_deadlineScheduler)
    , m_ui(ui)
#else
    : m_ui(ui)
#endif
    , m_x(x)
    , m_y(y)
    , m_w(viewportWidth)
    , src(content)
    , load_pos(pos)
    , m_font(font)
{
    setFocusable(true);
    setFocusInsets({1, 1, 1, 1});
    setWidgetBounds({m_x, m_y, m_w, m_h});
}

/** @brief Label::refreshMetrics. */
void Label::refreshMetrics() {
    Canvas& canvas = m_ui.getCanvas();
    const uint8_t* previousFont = canvas.rawDisplay().getU8g2()->font;
    canvas.setFont(m_font);
    font_height_ = canvas.getAscent() - canvas.getDescent();
    load_distance_ = canvas.getFontAscent() + canvas.getFontDescent();
    text_width_ = src ? canvas.getUTF8Width(src) : 0;
    if (previousFont) canvas.setFont(previousFont);
}

/**
 * @brief Initialize the label when loaded. Calculates animated starting position
 *        based on alignment and triggers slide-in animation.
 */
void Label::onLoad() {
    onLoad(LoadTransition::Animated);
}

/** @brief Label::onLoad. */
void Label::onLoad(LoadTransition transition) {
    load(transition == LoadTransition::Animated);
}

/** @brief Label::onLoadImmediately. */
void Label::onLoadImmediately() {
    onLoad(LoadTransition::Immediate);
}

/** @brief Label::load. */
void Label::load(bool animate) {
    refreshMetrics();
    loaded_ = true;

    anim_x = m_x;
    anim_y = m_y;

    if (animate) {
        // Set initial animated position based on alignment
        switch (load_pos) {
            case POS::TOP: {
                anim_y = m_y - load_distance_;
                if (!m_ui.animate(anim_y, m_y, LOAD_ANIMATION_DURATION_MS, EasingType::EASE_OUT_CUBIC, PROTECTION::NOT_PROTECTED)) {
                    anim_y = m_y;
                }
            } break;
            case POS::BOTTOM: {
                anim_y = m_y + load_distance_;
                if (!m_ui.animate(anim_y, m_y, LOAD_ANIMATION_DURATION_MS, EasingType::EASE_OUT_CUBIC, PROTECTION::NOT_PROTECTED)) {
                    anim_y = m_y;
                }
            } break;
            case POS::LEFT: {
                anim_x = m_x - text_width_;
                if (!m_ui.animate(anim_x, m_x, LOAD_ANIMATION_DURATION_MS, EasingType::EASE_OUT_CUBIC, PROTECTION::NOT_PROTECTED)) {
                    anim_x = m_x;
                }
            } break;
            case POS::RIGHT: {
                anim_x = m_x + text_width_;
                if (!m_ui.animate(anim_x, m_x, LOAD_ANIMATION_DURATION_MS, EasingType::EASE_OUT_CUBIC, PROTECTION::NOT_PROTECTED)) {
                    anim_x = m_x;
                }
            } break;
        }
    }

#if PIXELUI_USE_LABEL_SCROLL
#if PIXELUI_USE_ANIMATION
    restartAutoScroll(animate ? LOAD_ANIMATION_DURATION_MS : 0U);
#else
    restartAutoScroll();
#endif
#endif

    m_ui.markDirty();
}

/**
 * @brief Clean up resources when label is offloaded.
 */
void Label::onOffload() {
    loaded_ = false;
#if PIXELUI_USE_LABEL_SCROLL
    stopAutoScroll();
#endif
}

/** @brief Label::setSize. */
void Label::setSize(uint16_t w, uint16_t h) {
    m_w = w;
    m_h = h;
    setWidgetBounds({m_x, m_y, m_w, m_h});
#if PIXELUI_USE_LABEL_SCROLL
    restartAutoScroll();
#endif
    m_ui.markDirty();
}

/** @brief Label::setViewportWidth. */
void Label::setViewportWidth(uint16_t width) {
    m_w = width;
    setWidgetBounds({m_x, m_y, m_w, m_h});
#if PIXELUI_USE_LABEL_SCROLL
    restartAutoScroll();
#endif
    m_ui.markDirty();
}

/** @brief Label::setOverflow. */
void Label::setOverflow(Overflow overflow) {
    if (overflow_ == overflow) return;
    overflow_ = overflow;
#if PIXELUI_USE_LABEL_SCROLL
    restartAutoScroll();
#endif
    m_ui.markDirty();
}

/** @brief Label::setTextAlignment. */
void Label::setTextAlignment(TextAlignX alignment) {
    if (text_alignment_ == alignment) return;
    text_alignment_ = alignment;
#if PIXELUI_USE_LABEL_SCROLL
    restartAutoScroll();
#endif
    m_ui.markDirty();
}

/** @brief Label::setText. */
void Label::setText(const char* source) {
    src = source;
    if (loaded_) refreshMetrics();
#if PIXELUI_USE_LABEL_SCROLL
    restartAutoScroll();
#endif
    m_ui.markDirty();
}

#if PIXELUI_USE_LABEL_SCROLL
/** @brief Label::stopAutoScroll. */
void Label::stopAutoScroll() {
    scroll_running_ = false;
    scroll_offset_ = 0;
    scroll_phase_ = ScrollPhase::StartPause;
}

/** @brief Label::restartAutoScroll. */
void Label::restartAutoScroll(uint32_t additionalDelayMs) {
    stopAutoScroll();
    if (!loaded_ || overflow_ != Overflow::AutoScroll || m_w <= 0 ||
        text_width_ <= m_w) {
        return;
    }

    scroll_running_ = true;
    next_scroll_deadline_ = m_ui.getCurrentTime() + additionalDelayMs +
                            SCROLL_START_PAUSE_MS;
}

/** @brief Label::nextWakeupMs. */
uint32_t Label::nextWakeupMs(uint32_t currentTime) const {
    if (!scroll_running_) return PixelUITime::NO_WAKEUP;
    return PixelUITime::untilDeadline(currentTime, next_scroll_deadline_);
}

/** @brief Label::update. */
bool Label::update(uint32_t currentTime) {
    if (!scroll_running_ ||
        !PixelUITime::deadlineReached(currentTime, next_scroll_deadline_)) {
        return false;
    }

    switch (scroll_phase_) {
        case ScrollPhase::StartPause:
            scroll_phase_ = ScrollPhase::ScrollingToEnd;
            next_scroll_deadline_ = currentTime + SCROLL_PIXEL_INTERVAL_MS;
            return false;

        case ScrollPhase::ScrollingToEnd: {
            const uint32_t overdue = currentTime - next_scroll_deadline_;
            const int32_t step = static_cast<int32_t>(
                1U + overdue / SCROLL_PIXEL_INTERVAL_MS);
            const int32_t maximumOffset = text_width_ - m_w;
            const int32_t previousOffset = scroll_offset_;
            scroll_offset_ -= step;
            if (-scroll_offset_ >= maximumOffset) {
                scroll_offset_ = -maximumOffset;
                scroll_phase_ = ScrollPhase::EndPause;
                next_scroll_deadline_ = currentTime + SCROLL_END_PAUSE_MS;
            } else {
                next_scroll_deadline_ +=
                    static_cast<uint32_t>(step) * SCROLL_PIXEL_INTERVAL_MS;
            }
            return scroll_offset_ != previousOffset;
        }

        case ScrollPhase::EndPause:
            scroll_phase_ = ScrollPhase::ScrollingToStart;
            next_scroll_deadline_ = currentTime + SCROLL_PIXEL_INTERVAL_MS;
            return false;

        case ScrollPhase::ScrollingToStart: {
            const uint32_t overdue = currentTime - next_scroll_deadline_;
            const int32_t step = static_cast<int32_t>(
                1U + overdue / SCROLL_PIXEL_INTERVAL_MS);
            const int32_t previousOffset = scroll_offset_;
            scroll_offset_ += step;
            if (scroll_offset_ >= 0) {
                scroll_offset_ = 0;
                scroll_phase_ = ScrollPhase::StartPause;
                next_scroll_deadline_ = currentTime + SCROLL_START_PAUSE_MS;
            } else {
                next_scroll_deadline_ +=
                    static_cast<uint32_t>(step) * SCROLL_PIXEL_INTERVAL_MS;
            }
            return scroll_offset_ != previousOffset;
        }
    }

    return false;
}
#endif

/**
 * @brief Render the label text on screen with clipping to avoid overflow.
 */
Canvas& Label::display() { return m_ui.getCanvas(); }

/** @brief Label::drawSelf. */
void Label::drawSelf(const WidgetRenderContext& context) {
    if (!src) return;

    Canvas& u8g2 = m_ui.getCanvas();
    u8g2.setFont(m_font);

    const int32_t viewportWidth = m_w > 0 ? m_w : text_width_;
    setClipWindow(context, {m_x, m_y - font_height_ , viewportWidth, font_height_ + 2});

    // Draw text at animated position
    int32_t drawX = anim_x + TextAlignHelper::calcAlignedOffset(
        viewportWidth, text_width_, text_alignment_,
        TextOverflowPlacement::PinToLeadingEdge);
#if PIXELUI_USE_LABEL_SCROLL
    drawX += scroll_offset_;
#endif
    u8g2.drawUTF8(context.originX + drawX, context.originY + anim_y, src);

    // Reset clipping
    restoreClipWindow(context);
}

/**
 * @brief Handle label selection. Triggers callback if defined.
 * @return False, indicating event propagation continues.
 */
bool Label::onSelect() {
    if (m_callback) {
        m_callback();
    }
    return false;
}

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

#include "ui/Popup/PopupBase.h"
#include "PixelUI.h"
#include "core/TimeUtils.h"
#include "core/animation/animation.h"

/**
 * @brief Construct a PopupBase object
 * @param ui Reference to the PixelUI manager
 * @param width Width of the popup
 * @param height Height of the popup
 * @param duration Duration in milliseconds before auto-closing (0 = no auto-close)
 *
 * Initializes the popup state to APPEARING and calculates the target box size.
 */
PopupBase::PopupBase(PixelUI& ui, uint16_t width, uint16_t height, uint16_t duration)
    : m_ui(ui), m_width(width), m_height(height), m_duration(duration)
{
    m_targetBoxSize = static_cast<int32_t>(m_width) << SHIFT_BITS;
}

/**
 * @brief Draw the popup frame and inner box.
 * @param rectX X position of the popup top-left corner
 * @param rectY Y position of the popup top-left corner
 * @param currentWidth Current width (animated)
 * @param currentHeight Current height (animated)
 *
 * Draws outer frame, inner frame, and fills the inner area.
 */
void PopupBase::drawPopupBox(const PopupContentBounds& bounds) {
    U8G2& u8g2 = m_ui.getU8G2();

    u8g2.setDrawColor(1);

    // Avoid invalid rounded boxes while the appearing/closing animation is tiny.
    if (4 > (bounds.width - 2 * BORDER_OFFSET) / 2 ||
        4 > (bounds.height - 2 * BORDER_OFFSET) / 2) {
        return;
    }

    u8g2.drawRFrame(bounds.x + BORDER_OFFSET, bounds.y + BORDER_OFFSET,
                    bounds.width - 2 * BORDER_OFFSET,
                    bounds.height - 2 * BORDER_OFFSET, 4);
    u8g2.drawRFrame(bounds.x, bounds.y, bounds.width, bounds.height, 4);

    u8g2.setDrawColor(0);
    u8g2.drawRBox(bounds.x + BORDER_WIDTH, bounds.y + BORDER_WIDTH,
                  bounds.width - 2 * BORDER_WIDTH,
                  bounds.height - 2 * BORDER_WIDTH, 4);

    u8g2.setDrawColor(1);
}

/**
 * @brief Setup clipping for the popup area to avoid drawing outside bounds
 * @param rectX X position of top-left corner
 * @param rectY Y position of top-left corner
 * @param currentWidth Current width of the popup
 * @param currentHeight Current height of the popup
 */
void PopupBase::setContentClip(const PopupContentBounds& bounds) {
    U8G2& u8g2 = m_ui.getU8G2();
    u8g2.setClipWindow(bounds.x, bounds.y,
                       bounds.x + bounds.width, bounds.y + bounds.height);
}

/**
 * @brief Reset clipping to the full screen
 */
void PopupBase::resetClipWindow() {
    U8G2& u8g2 = m_ui.getU8G2();
    u8g2.setMaxClipWindow();
    u8g2.setDrawColor(1);
}

/**
 * @brief Update the popup state based on current time.
 * @param currentTime Current system time in milliseconds
 * @return true if the popup is still active, false if it has finished closing
 *
 * Manages APPEARING -> SHOWING -> CLOSING transitions internally.
 */
void PopupBase::beginAppearing(uint32_t currentTime) {
    m_started = true;
    m_transitionStartTime = currentTime;
    m_transitionStartSize = 0;
    m_currentBoxSize = 0;
}

bool PopupBase::updateAppearing(uint32_t currentTime) {
    const uint32_t elapsed = currentTime - m_transitionStartTime;
    if (elapsed >= TRANSITION_DURATION) {
        m_currentBoxSize = m_targetBoxSize;
        m_state = PopupState::SHOWING;
        m_stateStartTime = currentTime;
        onShown();
        return true;
    }

    const int32_t timeProgress = static_cast<int32_t>(
        (static_cast<int64_t>(elapsed) * FIXED_POINT_ONE) / TRANSITION_DURATION);
    const int32_t eased = EasingCalculator::calculate(EasingType::EASE_OUT_CUBIC, timeProgress);
    m_currentBoxSize = static_cast<int32_t>(
        (static_cast<int64_t>(m_targetBoxSize) * eased) / FIXED_POINT_ONE);
    return true;
}

bool PopupBase::updateClosing(uint32_t currentTime) {
    const uint32_t elapsed = currentTime - m_transitionStartTime;
    if (elapsed >= TRANSITION_DURATION) {
        m_currentBoxSize = 0;
        return false;
    }

    const int32_t timeProgress = static_cast<int32_t>(
        (static_cast<int64_t>(elapsed) * FIXED_POINT_ONE) / TRANSITION_DURATION);
    const int32_t eased = EasingCalculator::calculate(EasingType::EASE_IN_CUBIC, timeProgress);
    m_currentBoxSize = m_transitionStartSize - static_cast<int32_t>(
        (static_cast<int64_t>(m_transitionStartSize) * eased) / FIXED_POINT_ONE);
    return true;
}

/**
 * @brief Start the internal closing transition.
 */
void PopupBase::requestClose() {
    if (m_state != PopupState::CLOSING && onClosing()) {
        m_state = PopupState::CLOSING;
        m_transitionStartTime = m_ui.getCurrentTime();
        m_transitionStartSize = m_currentBoxSize;
        m_ui.markDirty();
    }
}

void PopupBase::resetAutoCloseTimer() {
    if (m_state == PopupState::SHOWING) {
        m_stateStartTime = m_ui.getCurrentTime();
    }
}

/**
 * @brief Update the popup
 * @param currentTime Current system time in milliseconds
 * @return true if still active
 *
 * Advances the internal transition and auto-close state machine.
 */
bool PopupBase::update(uint32_t currentTime) {
    if (!m_started) {
        beginAppearing(currentTime);
    }

    bool active = true;
    switch (m_state) {
        case PopupState::APPEARING:
            active = updateAppearing(currentTime);
            break;
        case PopupState::SHOWING:
            if (m_duration > 0U && currentTime - m_stateStartTime >= m_duration) {
                requestClose();
            }
            break;
        case PopupState::CLOSING:
            active = updateClosing(currentTime);
            break;
    }

    if (active && m_state != PopupState::SHOWING) {
        m_ui.markDirty();
    }
    return active;
}

uint32_t PopupBase::nextWakeupMs(
    uint32_t currentTime, uint32_t frameIntervalMs) const {
    if (!m_started) return 0U;

    if (m_state == PopupState::SHOWING) {
        if (m_duration == 0U) return PixelUITime::NO_WAKEUP;
        return PixelUITime::untilDeadline(
            currentTime, m_stateStartTime + m_duration);
    }

    const uint32_t remaining = PixelUITime::untilDeadline(
        currentTime, m_transitionStartTime + TRANSITION_DURATION);
    return PixelUITime::earlier(
        PixelUITime::normalizeInterval(frameIntervalMs), remaining);
}

/**
 * @brief Handle input events for the popup
 * @param event InputEvent triggered by user
 * @return true if handled
 *
 * Currently closes popup on any input while showing.
 */
bool PopupBase::handleInput(InputEvent event) {
    if (m_state == PopupState::CLOSING) {
        return true;
    }

    resetAutoCloseTimer();
    if (handleContentInput(event)) {
        return true;
    }

    if (m_state == PopupState::SHOWING) {
        requestClose();
    }
    return true;
}

/**
 * @brief Draw the popup on screen
 *
 * Calculates current width/height from animated _currentBoxSize and centers popup.
 * Sets clipping, draws popup frame, and calls drawContent() for derived classes.
 */
void PopupBase::draw() {
    int16_t screenWidth = m_ui.getDisplayWidth();
    int16_t screenHeight = m_ui.getDisplayHeight();
    int16_t centerX = screenWidth / 2;
    int16_t centerY = screenHeight / 2;
    
    int16_t currentWidth = m_currentBoxSize >> SHIFT_BITS;
    if (currentWidth <= 0) return;
    
    int16_t currentHeight = (m_width > 0) ? (currentWidth * m_height) / m_width : 0;
    if (currentHeight <= 0) return;
    
    int16_t rectX = centerX - currentWidth / 2;
    int16_t rectY = centerY - currentHeight / 2;
    
    const PopupContentBounds bounds{
        rectX, rectY, currentWidth, currentHeight, centerX, centerY};
    setContentClip(bounds);
    drawPopupBox(bounds);
    drawContent(bounds);
    resetClipWindow();
}

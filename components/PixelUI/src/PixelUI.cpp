/*
 * PixelUI.cpp - PixelUI main class implementation
 *
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

#include "PixelUI.h"
#include "core/ViewManager/ViewManager.h"
#include "core/animation/animation.h"
#include "ui/Popup/PopupProgress.h"
#include "ui/Popup/PopupInfo.h"
#include "ui/Popup/PopupValue4Digits.h"
#include "core/coroutine/Coroutine.h"
#include <cinttypes>

/**
 * @brief Construct a PixelUI instance with a U8G2 display reference
 * @param u8g2 Reference to the U8G2 display object
 *
 * Initializes core subsystems: ViewManager, AnimationManager, PopupManager, and CoroutineScheduler.
 */
PixelUI::PixelUI(U8G2& u8g2) : u8g2_(u8g2), _currentTime(0) {
    m_viewManagerPtr = std::make_shared<ViewManager>(*this);
    m_animationManagerPtr = std::make_shared<AnimationManager>();
    m_popupManagerPtr = std::make_shared<PopupManager>(*this);
    m_coroutineSchedulerPtr = std::make_shared<CoroutineScheduler>(*this);
}

/**
 * @brief Add a coroutine to the scheduler
 * @param coroutine Shared pointer to the Coroutine object
 */
void PixelUI::addCoroutine(Coroutine* coroutine) { 
    m_coroutineSchedulerPtr->addCoroutine(coroutine); 
}

/**
 * @brief Remove a coroutine from the scheduler
 * @param coroutine Shared pointer to the Coroutine object
 */
void PixelUI::removeCoroutine(Coroutine* coroutine) { 
    m_coroutineSchedulerPtr->removeCoroutine(coroutine); 
}

/**
 * @brief Placeholder initialization function
 *
 * Currently empty; kept for API consistency and future expansion.
 */
void PixelUI::begin() { }

/**
 * @brief Heartbeat function to update all subsystems
 * @param ms Milliseconds elapsed since last heartbeat
 *
 * Updates animation manager, popup manager, and coroutine scheduler.
 */
void PixelUI::Heartbeat(uint32_t ms) {
    _currentTime += ms;
    m_animationManagerPtr->update(_currentTime);
    m_popupManagerPtr->updatePopups(_currentTime);
    m_coroutineSchedulerPtr->update(_currentTime);
}

/**
 * @brief Add and start an animation
 * @param animation Shared pointer to an Animation object
 */
void PixelUI::addAnimation(std::shared_ptr<Animation> animation) {
    animation->start(_currentTime);
    m_animationManagerPtr->addAnimation(animation);
}

/**
 * @brief Animate a single integer value with optional protection
 */
void PixelUI::animate(int32_t& value, int32_t targetValue, uint32_t duration,
                      EasingType easing, PROTECTION prot) {
    auto animation = std::make_shared<CallbackAnimation>(
        value, targetValue, duration, easing,
        [&value](int32_t currentValue) { value = currentValue; }
    );
    if (prot == PROTECTION::PROTECTED) m_animationManagerPtr->markProtected(animation);
    addAnimation(animation);
}

/**
 * @brief Animate two integer values simultaneously
 */
void PixelUI::animate(int32_t& x, int32_t& y, int32_t targetX, int32_t targetY,
                      uint32_t duration, EasingType easing, PROTECTION prot) {
    auto animX = std::make_shared<CallbackAnimation>(x, targetX, duration, easing, [&x](int32_t val) { x = val; });
    auto animY = std::make_shared<CallbackAnimation>(y, targetY, duration, easing, [&y](int32_t val) { y = val; });
    addAnimation(animX);
    addAnimation(animY);
    if (prot == PROTECTION::PROTECTED) {
        m_animationManagerPtr->markProtected(animX);
        m_animationManagerPtr->markProtected(animY);
    }
}

/**
 * @brief Render the current drawable and active popups
 *
 * Handles optional fading effects and calls the refresh callback if set.
 */
void PixelUI::renderer() {
    if (m_viewManagerPtr->isTransitioning()) return;

    static uint8_t lastPopupCount = 0;
    uint8_t currentPopupCount = m_popupManagerPtr->getPopupCounts();
    if (currentPopupCount != lastPopupCount) { markDirty(); lastPopupCount = currentPopupCount; }

    if (getActiveAnimationCount() || isContinousRefreshEnabled()) { markDirty(); }

    if (!isFading_) {
        u8g2_.clearBuffer();
        if (currentDrawable_) currentDrawable_->draw();
        m_popupManagerPtr->drawPopups();
        u8g2_.sendBuffer();
        if (m_refresh_callback) m_refresh_callback();
        isDirty_ = false;
    } else {
        if (m_fadeStep == 0) {
            u8g2_.clearBuffer();
            if (currentDrawable_) currentDrawable_->draw();
            m_popupManagerPtr->drawPopups();
            u8g2_.sendBuffer();
            if (m_refresh_callback) m_refresh_callback();
            m_fadeStep = 1;
            m_lastFadeTime = getCurrentTime();
            return;
        }
        if (m_fadeStep >= 1 && m_fadeStep <= 4) {
            if (getCurrentTime() - m_lastFadeTime < 40) return;

            uint8_t *buf_ptr = u8g2_.getBufferPtr();
            uint16_t buf_len = 1024;
            switch (m_fadeStep) {
                case 1: for (uint16_t i=0;i<buf_len;i++) if (i%2) buf_ptr[i] &= 0xAA; break;
                case 2: for (uint16_t i=0;i<buf_len;i++) if (i%2) buf_ptr[i] &= 0x00; break;
                case 3: for (uint16_t i=0;i<buf_len;i++) if (!(i%2)) buf_ptr[i] &= 0x55; break;
                case 4: for (uint16_t i=0;i<buf_len;i++) if (!(i%2)) buf_ptr[i] &= 0x00; break;
            }
            u8g2_.sendBuffer();
            if (m_refresh_callback) m_refresh_callback();
            m_lastFadeTime = getCurrentTime();
            m_fadeStep++;
            if (m_fadeStep > 4) { isFading_ = false; m_fadeStep = 0; }
        }
    }
}

/**
 * @brief Show a progress popup
 */
void PixelUI::showPopupProgress(int32_t& value, int32_t minValue, int32_t maxValue,
                                const char* title, uint16_t width, uint16_t height,
                                uint16_t duration, uint8_t priority,
                                std::function<void(int32_t val)> update_cb, bool use_apparent_val) {
    if (minValue >= maxValue) return;
    if (width < 50) width = 50; 
    if (width > 120) width = 120;
    if (height < 30) height = 30; 
    if (height > 60) height = 60;
    if (duration > 30000) duration = 30000; 
    if (duration < 1000) duration = 1000;

    auto popup = std::make_shared<PopupProgress>(*this, width, height, value,
                                                 minValue, maxValue, title,
                                                 duration, priority, update_cb, use_apparent_val);
    m_popupManagerPtr->addPopup(popup);
    markDirty();
}

/**
 * @brief Show an informational popup
 */
void PixelUI::showPopupInfo(const char* text, const char* title,
                            uint16_t width, uint16_t height,
                            uint16_t duration, uint8_t priority) {
    if (!text) return;
    auto popup = std::make_shared<PopupInfo>(*this, width, height, text, title,
                                             duration, priority);
    m_popupManagerPtr->addPopup(popup);
    markDirty();
}

/**
 * @brief Show a 4-digit value popup
 */
void PixelUI::showPopupValue4Digits(int32_t& value, const char* title,
                                    uint16_t width, uint16_t height,
                                    uint16_t duration, uint8_t priority,
                                    std::function<void(int32_t val)> update_cb) {
    if (width < 50) width = 50; 
    if (width > 120) width = 120;
    if (height < 30) height = 30; 
    if (height > 60) height = 60;
    if (duration > 30000) duration = 30000; 
    if (duration < 1000) duration = 1000;

    auto popup = std::make_shared<PopupValue4Digits>(*this, width, height, value,
                                                     title, duration, priority,
                                                     update_cb);
    m_popupManagerPtr->addPopup(popup);
    markDirty();
}

/**
 * @brief Trigger a UI fading effect
 */
void PixelUI::markFading() {
    if (!isFading_) { 
        isFading_ = true;
        m_fadeStep = 1;                  
        m_lastFadeTime = getCurrentTime(); 
        markDirty();
    }
}
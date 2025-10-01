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

#include "PixelUI.h"
#include "core/ViewManager/ViewManager.h"
#include <functional>
#include "core/app/app_system.h"
#include "core/animation/animation.h"
#include "ui/Popup/Popup.h"
#include "core/coroutine/Coroutine.h"
#include <cinttypes>
/**
 * @class PixelUI
 * @brief Main class for the PixelUI framework.
 *
 * Handles the central logic for UI rendering, animation management,
 * and event handling.
 */
PixelUI::PixelUI(U8G2& u8g2) : u8g2_(u8g2), _currentTime(0) {
    m_viewManagerPtr = std::make_shared<ViewManager>(*this);
    m_animationManagerPtr = std::make_shared<AnimationManager>();
    m_popupManagerPtr = std::make_shared<PopupManager>(*this);
    m_coroutineSchedulerPtr = std::make_shared<CoroutineScheduler>(*this);
}

void PixelUI::addCoroutine(std::shared_ptr<Coroutine> coroutine) { 
        m_coroutineSchedulerPtr->addCoroutine(coroutine); 
    }
void PixelUI::removeCoroutine(std::shared_ptr<Coroutine> coroutine) { 
    m_coroutineSchedulerPtr->removeCoroutine(coroutine); 
}

/**
 * @brief Initialize the PixelUI system, including sorting registered applications.
 */
void PixelUI::begin() {

}

/**
 * @brief Update the UI state, including animations and popups.
 * @param ms Time elapsed since the last call in milliseconds.
 */
void PixelUI::Heartbeat(uint32_t ms) 
{
    _currentTime += ms;
    m_animationManagerPtr->update(_currentTime);
    m_popupManagerPtr->updatePopups(_currentTime);
    m_coroutineSchedulerPtr->update(_currentTime);
}

/** * @brief Add an animation to the manager and start it.
 * @param animation Shared pointer to the animation to add.
 */
void PixelUI::addAnimation(std::shared_ptr<Animation> animation) {
    animation->start(_currentTime);
    m_animationManagerPtr->addAnimation(animation);
}

/**
 * @brief create and start a single-value animation.
 * @param value reference to the value to animate (fixed-point).
 * @param targetValue target value (fixed-point).
 * @param duration animation duration (milliseconds).
 * @param easing easing type.
 * @param prot animation protection status.
 */
void PixelUI::animate(int32_t& value, int32_t targetValue, uint32_t duration, EasingType easing, PROTECTION prot) {
    auto animation = std::make_shared<CallbackAnimation>(
        value, targetValue, duration, easing,
        [&value](int32_t currentValue) {
            value = currentValue;
        }
    );
    if (prot == PROTECTION::PROTECTED) m_animationManagerPtr->markProtected(animation);
    addAnimation(animation);
}

/**
 * @brief create and start a two-value animation for x and y coordinates.
 * @param x reference to the x coordinate (fixed-point).
 * @param y reference to the y coordinate (fixed-point).
 * @param targetX target x coordinate (fixed-point).
 * @param targetY target y coordinate (fixed-point).
 * @param duration animation duration (milliseconds).
 * @param easing easing type.
 * @param prot animation protection status.
 */
void PixelUI::animate(int32_t& x, int32_t& y, int32_t targetX, int32_t targetY, uint32_t duration, EasingType easing, PROTECTION prot) {
    // animate X
    auto animX = std::make_shared<CallbackAnimation>(
        x, targetX, duration, easing,
        [&x](int32_t currentValue) {
            x = currentValue;
        }
    );
    addAnimation(animX);

    // animate Y
    auto animY = std::make_shared<CallbackAnimation>(
        y, targetY, duration, easing,
        [&y](int32_t currentValue) {
            y = currentValue;
        }
    );
    addAnimation(animY);

    if (prot == PROTECTION::PROTECTED) {
        m_animationManagerPtr->markProtected(animX);
        m_animationManagerPtr->markProtected(animY);
    }
}

/**
 * @brief The main rendering loop for the UI.
 *
 * This function is responsible for drawing all UI elements to the display buffer,
 * including the current drawable content and any active popups.
 */
/**
 * @brief The main rendering loop for the UI.
 *
 * This function is responsible for drawing all UI elements to the display buffer,
 * including the current drawable content and any active popups.
 */
void PixelUI::renderer() {
    if (m_viewManagerPtr->isTransitioning()) {
        return;
    }
    
    if (getActiveAnimationCount() || isContinousRefreshEnabled()) {
        markDirty();
    }
    if (isDirty_) {
        if (!isFading_){
            this->getU8G2().clearBuffer();
            
            // current drawable content controlled by applications
            if (currentDrawable_ && isDirty_) {
                currentDrawable_->draw();
                isDirty_ = false;
            }
            
            // render popups on top of everything else
            m_popupManagerPtr->drawPopups();

            this->getU8G2().sendBuffer();
            if (m_refresh_callback) m_refresh_callback();
        } else {
            uint8_t * buf_ptr = this->getU8G2().getBufferPtr();
            uint16_t buf_len = 1024;
            for (int fade = 1; fade <= 4; fade++){
                switch (fade)
                {
                    case 1: for (uint16_t i = 0; i < buf_len; ++i)  if (i % 2 != 0) buf_ptr[i] = buf_ptr[i] & 0xAA; break;
                    case 2: for (uint16_t i = 0; i < buf_len; ++i)  if (i % 2 != 0) buf_ptr[i] = buf_ptr[i] & 0x00; break;
                    case 3: for (uint16_t i = 0; i < buf_len; ++i)  if (i % 2 == 0) buf_ptr[i] = buf_ptr[i] & 0x55; break;
                    case 4: for (uint16_t i = 0; i < buf_len; ++i)  if (i % 2 == 0) buf_ptr[i] = buf_ptr[i] & 0x00; break;
                }
                this->getU8G2().sendBuffer();
                if (m_refresh_callback) m_refresh_callback();
                m_func_delay(40);
            }
            isFading_ = false;
        }
    }
}

/**
 * @brief Show a progress popup with animated border expansion.
 * @param value Reference to the progress value that will be monitored.
 * @param minValue Minimum value of the progress range.
 * @param maxValue Maximum value of the progress range.
 * @param title Optional title for the popup.
 * @param width Width of the popup in pixels.
 * @param height Height of the popup in pixels.
 * @param duration Duration to display the popup in milliseconds.
 * @param priority Priority level of the popup (higher number = higher priority).
 */
void PixelUI::showPopupProgress(int32_t& value, int32_t minValue, int32_t maxValue, 
                               const char* title, uint16_t width, uint16_t height, 
                               uint16_t duration, uint8_t priority) {
    if (minValue >= maxValue) {
        #ifdef USE_DEBUG_OUPUT
        if (m_func_debug_print) {
            m_func_debug_print("PopupProgress: Invalid range, minValue >= maxValue");
        }
        #endif
        return;
    }
    
    // Limits on size to save memory
    if (width < 50) width = 50;
    if (width > 120) width = 120;
    if (height < 30) height = 30;
    if (height > 60) height = 60;
    
    // Limits on duration
    if (duration > 30000) duration = 30000; // Max 30 seconds
    if (duration < 1000) duration = 1000;   // Min 1 second
    
    auto popup = std::make_shared<PopupProgress>(*this, width, height, value, minValue, maxValue, title, duration, priority);
    m_popupManagerPtr->addPopup(popup);
    markDirty();
}

/**
 * @brief Show an informational popup with animated border expansion.
 * @param text The text content to display in the popup.
 * @param title Optional title for the popup (currently unused).
 * @param width Width of the popup in pixels.
 * @param height Height of the popup in pixels.
 * @param duration Duration to display the popup in milliseconds.
 * @param priority Priority level of the popup (higher number = higher priority).
 */
void PixelUI::showPopupInfo(const char* text, const char* title, uint16_t width, uint16_t height, 
                            uint16_t duration, uint8_t priority) {
    if (!text) return;
    
    auto popup = std::make_shared<PopupInfo>(*this, width, height, text, title, duration, priority);
    m_popupManagerPtr->addPopup(popup);
    markDirty();
}

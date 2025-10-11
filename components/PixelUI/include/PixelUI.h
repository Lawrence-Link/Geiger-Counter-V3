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

#pragma once

#include "U8g2lib.h"
#include "core/animation/animation.h"
#include "core/coroutine/Coroutine.h"
#include "ui/IDrawable.h"
#include "core/CommonTypes.h"

/**
 * @class IInputHandler
 * @brief An interface for handling input events.
 */
class IInputHandler{
public:
    virtual bool handleInput(InputEvent event) = 0;
    virtual ~IInputHandler() = default;
};

using InputCallback = std::function<bool(InputEvent)>;
using DelayFunction = void (*)(uint32_t);

class ViewManager;
class PopupManager;
class CoroutineScheduler;

/**
 * @class PixelUI
 * @brief The main class for the PixelUI framework.
 *
 * This class serves as the central hub for managing UI state, rendering,
 * animations, and input handling.
 */
class PixelUI
{
public:
    PixelUI(U8G2& u8g2);
    ~PixelUI() = default;

    /**
     * @brief Initializes the UI framework.
     */
    void begin();

    void addCoroutine(std::shared_ptr<Coroutine> coroutine);
    void removeCoroutine(std::shared_ptr<Coroutine> coroutine);
    void clearAllCoroutines() { m_coroutineSchedulerPtr->clear(); }
    
    size_t getActiveCoroutineCount() { return m_coroutineSchedulerPtr->getActiveCount(); }

    /**
     * @brief Main update loop to be called periodically.
     * @param ms Time elapsed since the last call.
     */
    void Heartbeat(uint32_t ms);
    
    // animation related functions.
    
    /**
     * @brief Creates and starts a single-value animation.
     * @param value Reference to the value to animate.
     * @param targetValue The final value.
     * @param duration Duration of the animation.
     * @param easing Easing function to use.
     * @param prot Protection status.
     */
    void animate(int32_t& value, int32_t targetValue, uint32_t duration, EasingType easing = EasingType::LINEAR, PROTECTION prot = PROTECTION::NOT_PROTECTED);
    
    /**
     * @brief Creates and starts a two-value animation.
     * @param x Reference to the x-coordinate.
     * @param y Reference to the y-coordinate.
     * @param targetX The final x-coordinate.
     * @param targetY The final y-coordinate.
     * @param duration Duration of the animation.
     * @param easing Easing function to use.
     * @param prot Protection status.
     */
    void animate(int32_t& x, int32_t& y, int32_t targetX, int32_t targetY, uint32_t duration, EasingType easing = EasingType::LINEAR, PROTECTION prot = PROTECTION::NOT_PROTECTED);
    
    /**
     * @brief Adds a custom animation to the manager.
     * @param animation The animation to add.
     */
    void addAnimation(std::shared_ptr<Animation> animation);
    
    /**
     * @brief Marks an animation as protected, preventing it from being cleared.
     * @param animation The animation to protect.
     */
    void markAnimationProtected(std::shared_ptr<Animation> animation) { m_animationManagerPtr->markProtected(animation); }
    
    /**
     * @brief Clears all unprotected animations.
     */
    void clearUnprotectedAnimations() { m_animationManagerPtr->clearUnprotected(); }
    
    /**
     * @brief Clears all animations.
     */
    void clearAllAnimations() { m_animationManagerPtr->clear(); }

    uint32_t getCurrentTime() const { return _currentTime; }

    // setters
    void setRefreshCallback(std::function<void()> function) { if (function) m_refresh_callback = function; }
    void setInputCallback(InputCallback callback) { if(callback) inputCallback_ = callback; }
    void setContinousDraw(bool isEnabled) { continousMode_ = isEnabled; };
    void setDelayFunction(DelayFunction func) {if (func) m_func_delay = func; }
    
    // TBD:
    // void setDebugPrintFunction(void (*func)(const char*)) { if (func) m_func_debug_print = func; }

    #ifdef USE_DEBUG_OUPUT
        void debugPrint(const char* msg);
    #endif
    
    // getters
    U8G2& getU8G2() const { return u8g2_; }
    std::shared_ptr<AnimationManager> getAnimationManPtr() { return m_animationManagerPtr; }
    std::shared_ptr<PopupManager> getPopupManagerPtr() { return m_popupManagerPtr; }
    std::shared_ptr<ViewManager> getViewManagerPtr() const { return m_viewManagerPtr; }

    // popup related functions

    /**
     * @brief Shows an informational popup.
     * @param text The text content.
     * @param title Optional title.
     * @param width Popup width.
     * @param height Popup height.
     * @param duration Display duration.
     * @param priority Popup priority.
     */
    void showPopupInfo(const char* text, const char* title = "", uint16_t width = 80, uint16_t height = 30, uint16_t duration = 3000, uint8_t priority = 0);

    /**
     * @brief Shows an popup contain 4 digits.
     * @param text The text content.
     * @param title Optional title.
     * @param width Popup width.
     * @param height Popup height.
     * @param duration Display duration.
     * @param priority Popup priority.
     * @param update_cb function callback when value changed.
     */
    void showPopupValue4Digits(
        int32_t& value,
        const char* title = "", 
        uint16_t width = 80, 
        uint16_t height = 30, 
        uint16_t duration = 3000, 
        uint8_t priority = 0, 
        std::function<void(int32_t val)> update_cb = nullptr);
    
    /**
     * @brief Shows a progress popup.
     * @param value A reference to the progress value.
     * @param minValue Minimum value.
     * @param maxValue Maximum value.
     * @param title Optional title.
     * @param width Popup width.
     * @param height Popup height.
     * @param duration Display duration.
     * @param priority Popup priority.
     * @param update_cb function callback when value changed.
     */
    void showPopupProgress(int32_t& value, 
        int32_t minValue, 
        int32_t maxValue, 
        const char* title, 
        uint16_t width = 100, 
        uint16_t height = 40, 
        uint16_t duration = 3000, 
        uint8_t priority = 0, 
        std::function<void(int32_t val)> update_cb = nullptr);

    /**
     * @brief Marks the display buffer as dirty, forcing a redraw.
     */
    void markDirty() { isDirty_ = true; }
    
    /**
     * @brief Marks the UI as fading out.
     */
    void markFading();

    bool handleInput(InputEvent event) {
        if (inputCallback_) return inputCallback_(event);
        return false;
    }
    
    /**
     * @brief The main rendering function.
     */
    void renderer();
    friend class ViewManager;
    friend class PopupManager;
protected:

    void setDrawable(std::shared_ptr<IDrawable> drawable) { currentDrawable_ = drawable; }
    bool isFading() const { return isFading_; }
private:
    U8G2& u8g2_;

    std::shared_ptr<AnimationManager> m_animationManagerPtr;
    std::shared_ptr<ViewManager> m_viewManagerPtr;
    std::shared_ptr<PopupManager> m_popupManagerPtr;
    std::shared_ptr<CoroutineScheduler> m_coroutineSchedulerPtr;

    uint32_t _currentTime = 0;
    std::shared_ptr<IDrawable> currentDrawable_;

    bool isDirty_ = false;
    bool isFading_ = false;
    bool continousMode_ = false;

    std::function<void()> m_refresh_callback = nullptr;
    DelayFunction m_func_delay = nullptr;
    InputCallback inputCallback_ = nullptr;

    bool isContinousRefreshEnabled() const { return continousMode_; }
    void (*m_func_debug_print)(const char*) = nullptr;
    uint32_t getActiveAnimationCount() const { return m_animationManagerPtr->activeCount(); }
    uint8_t m_fadeStep = 0;
    uint32_t m_lastFadeTime = 0;
};

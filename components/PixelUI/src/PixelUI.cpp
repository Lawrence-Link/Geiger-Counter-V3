/*
 * PixelUI.cpp - PixelUI main class implementation
 *
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

#include "PixelUI.h"
#include "core/TimeUtils.h"
#if PIXELUI_USE_POPUP_VALUE_DIGITS
#include "ui/Popup/PopupValueDigits.h"
#endif
#include <inttypes.h>

/**
 * @brief Construct a PixelUI instance with a U8G2 display reference
 * @param u8g2 Reference to the U8G2 display object
 *
 * Initializes core subsystems: ViewManager, AnimationManager, PopupManager, and CoroutineScheduler.
 */
PixelUI::PixelUI(U8G2& u8g2)
    : u8g2_(u8g2)
      , displayWidth_(0U)
      , displayHeight_(0U)
      , displayBufferSize_(0U)
      , canvas_(u8g2, 0, 0)
#if PIXELUI_USE_POPUP
      , m_popupManager(*this)
#endif
#if PIXELUI_USE_COROUTINE
      , m_coroutineScheduler(*this)
#endif
#if PIXELUI_USE_FOCUS
      , m_focusManager(*this)
#endif
      , _currentTime(0)
      , m_viewManager(*this) {
    m_viewManager.attachInputRouter();
}

PixelUI::~PixelUI() = default;

/**
 * @brief Add a coroutine to the scheduler
 * @param coroutine Non-owning pointer to the Coroutine object
 */
void PixelUI::addCoroutine(Coroutine* coroutine) { 
#if PIXELUI_USE_COROUTINE
    m_coroutineScheduler.addCoroutine(coroutine);
#else
    (void)coroutine;
#endif
}

/**
 * @brief Remove a coroutine from the scheduler
 * @param coroutine Non-owning pointer to the Coroutine object
 */
void PixelUI::removeCoroutine(Coroutine* coroutine) { 
#if PIXELUI_USE_COROUTINE
    m_coroutineScheduler.removeCoroutine(coroutine);
#else
    (void)coroutine;
#endif
}

/**
 * @brief Initializes display-dependent PixelUI state after U8G2 setup
 */
void PixelUI::begin() {
    displayWidth_ = u8g2_.getDisplayWidth();
    displayHeight_ = u8g2_.getDisplayHeight();
    displayBufferSize_ = (displayWidth_ != 0U && displayHeight_ != 0U)
            ? u8g2_GetBufferSize(u8g2_.getU8g2())
            : 0U;
    canvas_.setDisplaySize(displayWidth_, displayHeight_);
}

/**
 * @brief Compatibility wrapper for hosts that inject ticks outside an ISR.
 */
void PixelUI::heartbeat(uint32_t ms) {
    pendingTickMs_.fetch_add(ms, etl::memory_order_relaxed);
}

/** @brief PixelUI::setTaskNotifyFromISR. */
void PixelUI::setTaskNotifyFromISR(IsrTaskNotifyFunction function, void* context) {
    m_taskNotifyContext_ = context;
    m_taskNotifyFromISR_ = function;
}

/** @brief PixelUI::tickFromISR. */
void PixelUI::tickFromISR(uint32_t elapsedMs) {
    if (elapsedMs == 0U) return;

    const uint32_t previous =
        pendingTickMs_.fetch_add(elapsedMs, etl::memory_order_relaxed);
    if (previous == 0U && m_taskNotifyFromISR_ != nullptr) {
        m_taskNotifyFromISR_(m_taskNotifyContext_);
    }
}

/** @brief PixelUI::process. */
bool PixelUI::process() {
    const uint32_t elapsedMs =
        pendingTickMs_.exchange(0U, etl::memory_order_relaxed);
    _currentTime += elapsedMs;

    if (m_deadlineScheduler.update(_currentTime)) markDirty();

#if PIXELUI_USE_ANIMATION
    const bool animationIsDue =
        m_animationManager.nextWakeupMs(_currentTime, 1U) == 0U;
    const bool animationWasActive = m_animationManager.activeCount() != 0U;
#endif
#if PIXELUI_USE_POPUP
    if (elapsedMs != 0U ||
        m_popupManager.nextWakeupMs(_currentTime, 1U) == 0U) {
        m_popupManager.updatePopups(_currentTime);
    }
#endif
#if PIXELUI_USE_COROUTINE
    if (elapsedMs != 0U || m_coroutineScheduler.nextWakeupMs(_currentTime) == 0U) {
        m_coroutineScheduler.update(_currentTime);
    }
#endif
#if PIXELUI_USE_ANIMATION
    if (elapsedMs != 0U || animationIsDue) {
        m_animationManager.update(_currentTime);
        // Completion removes the animation after writing its final value.
        if (animationWasActive) markDirty();
    }
#endif
    return elapsedMs != 0U;
}

/** @brief PixelUI::handler. */
uint32_t PixelUI::handler(uint32_t frameIntervalMs) {
    renderer();
    return nextWakeupMs(frameIntervalMs);
}

/** @brief PixelUI::setRenderRequestCallback. */
void PixelUI::setRenderRequestCallback(VoidCallback function) {
    m_render_request_callback = etl::move(function);
    if (m_render_request_callback && hasPendingFrame()) {
        m_render_request_callback();
    }
}

/** @brief PixelUI::markDirty. */
void PixelUI::markDirty() {
    if (isDirty_) return;

    isDirty_ = true;
    if (m_render_request_callback) {
        m_render_request_callback();
    }
}

bool PixelUI::animateCallback(
    int32_t startValue,
    int32_t endValue,
    uint32_t duration,
    EasingType easing,
    ValueCallback callback,
    PROTECTION protection,
    AnimationHandle* handle) {
#if PIXELUI_USE_ANIMATION
    const bool added = m_animationManager.emplace(
        startValue,
        endValue,
        duration,
        easing,
        etl::move(callback),
        protection,
        getCurrentTime(),
        handle);
    if (added) markDirty();
    return added;
#else
    (void)startValue;
    (void)duration;
    (void)easing;
    (void)protection;
    if (handle != nullptr) *handle = INVALID_ANIMATION_HANDLE;
    if (callback) callback(endValue);
    return true;
#endif
}

/**
 * @brief Animate a single integer value with optional protection
 */
bool PixelUI::animate(int32_t& value, int32_t targetValue, uint32_t duration,
                      EasingType easing, PROTECTION prot,
                      AnimationHandle* handle) {

    return animateCallback(
        value,
        targetValue,
        duration,
        easing,
        [&value](int32_t currentValue) { value = currentValue; },
        prot,
        handle);
}

/** @brief PixelUI::scrollCanvasBy. */
bool PixelUI::scrollCanvasBy(int32_t deltaY) {
    if (!canvas_.camera().scrollBy(deltaY)) return false;
    markDirty();
    return true;
}

/** @brief PixelUI::scrollCanvasTo. */
bool PixelUI::scrollCanvasTo(int32_t y) {
    if (!canvas_.camera().setY(y)) return false;
    markDirty();
    return true;
}

/** @brief PixelUI::ensureCanvasVisible. */
bool PixelUI::ensureCanvasVisible(int32_t top, int32_t bottom) {
    if (!canvas_.camera().ensureVisible(top, bottom)) return false;
    markDirty();
    return true;
}

bool PixelUI::animateCanvasTo(int32_t y, uint32_t duration,
                              EasingType easing, PROTECTION protection) {
    const int32_t target = etl::max(0, etl::min(y, canvas_.camera().maxY()));
    const int32_t start = canvas_.camera().storedY();
    if (start == target) return false;
    return animateCallback(
        start, target, duration, easing,
        [this](int32_t value) { canvas_.camera().setY(value); }, protection);
}

/**
 * @brief Animate two integer values simultaneously
 */
bool PixelUI::animate(int32_t& x, int32_t& y, int32_t targetX, int32_t targetY,
                      uint32_t duration, EasingType easing, PROTECTION prot) {

#if PIXELUI_USE_ANIMATION
    if (m_animationManager.available() < 2U) {
        return false;
    }

    const bool xAdded = animateCallback(
        x, targetX, duration, easing, [&x](int32_t val) { x = val; }, prot);
    const bool yAdded = animateCallback(
        y, targetY, duration, easing, [&y](int32_t val) { y = val; }, prot);
    return xAdded && yAdded;
#else
    (void)duration;
    (void)easing;
    (void)prot;
    x = targetX;
    y = targetY;
    return true;
#endif
}
/**
 * @brief Add a widget to the FocusManager
 * @param w Pointer to the widget to add
 */
bool PixelUI::addWidgetToFocusManager(IWidget* w) {
#if PIXELUI_USE_FOCUS
    return m_focusManager.addWidget(w);
#else
    (void)w;
    return false;
#endif
}
/**
 * @brief Clear all widgets from the FocusManager
 */
void PixelUI::clearFocusManager() {
#if PIXELUI_USE_FOCUS
    m_focusManager.clear();
#endif
}

/** @brief PixelUI::getFocusedWidgetCount. */
size_t PixelUI::getFocusedWidgetCount() const {
#if PIXELUI_USE_FOCUS
    return m_focusManager.widgetCount();
#else
    return 0U;
#endif
}
/**
 * @brief Handle input event
 * @param event InputEvent to handle
 *
 * Passes the input event to the FocusManager and the registered input callback.
 */

void PixelUI::handleInput(InputEvent event) {
#if PIXELUI_USE_FOCUS
    if (m_focusManager.handleInput(event)) return;
#endif
    if (inputCallback_) inputCallback_(event);
}

/**
 * @brief Render the current drawable and active popups
 *
 * Handles optional fading effects and calls the refresh callback if set.
 */
bool PixelUI::needsHeartbeat() const {
    if (pendingTickMs_.load(etl::memory_order_relaxed) != 0U ||
        isFading_ || continousMode_) return true;
    if (m_deadlineScheduler.nextWakeupMs(_currentTime) !=
        PixelUITime::NO_WAKEUP) return true;
#if PIXELUI_USE_ANIMATION
    if (m_animationManager.nextWakeupMs(_currentTime, 1U) !=
        PixelUITime::NO_WAKEUP) return true;
#endif
#if PIXELUI_USE_POPUP
    if (m_popupManager.nextWakeupMs(_currentTime, 1U) !=
        PixelUITime::NO_WAKEUP) return true;
#endif
#if PIXELUI_USE_COROUTINE
    if (m_coroutineScheduler.nextWakeupMs(_currentTime) !=
        PixelUITime::NO_WAKEUP) return true;
#endif
    return false;
}

/** @brief PixelUI::nextWakeupMs. */
uint32_t PixelUI::nextWakeupMs(uint32_t periodicTickMs) const {
    periodicTickMs = PixelUITime::normalizeInterval(periodicTickMs);
#if PIXELUI_ENABLE_TICKLESS
    return calculateNextWakeupMs(periodicTickMs);
#else
    return periodicTickMs;
#endif
}

/** @brief PixelUI::calculateNextWakeupMs. */
uint32_t PixelUI::calculateNextWakeupMs(uint32_t frameIntervalMs) const {
    if (pendingTickMs_.load(etl::memory_order_relaxed) != 0U || isDirty_) {
        return 0U;
    }

    uint32_t next = PixelUITime::NO_WAKEUP;
    const uint32_t currentTime = _currentTime;

    next = PixelUITime::earlier(
        next, m_deadlineScheduler.nextWakeupMs(currentTime));

    if (continousMode_) {
        next = frameIntervalMs;
    }
    if (isFading_) {
        const uint32_t fadeDelay = (m_fadeStep == 0U)
            ? 0U
            : PixelUITime::untilDeadline(currentTime, m_lastFadeTime + 40U);
        next = PixelUITime::earlier(next, fadeDelay);
    }
#if PIXELUI_USE_ANIMATION
    next = PixelUITime::earlier(
        next, m_animationManager.nextWakeupMs(currentTime, frameIntervalMs));
#endif
#if PIXELUI_USE_POPUP
    next = PixelUITime::earlier(
        next, m_popupManager.nextWakeupMs(currentTime, frameIntervalMs));
#endif
#if PIXELUI_USE_COROUTINE
    next = PixelUITime::earlier(
        next, m_coroutineScheduler.nextWakeupMs(currentTime));
#endif
    return next;
}

/** @brief PixelUI::hasPendingFrame. */
bool PixelUI::hasPendingFrame() const {
    return isDirty_ || isFading_ || continousMode_;
}

/** @brief PixelUI::renderer. */
bool PixelUI::renderer() {
    if (m_viewManager.isTransitionCommitInProgress()) return false;

    // Backward compatibility: legacy hosts may still call only renderer(). New
    // hosts should call process() explicitly before renderer() on the UI task.
    process();

    if (isContinousRefreshEnabled()) markDirty();

    if (!isFading_ && !isDirty_) return false;

    if (!isFading_) {
        // Consume the request before drawing so an invalidation raised by draw()
        // remains pending for the next pass instead of being lost.
        isDirty_ = false;
        u8g2_.clearBuffer();
        canvas_.beginFrame();
        if (currentDrawable_) currentDrawable_->draw();
        if (canvas_.endFrame()) markDirty();
#if PIXELUI_USE_POPUP
        m_popupManager.drawPopups();
#endif
#if PIXELUI_USE_FOCUS
        m_focusManager.draw();
#endif
        u8g2_.sendBuffer();
        if (m_refresh_callback) m_refresh_callback();
        return true;
    } else {
        if (m_fadeStep == 0) {
            isDirty_ = false;
            u8g2_.clearBuffer();
            canvas_.beginFrame();
            if (currentDrawable_) currentDrawable_->draw();
            if (canvas_.endFrame()) markDirty();
#if PIXELUI_USE_POPUP
            m_popupManager.drawPopups();
#endif
            u8g2_.sendBuffer();
            if (m_refresh_callback) m_refresh_callback();
            m_fadeStep = 1;
            m_lastFadeTime = getCurrentTime();
            return true;
        }
        if (m_fadeStep >= 1 && m_fadeStep <= 4) {
            if (getCurrentTime() - m_lastFadeTime < 40) return false;

            isDirty_ = false;
            uint8_t *buf_ptr = u8g2_.getBufferPtr();
            const uint16_t buf_len = displayBufferSize_;
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
            if (m_fadeStep > 4) {
                isFading_ = false;
                m_fadeStep = 0;
                m_viewManager.completePendingEnter();
                markDirty();
            }
            return true;
        }
    }
    return false;
}

/**
 * @brief Show a progress popup
 */
bool PixelUI::showPopupProgress(
    ValueEditorBinding binding, const NumericRange& range,
    NumericFormatter formatter, const char* title,
    uint16_t width, uint16_t height, uint16_t duration,
    ValueEditPolicy policy, PopupProgressMode mode) {
#if PIXELUI_USE_POPUP_PROGRESS
    if (!binding.valid()) return false;
    if (width < 50) width = 50;
    if (width > 120) width = 120;
    if (height < 30) height = 30;
    if (height > 60) height = 60;
    if (duration > 30000) duration = 30000;
    if (duration < 1000) duration = 1000;

    if (m_popupManager.enqueueProgress(
            width, height, binding, range, formatter, title, duration,
            nullptr, policy, mode)) {
        markDirty();
        return true;
    }
    return false;
#else
    (void)binding;
    (void)range;
    (void)formatter;
    (void)title;
    (void)width;
    (void)height;
    (void)duration;
    (void)policy;
    (void)mode;
    return false;
#endif
}

bool PixelUI::showPopupProgress(int32_t& value, int32_t minValue, int32_t maxValue,
                                const char* title, uint16_t width, uint16_t height,
                                uint16_t duration, ValueCallback update_cb,
                                PopupProgressMode mode) {
#if PIXELUI_USE_POPUP_PROGRESS
    NumericRange range;
    if (!NumericRange::tryCreate(minValue, maxValue, 1, range)) return false;
    if (width < 50) width = 50;
    if (width > 120) width = 120;
    if (height < 30) height = 30;
    if (height > 60) height = 60;
    if (duration > 30000) duration = 30000;
    if (duration < 1000) duration = 1000;
    if (m_popupManager.enqueueProgress(
            width, height, ValueEditorBinding::reference(value), range,
            NumericFormatter{}, title, duration, etl::move(update_cb),
            ValueEditPolicy::CommitOnConfirm, mode)) {
        markDirty();
        return true;
    }
    return false;
#else
    (void)value;
    (void)minValue;
    (void)maxValue;
    (void)title;
    (void)width;
    (void)height;
    (void)duration;
    (void)update_cb;
    (void)mode;
    return false;
#endif
}

/**
 * @brief Show an informational popup
 */
bool PixelUI::showPopupInfo(const char* text, const char* title,
                            uint16_t width, uint16_t height,
                            uint16_t duration) {
#if PIXELUI_USE_POPUP_INFO
    if (!text) return false;
    if (m_popupManager.enqueueInfo(width, height, text, title, duration)) {
        markDirty();
        return true;
    }
    return false;
#else
    (void)text;
    (void)title;
    (void)width;
    (void)height;
    (void)duration;
    return false;
#endif
}

bool PixelUI::showPopupKeyboard(
    char* output, size_t outputCapacity, uint16_t width, uint16_t height,
    uint16_t duration, VoidCallback commitCallback) {
#if PIXELUI_USE_POPUP_KEYBOARD
    if (!PopupKeyboard::isValidLayout(width, height) ||
        width > getDisplayWidth() || height > getDisplayHeight() ||
        duration > 30000U) {
        return false;
    }
    if (m_popupManager.enqueueKeyboard(
            width, height, output, outputCapacity, duration,
            etl::move(commitCallback))) {
        markDirty();
        return true;
    }
    return false;
#else
    (void)output;
    (void)outputCapacity;
    (void)width;
    (void)height;
    (void)duration;
    (void)commitCallback;
    return false;
#endif
}

/**
 * @brief Show a fixed-width value editor popup
 */
bool PixelUI::showPopupValueDigits(
    ValueEditorBinding binding, uint8_t digitCount,
    const char* title, uint16_t width, uint16_t height,
    uint16_t duration, ValueEditPolicy policy) {
#if PIXELUI_USE_POPUP_VALUE_DIGITS
    if (!binding.valid() ||
        !PopupValueDigits::isValidLayout(width, height, digitCount) ||
        width > getDisplayWidth() || height > getDisplayHeight()) {
        return false;
    }
    if (width > 120) width = 120;
    if (height > 60) height = 60;
    if (duration > 30000) duration = 30000;
    if (duration < 1000) duration = 1000;
    if (m_popupManager.enqueueValueDigits(
            width, height, binding, digitCount, title, duration,
            nullptr, policy)) {
        markDirty();
        return true;
    }
    return false;
#else
    (void)binding;
    (void)digitCount;
    (void)title;
    (void)width;
    (void)height;
    (void)duration;
    (void)policy;
    return false;
#endif
}

bool PixelUI::showPopupValueDigits(int32_t& value, uint8_t digitCount,
                                   const char* title, uint16_t width,
                                   uint16_t height, uint16_t duration,
                                   ValueCallback update_cb) {
#if PIXELUI_USE_POPUP_VALUE_DIGITS
    if (!PopupValueDigits::isValidLayout(width, height, digitCount) ||
        width > getDisplayWidth() || height > getDisplayHeight()) {
        return false;
    }
    if (width > 120) width = 120;
    if (height > 60) height = 60;
    if (duration > 30000) duration = 30000; 
    if (duration < 1000) duration = 1000;

    if (m_popupManager.enqueueValueDigits(
            width, height, value, digitCount, title, duration, etl::move(update_cb))) {
        markDirty();
        return true;
    }
    return false;
#else
    (void)value;
    (void)digitCount;
    (void)title;
    (void)width;
    (void)height;
    (void)duration;
    (void)update_cb;
    return false;
#endif
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

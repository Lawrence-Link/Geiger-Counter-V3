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

#include "core/animation/animation.h"
#include "core/TimeUtils.h"

#define MUL_FIXED(a, b) ((int64_t)(a) * (b) >> SHIFT_BITS)

/**
 * @brief Easing function implementation, all calculations use fixed-point numbers.
 * @param type Easing type.
 * @param t Normalized time, fixed-point.
 * @return Normalized progress, fixed-point.
 */
int32_t EasingCalculator::calculate(EasingType type, int32_t t) 
{
    switch (type) 
    {
        case EasingType::LINEAR:            return t;
        case EasingType::EASE_IN_QUAD:      return MUL_FIXED(t, t);
        case EasingType::EASE_OUT_QUAD:     
        {
            int32_t inverse_t = FIXED_POINT_ONE - t;
            return FIXED_POINT_ONE - MUL_FIXED(inverse_t, inverse_t);
        }
        case EasingType::EASE_IN_OUT_QUAD:  
            if (t < (FIXED_POINT_ONE / 2)) {
                return MUL_FIXED(2 * t, t);
            } else {
                int32_t val = (FIXED_POINT_ONE * 2) - (2 * t);
                return FIXED_POINT_ONE - MUL_FIXED(val, val) / 2;
            }
        case EasingType::EASE_IN_CUBIC:     return MUL_FIXED(MUL_FIXED(t, t), t);
        case EasingType::EASE_OUT_CUBIC:    
        {
            int32_t inverse_t = FIXED_POINT_ONE - t;
            return FIXED_POINT_ONE - MUL_FIXED(MUL_FIXED(inverse_t, inverse_t), inverse_t);
        }
        case EasingType::EASE_IN_OUT_CUBIC: 
            if (t < (FIXED_POINT_ONE / 2)) {
                return MUL_FIXED(4 * t, MUL_FIXED(t, t));
            } else {
                int32_t val = (FIXED_POINT_ONE * 2) - (2 * t);
                return FIXED_POINT_ONE - MUL_FIXED(MUL_FIXED(val, val), val) / 2;
            }
        case EasingType::EASE_OUT_BOUNCE:   return easeOutBounce(t);
        default:                            return t;
    }
}

/**
 * @brief EaseOutBounce easing function, implemented with fixed-point numbers.
 * @param t Normalized time, fixed-point.
 * @return Normalized progress, fixed-point.
 */
int32_t EasingCalculator::easeOutBounce(int32_t t) {
    // Use macros to simplify fixed-point operations
    #define FIXED_DIV(a, b) ((int64_t)(a) * FIXED_POINT_ONE / (b))
    #define FIXED_MUL(a, b) ((int64_t)(a) * (b) / FIXED_POINT_ONE)

    constexpr int32_t n1 = FIXED_POINT_ONE * 121 / 16;
    constexpr int32_t d1 = FIXED_POINT_ONE * 11 / 4;
    
    if (t < FIXED_DIV(FIXED_POINT_ONE, d1)) 
    {
        return FIXED_MUL(n1, FIXED_MUL(t, t));
    } 
    else if (t < FIXED_DIV(2 * FIXED_POINT_ONE, d1))
    {
        t -= FIXED_DIV(3 * FIXED_POINT_ONE / 2, d1);
        return FIXED_MUL(n1, FIXED_MUL(t, t)) + 3 * FIXED_POINT_ONE / 4;
    }
    else if (t < FIXED_DIV(5 * FIXED_POINT_ONE / 2, d1))
    {
        t -= FIXED_DIV(9 * FIXED_POINT_ONE / 4, d1);
        return FIXED_MUL(n1, FIXED_MUL(t, t)) + 15 * FIXED_POINT_ONE / 16;
    }
    else 
    {
        t -= FIXED_DIV(21 * FIXED_POINT_ONE / 8, d1);
        return FIXED_MUL(n1, FIXED_MUL(t, t)) + 63 * FIXED_POINT_ONE / 64;
    }
}

/*
@brief Start the animation, set the start time, and mark it as active.
@param currentTime Current time (milliseconds).
*/
void Animation::start(uint32_t currentTime) {
    _startTime = currentTime;
    _progress = 0;
    _isActive = true;
}

/*
@brief Stop the animation, mark it as inactive.
*/
void Animation::stop() {
    _isActive = false;
}

/*
@brief Update the animation progress based on the current time.
@param currentTime Current time (milliseconds).
@return True if the animation is still running, otherwise false.
*/
bool Animation::update(uint32_t currentTime) {
    if (!_isActive) return false;
    
    uint32_t elapsed = currentTime - _startTime;
    
    // a small gap to prevent from jitter at the end
    bool completed = (elapsed >= _duration) || (elapsed >= _duration - 1);
    
    int32_t t = completed ? FIXED_POINT_ONE : 
                ((int64_t)elapsed * FIXED_POINT_ONE) / _duration;
    
    _progress = EasingCalculator::calculate(_easing, t);
    
    if (completed) {
        _progress = FIXED_POINT_ONE;
        _isActive = false;
        return false;
    }
    return true;
}

uint32_t Animation::nextWakeupMs(
    uint32_t currentTime, uint32_t frameIntervalMs) const {
    if (!_isActive) return PixelUITime::NO_WAKEUP;

    const uint32_t elapsed = currentTime - _startTime;
    if (elapsed >= _duration || (_duration > 0U && elapsed >= _duration - 1U)) {
        return 0U;
    }

    return PixelUITime::earlier(
        PixelUITime::normalizeInterval(frameIntervalMs), _duration - elapsed);
}

bool AnimationManager::emplace(
    int32_t startValue,
    int32_t endValue,
    uint32_t duration,
    EasingType easing,
    UpdateCallback callback,
    PROTECTION protection,
    uint32_t currentTime,
    AnimationHandle* handle) {
    if (handle != nullptr) *handle = INVALID_ANIMATION_HANDLE;
    if (_animations.full() || !callback) {
        return false;
    }

    const AnimationHandle assignedHandle = nextHandle();
    CallbackAnimation& animation = _animations.emplace_back(
        assignedHandle,
        startValue,
        endValue,
        duration,
        easing,
        etl::move(callback));
    animation.setProtected(protection == PROTECTION::PROTECTED);
    animation.start(currentTime);
    if (handle != nullptr) *handle = assignedHandle;
    return true;
}

/** @brief AnimationManager::nextHandle. */
AnimationHandle AnimationManager::nextHandle() {
    AnimationHandle candidate = nextHandle_++;
    if (nextHandle_ == INVALID_ANIMATION_HANDLE) ++nextHandle_;
    if (candidate == INVALID_ANIMATION_HANDLE) candidate = nextHandle_++;
    return candidate;
}

/** @brief AnimationManager::cancel. */
bool AnimationManager::cancel(AnimationHandle handle) {
    if (handle == INVALID_ANIMATION_HANDLE) return false;
    for (auto iterator = _animations.begin(); iterator != _animations.end(); ++iterator) {
        if (iterator->handle() != handle) continue;
        _animations.erase(iterator);
        return true;
    }
    return false;
}

/*
@brief Update all active animations based on the current time.
@param currentTime Current time (milliseconds).
*/
void AnimationManager::update(uint32_t currentTime) {

    if (_animations.empty()) {
        return;
    }

    auto writePos = _animations.begin();
    for (auto readPos = _animations.begin(); readPos != _animations.end(); ++readPos) {
        if (readPos->update(currentTime)) {
            if (writePos != readPos) {
                *writePos = etl::move(*readPos);
            }
            ++writePos;
        } else {
            // If the animation is complete and not protected, remove it
            if (!readPos->isProtected()) {
                // Ensure the animation is unprotected after completion so it can be cleared next time
                readPos->setProtected(false);
            }
        }
    }
    _animations.erase(writePos, _animations.end());
}

/*
@brief clear all animations in the manager.
*/
void AnimationManager::clear(){
    _animations.clear();
}

/*
@brief clean all unprotected animations in the manager.
*/
void AnimationManager::clearUnprotected() {
    if (_animations.empty()) {
        return;
    }
    
    auto writePos = _animations.begin();
    for (auto readPos = _animations.begin(); readPos != _animations.end(); ++readPos) {
        if (readPos->isProtected()) {
            if (writePos != readPos) {
                *writePos = etl::move(*readPos);
            }
            ++writePos;
        }
    }
    _animations.erase(writePos, _animations.end());
}

/*
@brief clean all protection marks from all animations.
*/
void AnimationManager::clearAllProtectionMarks() {
    for (auto& animation : _animations) {
        animation.setProtected(false);
    }
}

/*
@brief acquire number of current active count
@return (size_t) number of current active count
*/
size_t AnimationManager::activeCount() const {
    return _animations.size();
}

uint32_t AnimationManager::nextWakeupMs(
    uint32_t currentTime, uint32_t frameIntervalMs) const {
    uint32_t next = PixelUITime::NO_WAKEUP;
    for (const auto& animation : _animations) {
        next = PixelUITime::earlier(
            next, animation.nextWakeupMs(currentTime, frameIntervalMs));
    }
    return next;
}

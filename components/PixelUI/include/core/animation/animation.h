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
#include <stdint.h>
#include <stddef.h>
#include "etl/vector.h"
#include "etl/inplace_function.h"
#include "config.h"
#include "core/CommonTypes.h"

// Fixed-point shift bits used by easing and popup transitions.
#define SHIFT_BITS 12
#define FIXED_POINT_ONE (1 << SHIFT_BITS)

using AnimationHandle = uint32_t;
constexpr AnimationHandle INVALID_ANIMATION_HANDLE = 0U;

/**
 * @class EasingCalculator
 * @brief Calculate easing progress based on easing type and normalized time.
 */
class EasingCalculator {
public:
    /**
     * @brief calculate progress based on easing type and normalized time.
     * @param type easing type.
     * @param t normalized time, fixed-point.
     */
    static int32_t calculate(EasingType type, int32_t t);
private:
    /**
     * @brief ease out bounce function.
     * @param t normalized time, fixed-point.
     */
    static int32_t easeOutBounce(int32_t t);
};

/**
 * @class Animation
 * @brief Base class for animations, managing timing and easing.
 */
class Animation {
public:
/** @brief Animation. */
    Animation(uint32_t duration, EasingType easing = EasingType::LINEAR) :
        _progress(0),           
        _isActive(false),       
        _isProtected(false),    
        _easing(easing),        
        _startTime(0),          
        _duration(duration)     
    {}
    
/** @brief ~Animation. */
    ~Animation() = default;
    
/** @brief start. */
    void start(uint32_t currentTime);
/** @brief stop. */
    void stop();
/** @brief update. */
    bool update(uint32_t currentTime);
/** @brief isActive. */
    bool isActive() const { return _isActive; }
/** @brief isProtected. */
    bool isProtected() const { return _isProtected; }
/** @brief setProtected. */
    void setProtected(bool prot) { _isProtected = prot; }
/** @brief getProgress. */
    int32_t getProgress() const { return _progress; }
/** @brief nextWakeupMs. */
    uint32_t nextWakeupMs(uint32_t currentTime, uint32_t frameIntervalMs) const;

protected:
    int32_t _progress = 0;     

private:
    bool _isActive;             
    bool _isProtected = false;  
    EasingType _easing;         
    uint32_t _startTime;        
    uint32_t _duration;         
};

/**
 * @class CallbackAnimation
 * @brief Animation that calls a callback with the current value on each update.
 */
class CallbackAnimation : public Animation {
public:
/** @brief CallbackAnimation. */
    CallbackAnimation(AnimationHandle handle,
                      int32_t startVal, int32_t endVal, uint32_t duration, EasingType easing,
                      etl::inplace_function<void(int32_t), CALLBACK_STORAGE_SIZE> updateCallback)
        : Animation(duration, easing),
          _handle(handle),
          _startVal(startVal),
          _endVal(endVal),
          _updateCallback(updateCallback) {}
          
/** @brief update. */
    bool update(uint32_t currentTime) {
        bool isRunning = Animation::update(currentTime);
        if (_updateCallback) { 
            const int64_t delta =
                static_cast<int64_t>(_endVal) - _startVal;
            const int32_t currentValue = static_cast<int32_t>(
                static_cast<int64_t>(_startVal) +
                (delta * _progress) / FIXED_POINT_ONE);
            _updateCallback(currentValue);
        }
        return isRunning;
    }

private:
    AnimationHandle _handle;
    int32_t _startVal;
    int32_t _endVal;
    etl::inplace_function<void(int32_t), CALLBACK_STORAGE_SIZE> _updateCallback;

public:
/** @brief handle. */
    AnimationHandle handle() const { return _handle; }
};

/**
 * @class AnimationManager
 * @brief Owns callback animations in fixed-capacity inline storage.
 */
class AnimationManager {
public:
    using UpdateCallback = etl::inplace_function<void(int32_t), CALLBACK_STORAGE_SIZE>;

/** @brief ~AnimationManager. */
    ~AnimationManager() = default;
/** @brief emplace. */
    bool emplace(
        int32_t startValue,
        int32_t endValue,
        uint32_t duration,
        EasingType easing,
        UpdateCallback callback,
        PROTECTION protection,
        uint32_t currentTime,
        AnimationHandle* handle = nullptr);
/** @brief cancel. */
    bool cancel(AnimationHandle handle);
/** @brief update. */
    void update(uint32_t currentTime);
/** @brief clear. */
    void clear();
/** @brief clearUnprotected. */
    void clearUnprotected();
/** @brief clearAllProtectionMarks. */
    void clearAllProtectionMarks();
/** @brief activeCount. */
    size_t activeCount() const;
/** @brief available. */
    size_t available() const { return _animations.available(); }
/** @brief nextWakeupMs. */
    uint32_t nextWakeupMs(uint32_t currentTime, uint32_t frameIntervalMs) const;

private:
/** @brief nextHandle. */
    AnimationHandle nextHandle();

    etl::vector<CallbackAnimation, MAX_ANIMATION_COUNT> _animations;
    AnimationHandle nextHandle_ = 1U;
};

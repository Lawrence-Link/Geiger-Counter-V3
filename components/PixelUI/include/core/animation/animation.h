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
#include <cstdint>
#include <memory>
#include "etl/vector.h"
#include <functional>
#include "config.h"
#include "core/CommonTypes.h"
#include <mutex>

// fixed-point shift bits to support fractional values in integer arithmetic
// eg. SHIFT_BITS = 12 means 0.5f is represented as 0.5 * (1 << 12) = 2048
#define SHIFT_BITS 12
#define FIXED_POINT_ONE (1 << SHIFT_BITS)
#define FLOAT_TO_FIXED(f) ((int32_t)((f) * FIXED_POINT_ONE))

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
    Animation(uint32_t duration, EasingType easing = EasingType::LINEAR) :
        _progress(0),           
        _isActive(false),       
        _isProtected(false),    
        _easing(easing),        
        _startTime(0),          
        _duration(duration)     
    {}
    
    virtual ~Animation() = default;
    
    void start(uint32_t currentTime);
    void stop();
    virtual bool update(uint32_t currentTime);
    bool isActive() const { return _isActive; }
    bool isProtected() const { return _isProtected; }
    void setProtected(bool prot) { _isProtected = prot; }
    int32_t getProgress() const { return _progress; }

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
 * @class AnimationManager
 * @brief Manages multiple animations, updating and cleaning them up.
 */
class AnimationManager {
public:
    ~AnimationManager() = default;
    void addAnimation(std::shared_ptr<Animation> animation);
    void update(uint32_t currentTime);
    void clear();
    
    // Protection mechanism
    void markProtected(std::shared_ptr<Animation> animation);
    void clearUnprotected();
    void clearAllProtectionMarks();
    size_t activeCount() const;

private:
    etl::vector<std::shared_ptr<Animation>, MAX_ANIMATION_COUNT> _animations;
    mutable  std::recursive_mutex _mutex; 
};

/**
 * @class CallbackAnimation
 * @brief Animation that calls a callback with the current value on each update.
 */
class CallbackAnimation : public Animation {
public:
    CallbackAnimation(int32_t startVal, int32_t endVal, uint32_t duration, EasingType easing,
                      std::function<void(int32_t)> updateCallback)
        : Animation(duration, easing),
          _startVal(startVal),
          _endVal(endVal),
          _updateCallback(updateCallback) {}
          
    bool update(uint32_t currentTime) override {
        bool isRunning = Animation::update(currentTime);
        if (_updateCallback) { 
            int32_t delta = _endVal - _startVal;
            int32_t currentValue = _startVal + ((int64_t)delta * _progress) / FIXED_POINT_ONE;
            _updateCallback(currentValue);
        }
        return isRunning;
    }

private:
    int32_t _startVal;
    int32_t _endVal;
    std::function<void(int32_t)> _updateCallback;
};
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

#include "core/animation/animation.h"

// Convert float multiplication to integer multiplication and bit shift
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

    const int32_t n1 = FLOAT_TO_FIXED(7.5625f);
    const int32_t d1 = FLOAT_TO_FIXED(2.75f);
    
    if (t < FIXED_DIV(FIXED_POINT_ONE, d1)) 
    {
        return FIXED_MUL(n1, FIXED_MUL(t, t));
    } 
    else if (t < FIXED_DIV(FLOAT_TO_FIXED(2.0f), d1))
    {
        t -= FIXED_DIV(FLOAT_TO_FIXED(1.5f), d1);
        return FIXED_MUL(n1, FIXED_MUL(t, t)) + FLOAT_TO_FIXED(0.75f);
    }
    else if (t < FIXED_DIV(FLOAT_TO_FIXED(2.5f), d1))
    {
        t -= FIXED_DIV(FLOAT_TO_FIXED(2.25f), d1);
        return FIXED_MUL(n1, FIXED_MUL(t, t)) + FLOAT_TO_FIXED(0.9375f);
    }
    else 
    {
        t -= FIXED_DIV(FLOAT_TO_FIXED(2.625f), d1);
        return FIXED_MUL(n1, FIXED_MUL(t, t)) + FLOAT_TO_FIXED(0.984375f);
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
bool Animation::update(uint32_t currentTime){
    if (!_isActive) {
        return false;
    }

    uint32_t elapsed = currentTime - _startTime;
    
    // If the animation is complete, force the progress to the fixed-point representation of 1.0
    bool completed = (elapsed >= _duration);
    
    // Calculate the normalized time 't', using 64-bit integer to avoid overflow
    int32_t t = completed ? FIXED_POINT_ONE : ((int64_t)elapsed * FIXED_POINT_ONE) / _duration;

    // Calculate the final progress using the easing function
    _progress = EasingCalculator::calculate(_easing, t);

    if (completed) {
        _isActive = false;
        return false;
    }
    return true;
}

/*
@brief Add a new animation to the manager.
@param animation Shared pointer to the Animation object to be added.
*/
void AnimationManager::addAnimation(std::shared_ptr<Animation> animation) {
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    if (!animation || _animations.full()) {
        return;
    }
    _animations.push_back(animation);
}

/*
@brief Update all active animations based on the current time.
@param currentTime Current time (milliseconds).
*/
void AnimationManager::update(uint32_t currentTime) {
    std::lock_guard<std::recursive_mutex> lock(_mutex);

    if (_animations.empty()) {
        return;
    }

    auto writePos = _animations.begin();
    for (auto readPos = _animations.begin(); readPos != _animations.end(); ++readPos) {
        if ((*readPos)->update(currentTime)) {
            if (writePos != readPos) {
                *writePos = std::move(*readPos);
            }
            ++writePos;
        } else {
            // If the animation is complete and not protected, remove it
            if (!readPos->get()->isProtected()) {
                // Ensure the animation is unprotected after completion so it can be cleared next time
                readPos->get()->setProtected(false);
            }
        }
    }
    _animations.erase(writePos, _animations.end());
}

/*
@brief clear all animations in the manager.
*/
void AnimationManager::clear(){
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    _animations.clear();
}

/*
@brief mark a animation as protected, preventing it from being cleared.
@param animation the animation to be marked as protected.
*/
void AnimationManager::markProtected(std::shared_ptr<Animation> animation) {
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    if (animation) {
        animation->setProtected(true);
    }
}

/*
@brief clean all unprotected animations in the manager.
*/
void AnimationManager::clearUnprotected() {
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    if (_animations.empty()) {
        return;
    }
    
    auto writePos = _animations.begin();
    for (auto readPos = _animations.begin(); readPos != _animations.end(); ++readPos) {
        if (readPos->get()->isProtected()) {
            if (writePos != readPos) {
                *writePos = std::move(*readPos);
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
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    for (const auto& anim_ : _animations) {
        anim_->setProtected(false);
    }
}

/*
@brief acquire number of current active count
@return (size_t) number of current active count
*/
size_t AnimationManager::activeCount() const {
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    return _animations.size();
}

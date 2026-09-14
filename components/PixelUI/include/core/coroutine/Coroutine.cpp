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

#include "core/coroutine/Coroutine.h"
#include "PixelUI.h"
#include "core/TimeUtils.h"

/**
 * @brief Constructor of the coroutine class
 */
Coroutine::Coroutine(CoroutineFunction func) 
    : function_(func) {
    context_.state = CoroutineState::CREATED;
}

/**
 * @brief Start a coroutine
 */
void Coroutine::start() {
    if (context_.state == CoroutineState::CREATED) {
        context_.state = CoroutineState::RUNNING;
        context_.pc = 0;
    }
}

/**
 * @brief Resumes a coroutine
 * @param currentTime uint32_t, current timestamp of the system
 */
void Coroutine::resume(uint32_t currentTime, bool animationActive) {
    if (context_.state == CoroutineState::SUSPENDED &&
        shouldRun(currentTime, animationActive)) {
        context_.state = CoroutineState::RUNNING;
    }
    
    if (context_.state == CoroutineState::RUNNING) {
        function_(context_); 
    }
}

/**
 * @brief Reset a coroutine to its initial state
 */
void Coroutine::reset() {
    context_.state = CoroutineState::CREATED;
    context_.pc = 0;
    context_.waitUntil = 0;
    context_.waitReason = CoroutineWaitReason::NONE;
}


/**
 * @brief check whether a coroutine should run or not
 */
bool Coroutine::shouldRun(uint32_t currentTime, bool animationActive) const {
    if (context_.state == CoroutineState::FINISHED) {
        return false;
    }
    
    if (context_.state == CoroutineState::SUSPENDED) {
        switch (context_.waitReason) {
            case CoroutineWaitReason::DELAY:
                return PixelUITime::deadlineReached(currentTime, context_.waitUntil);
            case CoroutineWaitReason::ANIMATION:
                return !animationActive;
            case CoroutineWaitReason::NONE:
                return true;
        }
    }
    
    return context_.state == CoroutineState::RUNNING;
}

uint32_t Coroutine::nextWakeupMs(
    uint32_t currentTime, bool animationActive) const {
    if (context_.state == CoroutineState::RUNNING) return 0U;
    if (context_.state != CoroutineState::SUSPENDED) {
        return PixelUITime::NO_WAKEUP;
    }

    switch (context_.waitReason) {
        case CoroutineWaitReason::DELAY:
            return PixelUITime::untilDeadline(currentTime, context_.waitUntil);
        case CoroutineWaitReason::ANIMATION:
            return animationActive ? PixelUITime::NO_WAKEUP : 0U;
        case CoroutineWaitReason::NONE:
            return 0U;
    }
    return PixelUITime::NO_WAKEUP;
}
/**
 * @brief Coroutine Scheduler Constructor
 * @param ui A reference to the PixelUI object, potentially used for UI operations or context 
 * within the coroutines.
 */
CoroutineScheduler::CoroutineScheduler(PixelUI& ui) : ui_(ui) {}

/**
 * @brief Add a coroutine to the scheduler
 * * Adds a new coroutine to the list of scheduled items and immediately starts its execution.
 * @param coroutine A non-owning pointer to the Coroutine object to be added.
 */
void CoroutineScheduler::addCoroutine(Coroutine* coroutine) { 
    // Check if the pointer is null
    if (!coroutine) return; 
    
    // Add the coroutine to the internal list
    coroutines_.push_back(coroutine);
}

/**
 * @brief Remove a coroutine from the scheduler
 * * Removes the specified coroutine by comparing pointers.
 * @param coroutine A non-owning pointer to the Coroutine object to be removed.
 */
void CoroutineScheduler::removeCoroutine(Coroutine* coroutine) { 
    // Use the erase-remove idiom to safely remove the matching element from the vector.
    coroutines_.erase(
        etl::remove(coroutines_.begin(), coroutines_.end(), coroutine),
        coroutines_.end()
    );
}

/**
 * @brief Update all coroutines
 * * This is the core function of the scheduler. It checks and executes coroutines that 
 * are ready to run. It is typically called in every frame or time step of the main loop.
 * @param currentTime The current timestamp (usually milliseconds or frame count), 
 * used to determine if a coroutine's resume condition has been met.
 */
void CoroutineScheduler::update(uint32_t currentTime) {
    // Remove finished coroutines
    // Use the erase-remove_if idiom to remove all coroutines for which isFinished() returns true.
    coroutines_.erase(
        etl::remove_if(coroutines_.begin(), coroutines_.end(),
            [](const Coroutine* coro) { 
                return coro->isFinished(); // Check if the coroutine has completed its work
            }),
        coroutines_.end()
    );
    
    // Execute coroutines that should run
    for (auto& coroutine : coroutines_) {
        const bool animationActive = ui_.activeAnimationCount() != 0U;
        // shouldRun checks if the coroutine meets its run condition (e.g., a delay time has elapsed)
        if (coroutine->shouldRun(currentTime, animationActive)) {
            // Resume the coroutine's execution
            coroutine->resume(currentTime, animationActive);
        }
    }

    coroutines_.erase(
        etl::remove_if(coroutines_.begin(), coroutines_.end(),
            [](const Coroutine* coro) { return coro->isFinished(); }),
        coroutines_.end());
}

uint32_t CoroutineScheduler::nextWakeupMs(uint32_t currentTime) const {
    const bool animationActive = ui_.activeAnimationCount() != 0U;
    uint32_t next = PixelUITime::NO_WAKEUP;
    for (const Coroutine* coroutine : coroutines_) {
        if (coroutine != nullptr) {
            next = PixelUITime::earlier(
                next, coroutine->nextWakeupMs(currentTime, animationActive));
        }
    }
    return next;
}

/**
 * @brief Clear all coroutines
 * * Stops and clears all coroutines currently being scheduled or waiting for execution.
 */
void CoroutineScheduler::clear() {
    // Empty the internal vector storing the coroutines
    coroutines_.clear();
}

/**
 * @brief Get the count of active coroutines
 * * @return The total number of coroutines currently in the scheduler, either running or suspended.
 */
size_t CoroutineScheduler::getActiveCount() const {
    return coroutines_.size();
}

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

#include "core/coroutine/Coroutine.h"
#include "PixelUI.h"
#include <algorithm>

/**
 * @brief Constructor of the coroutine class
 */
Coroutine::Coroutine(CoroutineFunction func, PixelUI& ui) 
    : function_(func), ui_(ui) {
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
 * @brief Rusume a coroutine
 * @param currentTime uint32_t, current timestamp of the system
 */
void Coroutine::resume(uint32_t currentTime) {
    if (context_.state == CoroutineState::SUSPENDED && currentTime >= context_.waitUntil) {
        context_.state = CoroutineState::RUNNING;
    }
    
    if (context_.state == CoroutineState::RUNNING) {
        function_(context_, ui_);
    }
}

/**
 * @brief check whether a coroutine should run or not
 */
bool Coroutine::shouldRun(uint32_t currentTime) const {
    if (context_.state == CoroutineState::FINISHED) {
        return false;
    }
    
    if (context_.state == CoroutineState::SUSPENDED) {
        if (currentTime >= context_.waitUntil) {
            // The coroutine should resume once the delay period has elapsed
            return true;
        }
        return false;
    }
    
    return context_.state == CoroutineState::RUNNING;
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
 * @param coroutine A shared pointer to the Coroutine object to be added.
 */
void CoroutineScheduler::addCoroutine(std::shared_ptr<Coroutine> coroutine) {
    // Check if the pointer is null
    if (!coroutine) return; 
    
    // Add the coroutine to the internal list
    coroutines_.push_back(coroutine);
    
    // Start the coroutine (e.g., set its initial state or execute the first step)
    coroutine->start();
}

/**
 * @brief Remove a coroutine from the scheduler
 * * Removes the specified coroutine by comparing the shared pointers.
 * @param coroutine A shared pointer to the Coroutine object to be removed.
 */
void CoroutineScheduler::removeCoroutine(std::shared_ptr<Coroutine> coroutine) {
    // Use the erase-remove idiom to safely remove the matching element from the vector.
    coroutines_.erase(
        std::remove(coroutines_.begin(), coroutines_.end(), coroutine),
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
        std::remove_if(coroutines_.begin(), coroutines_.end(),
            [](const std::shared_ptr<Coroutine>& coro) {
                return coro->isFinished(); // Check if the coroutine has completed its work
            }),
        coroutines_.end()
    );
    
    // Execute coroutines that should run
    for (auto& coroutine : coroutines_) {
        // shouldRun checks if the coroutine meets its run condition (e.g., a delay time has elapsed)
        if (coroutine->shouldRun(currentTime)) {
            // Resume the coroutine's execution
            coroutine->resume(currentTime);
        }
    }
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
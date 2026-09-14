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

#include <etl/vector.h>
#include <etl/inplace_function.h>
#include "config.h"
#include "core/TimeUtils.h"

class PixelUI;

enum class CoroutineWaitReason : uint8_t {
    NONE,
    DELAY,
    ANIMATION
};

/**
 * @brief Coroutine state emulation
 */
enum class CoroutineState {
    CREATED,    
    RUNNING,    
    SUSPENDED,  
    FINISHED   
};

/**
 * @brief coroutine context
 */
struct CoroutineContext {
    uint32_t pc = 0;           // program counter
    uint32_t waitUntil = 0;
    uint32_t localData[8] = {0};
    CoroutineState state = CoroutineState::CREATED;
    CoroutineWaitReason waitReason = CoroutineWaitReason::NONE;
};

/**
 * @brief definition for the CoroutineFunction type
 */
using CoroutineFunction = etl::inplace_function<void(CoroutineContext&), CALLBACK_STORAGE_SIZE>;

/**
 * @brief Coroutine class
 */
class Coroutine {
public:
    /**
     * @brief Creates a coroutine backed by a fixed callback.
     * @param func Callback implementing the coroutine state machine.
     */
    Coroutine(CoroutineFunction func);
    /** @brief Destroys the coroutine and its fixed context. */
    ~Coroutine() = default;

    /** @brief Moves a newly created coroutine to the running state. */
    void start();
    /**
     * @brief Resumes execution when the current wait condition is satisfied.
     * @param currentTime Current monotonic time in milliseconds.
     * @param animationActive Whether animations are currently active.
     */
    void resume(uint32_t currentTime, bool animationActive = false);
    /** @brief Returns the coroutine to its initial program-counter state. */
    void reset(); 
    /** @return Whether the coroutine reached its terminal state. */
    bool isFinished() const { return context_.state == CoroutineState::FINISHED; }
    /** @return Whether the coroutine is ready to execute at the supplied time. */
    bool shouldRun(uint32_t currentTime, bool animationActive = false) const;
    /** @return Delay until execution, or NO_WAKEUP when no wakeup is pending. */
    uint32_t nextWakeupMs(uint32_t currentTime, bool animationActive) const;
    
    /** @return Mutable state used by the coroutine callback. */
    CoroutineContext& getContext() { return context_; }
    /** @return Read-only state used by the coroutine callback. */
    const CoroutineContext& getContext() const { return context_; }

private:
    CoroutineFunction function_;
    CoroutineContext context_;
};

/**
 * @brief Coroutine Scheduler
 */
class CoroutineScheduler {
public:
    /** @brief Creates a scheduler that observes the supplied UI clock and animations. */
    explicit CoroutineScheduler(PixelUI& ui);
    
    /** @brief Adds a non-owning coroutine pointer to the scheduler. */
    void addCoroutine(Coroutine* coroutine);
    /** @brief Removes a coroutine pointer from the scheduler without destroying it. */
    void removeCoroutine(Coroutine* coroutine);
    /** @brief Resumes ready coroutines and removes those that have finished. */
    void update(uint32_t currentTime);
    /** @return Earliest wakeup delay among scheduled coroutines. */
    uint32_t nextWakeupMs(uint32_t currentTime) const;
    /** @brief Removes all coroutine pointers without destroying their owners. */
    void clear();
    
    /** @return Number of coroutine pointers currently scheduled. */
    size_t getActiveCount() const;

private:
    etl::vector<Coroutine*, MAX_COROUTINE_NUM> coroutines_;
    PixelUI& ui_;
};

#define CORO_BEGIN(ctx) switch((ctx).pc) { case 0:
    
#define CORO_END(ctx) \
    (ctx).waitReason = CoroutineWaitReason::NONE; \
    (ctx).state = CoroutineState::FINISHED; return; }

#define CORO_YIELD(ctx, line) do { (ctx).pc = line; return; case line:; } while(0)

#define CORO_DELAY(ctx, ui, ms, line) do { \
    (ctx).waitUntil = (ui).getCurrentTime() + (ms); \
    (ctx).pc = (line); \
    (ctx).state = CoroutineState::SUSPENDED; \
    (ctx).waitReason = CoroutineWaitReason::DELAY; \
    return; \
    case (line): \
    if (!PixelUITime::deadlineReached((ui).getCurrentTime(), (ctx).waitUntil)) return; \
    (ctx).waitReason = CoroutineWaitReason::NONE; \
    (ctx).state = CoroutineState::RUNNING; \
} while(0)

#define CORO_WAIT_ANIMATION(ctx, ui, line) do { \
    (ctx).pc = line; \
    (ctx).state = CoroutineState::SUSPENDED; \
    (ctx).waitReason = CoroutineWaitReason::ANIMATION; \
    return; \
    case line: \
    if ((ui).activeAnimationCount() > 0) return; \
    (ctx).waitReason = CoroutineWaitReason::NONE; \
    (ctx).state = CoroutineState::RUNNING; \
} while(0)

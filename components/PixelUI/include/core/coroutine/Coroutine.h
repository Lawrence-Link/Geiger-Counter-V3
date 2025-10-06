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

#include <functional>
#include <memory>

class PixelUI;

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
};

/**
 * @brief definition for the CoroutineFunction type
 */
using CoroutineFunction = std::function<void(CoroutineContext&, PixelUI&)>;

/**
 * @brief Coroutine class
 */
class Coroutine {
public:
    Coroutine(CoroutineFunction func, PixelUI& ui);
    ~Coroutine() = default;

    void start();
    void resume(uint32_t currentTime);
    bool isFinished() const { return context_.state == CoroutineState::FINISHED; }
    bool shouldRun(uint32_t currentTime) const;
    
    CoroutineContext& getContext() { return context_; }
    const CoroutineContext& getContext() const { return context_; }

private:
    CoroutineFunction function_;
    CoroutineContext context_;
    PixelUI& ui_;
};

/**
 * @brief Coroutine Scheduler
 */
class CoroutineScheduler {
public:
    explicit CoroutineScheduler(PixelUI& ui);
    
    void addCoroutine(std::shared_ptr<Coroutine> coroutine);
    void removeCoroutine(std::shared_ptr<Coroutine> coroutine);
    void update(uint32_t currentTime);
    void clear();
    
    size_t getActiveCount() const;

private:
    std::vector<std::shared_ptr<Coroutine>> coroutines_;
    PixelUI& ui_;
};

#define CORO_BEGIN(ctx) switch((ctx).pc) { case 0:
    
#define CORO_END(ctx) (ctx).state = CoroutineState::FINISHED; return; }

#define CORO_YIELD(ctx, line) do { (ctx).pc = line; return; case line:; } while(0)

#define CORO_DELAY(ctx, ui, ms, line) do { \
    (ctx).waitUntil = (ui).getCurrentTime() + (ms); \
    (ctx).pc = (line); \
    (ctx).state = CoroutineState::SUSPENDED; \
    return; \
    case (line): \
    if ((ui).getCurrentTime() < (ctx).waitUntil) return; \
    (ctx).state = CoroutineState::RUNNING; \
} while(0)

// (可选但推荐) 同样修复 CORO_WAIT_ANIMATION 宏
#define CORO_WAIT_ANIMATION(ctx, ui, line) do { \
    (ctx).pc = line; \
    (ctx).state = CoroutineState::SUSPENDED; \
    return; \
    case line: \
    if ((ui).getActiveAnimationCount() > 0) return; \
    (ctx).state = CoroutineState::RUNNING; \
} while(0)

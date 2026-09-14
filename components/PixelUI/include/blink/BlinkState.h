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

#include "core/scheduling/UiDeadlineScheduler.h"
#include <stdint.h>

class PixelUI;

class BlinkState final : private UiDeadlineSource {
public:
    /**
     * @brief Creates blink state managed by the UI deadline scheduler.
     * @param ui UI instance that supplies time, redraw requests, and scheduling.
     * @param intervalMs Time in milliseconds between visibility changes.
     */
    explicit BlinkState(PixelUI& ui, uint32_t intervalMs = 500U);

    /** @brief Unregisters this state from the deadline scheduler. */
    ~BlinkState() override = default;

    /** @brief Starts blinking from the visible state. */
    void start();

    /** @brief Stops blinking and makes the controlled content invisible. */
    void stop();

    /** @brief Continues blinking until the content next becomes visible. */
    void stopWhenVisible();

    /**
     * @brief Changes the interval between visibility changes.
     * @param intervalMs New interval in milliseconds; zero is normalized to one.
     */
    void setInterval(uint32_t intervalMs);

    /** @return Whether the controlled content is currently visible. */
    bool isVisible() const { return visible_; }

    /** @return Whether blinking is currently active. */
    bool isRunning() const { return running_; }

    /** @return The normalized blink interval in milliseconds. */
    uint32_t interval() const { return intervalMs_; }

private:
    /**
     * @brief Reports the delay until the next visibility change.
     * @param currentTime Current monotonic time in milliseconds.
     * @return Delay in milliseconds, or PixelUITime::NO_WAKEUP when stopped.
     */
    uint32_t nextWakeupMs(uint32_t currentTime) const override;

    /**
     * @brief Applies all visibility changes due at the supplied time.
     * @param currentTime Current monotonic time in milliseconds.
     * @return Whether the final visibility differs from its previous value.
     */
    bool update(uint32_t currentTime) override;

    PixelUI& ui_;
    uint32_t intervalMs_;
    uint32_t lastToggleMs_;
    bool visible_ = true;
    bool running_ = false;
    bool stopWhenVisible_ = false;
};

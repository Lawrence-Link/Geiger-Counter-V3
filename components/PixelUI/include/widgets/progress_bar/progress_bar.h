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

#include "widgets/IWidget.h"
#include "core/animation/animation.h"

class ProgressBar : public IWidget {
public:
    /** @brief Creates a progress bar whose value is clamped to 0..100 percent. */
    ProgressBar(PixelUI& ui, int16_t x, int16_t y,
                uint16_t width, uint16_t height,
                int32_t percent = 0, bool useAnimation = true);
    /** @brief Cancels any animation owned by this progress bar. */
    ~ProgressBar() override;

    /** @brief Loads the progress bar, animating from zero when enabled. */
    void onLoad() override;
    /** @brief Cancels animation and restores the requested progress. */
    void onOffload() override;

    /** @brief Sets progress, using the configured animation behavior. */
    void setPercent(int32_t percent);
    /** @brief Sets progress without animation. */
    void setPercentImmediate(int32_t percent);
    /** @return Requested progress in the range 0..100. */
    int32_t getPercent() const { return percent_; }

    /** @brief Updates the progress bar position. */
    void setPosition(int16_t x, int16_t y);
    /** @brief Updates the progress bar size. */
    void setSize(uint16_t width, uint16_t height);

private:
    static constexpr int32_t MAX_PERCENT = 100;
    static constexpr uint32_t ANIMATION_DURATION_MS = 300U;

    PixelUI& ui_;
    int32_t percent_ = 0;
    int32_t displayedPercent_ = 0;
    bool useAnimation_ = true;
    AnimationHandle animation_ = INVALID_ANIMATION_HANDLE;

    static int32_t clampPercent(int32_t percent);
    void cancelAnimation();
    void updateDisplayedPercent(
        PROTECTION protection = PROTECTION::NOT_PROTECTED);

    /** @brief Draws the rounded outline and current fill. */
    void drawSelf(const WidgetRenderContext& context) override;
    /** @brief Returns the camera-aware drawing surface. */
    Canvas& display() override;
};

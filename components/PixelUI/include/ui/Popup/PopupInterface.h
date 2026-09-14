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
#include "core/CommonTypes.h"

/**
 * @enum PopupState
 * @brief The state of a popup's animation.
 */
enum class PopupState {
    APPEARING, // Popup is expanding
    SHOWING,   // Popup is displayed statically
    CLOSING    // Popup is shrinking
};

/**
 * @class IPopup
 * @brief An interface for all popup types.
 */
class IPopup {
public:
/** @brief IPopup. */
    IPopup() = default;
/** @brief ~IPopup. */
    virtual ~IPopup() {}

/** @brief update. */
    virtual bool update(uint32_t currentTime) = 0;
/** @brief draw. */
    virtual void draw() = 0;
/** @brief handleInput. */
    virtual bool handleInput(InputEvent event) = 0;
/** @brief getDuration. */
    virtual uint16_t getDuration() const = 0;
/** @brief nextWakeupMs. */
    virtual uint32_t nextWakeupMs(
        uint32_t currentTime, uint32_t frameIntervalMs) const {
        (void)currentTime;
        return (frameIntervalMs == 0U) ? 1U : frameIntervalMs;
    }
};

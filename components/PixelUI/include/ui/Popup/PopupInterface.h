/*
 * Copyright (C) 2025 Lawrence Link
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include <cstdint>
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
    IPopup() = default;
    virtual ~IPopup() {}

    virtual bool update(uint32_t currentTime) = 0;
    virtual void draw() = 0;
    virtual bool handleInput(InputEvent event) = 0;
    virtual uint8_t getPriority() const = 0;
    virtual uint16_t getDuration() const = 0;
};
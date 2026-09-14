#pragma once

#include "core/CommonTypes.h"

class IInputHandler {
public:
    /**
     * @brief Handles an input event.
     * @param event Input event to process.
     * @return true if the event was consumed; false to allow further handling.
     */
    virtual bool handleInput(InputEvent event) = 0;

    /** @brief Destroys the input handler. */
    virtual ~IInputHandler() = default;
};

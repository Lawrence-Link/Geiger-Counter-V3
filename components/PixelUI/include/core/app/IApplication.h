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

#include "core/Callbacks.h"
#include "core/IInputHandler.h"
#include "ui/IDrawable.h"
#include <etl/inplace_function.h>
#include "config.h"
#include <stdint.h>

class PixelUI;

class IApplication : public IDrawable, public IInputHandler {
    friend class ViewManager;

public:

    using ExitCallback = etl::inplace_function<void(), CALLBACK_STORAGE_SIZE>; // Exit callback function
    /**
     * @brief Creates an application and configures viewport scrolling.
     * @param useVerticalScroll Whether unhandled vertical input moves the canvas camera.
     */
    explicit IApplication(bool useVerticalScroll = false)
        : useVerticalScroll(useVerticalScroll) {}
    /** @brief Destroys the application through its virtual interface. */
    virtual ~IApplication() = default;

    // Enable the global canvas camera for this application. Unhandled UP/DOWN
    // input scrolls by one viewport; an app may consume it and position the
    // camera itself (ListView does this to keep its selection visible).
    bool useVerticalScroll = false;

private:
    bool m_enterTransitionEnabled = false;

public:

    // Enables the standard horizontal slide applied when onEnter begins.
    // Disabled by default so existing applications keep their current behavior.
    /** @param enabled Whether entering this application uses the horizontal slide. */
    void setEnterTransitionEnabled(bool enabled) {
        m_enterTransitionEnabled = enabled;
    }
    /** @return Whether the standard horizontal entry transition is enabled. */
    bool isEnterTransitionEnabled() const { return m_enterTransitionEnabled; }

    // Called when the app is pushed to the top of the stack
    /** @param exitCallback Callback used to request removal of this application. */
    virtual void onEnter(ExitCallback exitCallback) { m_exitCallback = exitCallback; }
    // Called when the app is exited
    /** @brief Handles application exit; the default implementation does nothing. */
    virtual void onExit() {}; 
    // Called when the app is paused by another app being pushed on top
    /** @brief Handles pausing; the default implementation does nothing. */
    virtual void onPause() {};   
    // Called when the top app is popped and this app resumes
    /** @brief Handles resuming; the default implementation does nothing. */
    virtual void onResume() {}; 

protected:
    /** @brief Invokes the stored callback to request that this application be popped. */
    void requestExit() {
        if (m_exitCallback) {
            m_exitCallback();
        }
    }

private:
    /** @brief Clears the non-owning exit callback before application destruction. */
    void clearExitCallback() { m_exitCallback = nullptr; }

    ExitCallback m_exitCallback;
};

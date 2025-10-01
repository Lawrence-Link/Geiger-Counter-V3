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

#include "PixelUI.h"
#include <functional>

class IApplication : public IDrawable, public IInputHandler {
public:

    using ExitCallback = std::function<void()>; // Exit callback function
    virtual ~IApplication() = default;

    // Called when the app is pushed to the top of the stack
    virtual void onEnter(ExitCallback exitCallback) { m_exitCallback = exitCallback; }
    // Called when the app is exited
    virtual void onExit() {}; 
    // Called when the app is paused by another app being pushed on top
    virtual void onPause() {};   
    // Called when the top app is popped and this app resumes
    virtual void onResume() {};  

protected:
    void requestExit() {
        if (m_exitCallback) {
            m_exitCallback();
        }
    }

private:
    ExitCallback m_exitCallback;
};

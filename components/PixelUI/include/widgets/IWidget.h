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
#include "core/CommonTypes.h"

class IWidget{
private:
    bool focusable = false;
    bool Selected = false;
    FocusBox focus;

    void setSelected(bool state) { Selected = state; }
    bool isSelected() const { return Selected; }

public:
    virtual ~IWidget() = default;
    virtual void draw() = 0;

    /**
     * @brief Handles an input event.
     * @param event The event code.
     * @return true if the widget has completed its input handling and wants to return control to the FocusManager.
     */
    virtual bool handleEvent(InputEvent event) { return false; }

    virtual void onLoad() = 0;
    virtual void onOffload () = 0;

    /**
     * @brief Triggers the onSelect action.
     * @return true if the widget wants to take over input control, false otherwise.
     */
    virtual bool onSelect() { return false; }

    bool isFocusable() { return focusable; }
    void setFocusable(bool state) { focusable = state; }

    void setFocusBox(const FocusBox& pos) {focus = pos;}
    FocusBox getFocusBox() { return focus; }
};
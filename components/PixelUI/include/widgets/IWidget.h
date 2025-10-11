/*
 * Copyright (C) 2025 Lawrence Link
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
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
protected:
    // activation related
    bool m_is_active = false;
    uint32_t m_last_interaction_time = 0;

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
    virtual void onOffload() = 0;

    /**
     * @brief Triggers the onSelect action.
     * @return true if the widget wants to take over input control, false otherwise.
     */
    virtual bool onSelect() { return false; }

    /**
     * @brief Gets the timeout duration in milliseconds.
     * @return Timeout duration. Return 0 to disable timeout.
     */
    virtual uint32_t getTimeout() const { return 0; }

    /**
     * @brief Called when the widget becomes active (takes over input control).
     */
    virtual void onActivate(uint32_t currentTime) {
        m_is_active = true;
        m_last_interaction_time = currentTime;
    }

    /**
     * @brief Called when the widget is deactivated (returns control to FocusManager).
     */
    virtual void onDeactivate() {
        m_is_active = false;
    }

    /**
     * @brief Updates the last interaction time.
     */
    void updateInteractionTime(uint32_t currentTime) {
        m_last_interaction_time = currentTime;
    }

    /**
     * @brief Gets the last interaction time.
     */
    uint32_t getLastInteractionTime() const {
        return m_last_interaction_time;
    }

    /**
     * @brief Checks if the widget is currently active.
     */
    bool isActive() const { return m_is_active; }

    /**
     * @brief Checks if the widget is focusable
     */
    bool isFocusable() { return focusable; }
    void setFocusable(bool state) { focusable = state; }

    void setFocusBox(const FocusBox& pos) {focus = pos;}
    FocusBox getFocusBox() { return focus; }
};
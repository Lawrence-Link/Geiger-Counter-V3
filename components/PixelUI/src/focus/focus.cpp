/**
 * @file focus.cpp
 * @brief Manages focus and animation for UI widgets.
 */

#include "focus/focus.h"
#include <iostream>

void FocusManager::clearActiveWidget() {
    m_activeWidget = nullptr;
}

/**
 * @brief Moves the focus to the next widget in the list.
 *
 * This function increments the focus index, wrapping around to the beginning
 * of the list. If the focus was previously inactive (index == -1), it will
 * be set to the first widget. This also initiates a new animation and resets
 * the inactivity timer.
 */

void FocusManager::moveNext() {
    if (m_Widgets.empty()) {
        index = -1;
        m_state = State::IDLE;
        return;
    }
    m_ui.getAnimationManPtr()->clearUnprotected();
    int old_index = index;
    if (index == -1) {
        index = 0;
    } else {
        index = (index + 1) % m_Widgets.size();
    }

    if (index != old_index) {
        m_state = State::ANIMATING;
        last_focus_change_time = m_ui.getCurrentTime(); // reset timer

        // Start the animation. The starting values for m_current_focus_box
        // will be automatically inherited from the last drawn state.
        FocusBox target = m_Widgets[index]->getFocusBox();
        m_ui.animate(m_current_focus_box.x, target.x, 100, EasingType::EASE_OUT_QUAD);
        m_ui.animate(m_current_focus_box.y, target.y, 100, EasingType::EASE_OUT_QUAD);
        m_ui.animate(m_current_focus_box.w, target.w, 100, EasingType::EASE_OUT_QUAD);
        m_ui.animate(m_current_focus_box.h, target.h, 100, EasingType::EASE_OUT_QUAD);
    }
}

/**
 * @brief Moves the focus to the previous widget in the list.
 *
 * This function decrements the focus index, wrapping around to the end
 * of the list. If the focus was previously inactive (index == -1), it will
 * be set to the last widget. This also initiates a new animation and resets
 * the inactivity timer.
 */
void FocusManager::movePrev() {
    if (m_Widgets.empty()) {
        index = -1;
        m_state = State::IDLE;
        return;
    }
    m_ui.getAnimationManPtr()->clearUnprotected();
    int old_index = index;
    if (index == -1) {
        index = m_Widgets.size() - 1;
    } else {
        index = (index - 1 + m_Widgets.size()) % m_Widgets.size();
    }

    if (index != old_index) {
        m_state = State::ANIMATING;
        last_focus_change_time = m_ui.getCurrentTime();

        // Start the animation. The starting values for m_current_focus_box
        // will be automatically inherited from the last drawn state.
        FocusBox target = m_Widgets[index]->getFocusBox();
        m_ui.animate(m_current_focus_box.x, target.x, 100, EasingType::EASE_OUT_QUAD);
        m_ui.animate(m_current_focus_box.y, target.y, 100, EasingType::EASE_OUT_QUAD);
        m_ui.animate(m_current_focus_box.w, target.w, 100, EasingType::EASE_OUT_QUAD);
        m_ui.animate(m_current_focus_box.h, target.h, 100, EasingType::EASE_OUT_QUAD);
    }
}

/**
 * @brief Triggers the onSelect action of the currently focused widget.
 * If the widget is focusable, it can take over the input stream.
 */
void FocusManager::selectCurrent() {
    last_focus_change_time = m_ui.getCurrentTime();
    if (index >= 0 && index < (int)m_Widgets.size()) {
        IWidget* selectedWidget = m_Widgets[index];
        // If the widget's onSelect returns true, it wants to take over input.
        if (selectedWidget->onSelect()) {
            m_activeWidget = selectedWidget;
            // The focus manager is now idle, as the widget has control.
            m_state = State::IDLE;
        }
    }
}

/**
 * @brief Draws the focus box and handles all animation logic.
 *
 * This function is responsible for:
 * - Checking for a focus timeout (e.g., after 2.5 seconds of inactivity).
 * - Initiating the shrink animation when a timeout occurs.
 * - Checking for animation completion.
 * - Drawing the focus box based on the current animation state.
 */
void FocusManager::draw() {
    // Check if the focus has been on the same widget for more than 2.5 seconds.
    if (m_state != State::IDLE && m_ui.getCurrentTime() - last_focus_change_time > 2500) {
        // If not already shrinking, initiate the shrink animation.
        if (m_state != State::ANIMATING_SHRINK) {
            m_state = State::ANIMATING_SHRINK;

            // Get the center point of the current focus box.
            int32_t center_x = m_current_focus_box.x + m_current_focus_box.w / 2;
            int32_t center_y = m_current_focus_box.y + m_current_focus_box.h / 2;

            // Start the shrink animation for width and height,
            // while animating x,y to keep the center stable.
            m_ui.animate(m_current_focus_box.w, 0, 100, EasingType::EASE_IN_QUAD);
            m_ui.animate(m_current_focus_box.h, 0, 100, EasingType::EASE_IN_QUAD);
            m_ui.animate(m_current_focus_box.x, center_x, 100, EasingType::EASE_IN_QUAD);
            m_ui.animate(m_current_focus_box.y, center_y, 100, EasingType::EASE_IN_QUAD);
        }
    }

    if (m_state != State::IDLE) {
        // If in shrink animation, check for its completion.
        if (m_state == State::ANIMATING_SHRINK) {
            // Animation is considered complete when width and height are close to 0.
            if (m_current_focus_box.w <= 1 && m_current_focus_box.h <= 1) {
                m_state = State::IDLE;
                index = -1;
                return; // Animation finished, return immediately without drawing.
            }
        }
        
        U8G2& u8g2 = m_ui.getU8G2();
        u8g2.setDrawColor(2);

        // Get the target focus box for the animation endpoint.
        if (index >= 0 && index < (int)m_Widgets.size()) {
            m_target_focus_box = m_Widgets[index]->getFocusBox();
        }

        // Check if the primary animation is complete.
        if (m_state == State::ANIMATING && m_current_focus_box == m_target_focus_box) {
            m_state = State::FOCUSED;
        }

        // Draw the focus box based on the current state.
        switch (m_state) {
            case State::ANIMATING:
            case State::FOCUSED:
            case State::ANIMATING_SHRINK:
                u8g2.drawBox(m_current_focus_box.x, m_current_focus_box.y, m_current_focus_box.w, m_current_focus_box.h);
                break;
            default:
                break;
        }
        u8g2.setDrawColor(1);
    }
}

/**
 * @brief Adds a new widget to the focus manager's list.
 * @param w A pointer to the IWidget to add.
 *
 * If this is the first widget added, it becomes the initial focus.
 */
void FocusManager::addWidget(IWidget* w) {
    m_Widgets.push_back(w);
}

/**
 * @brief Removes a widget from the focus manager's list.
 * @param w A pointer to the IWidget to remove.
 *
 * This function also handles the focus index if the removed widget
 * was the last one or the currently focused one.
 */
void FocusManager::removeWidget(IWidget* w) {
    auto it = etl::find(m_Widgets.begin(), m_Widgets.end(), w);
    if (it != m_Widgets.end()) {
        m_Widgets.erase(it);
    }

    if (m_Widgets.empty()) {
        index = -1;
        m_state = State::IDLE; // Reset state
    } else if (index >= (int)m_Widgets.size()) {
        index = m_Widgets.size() - 1;
        m_state = State::FOCUSED; // Reset state
    }
}
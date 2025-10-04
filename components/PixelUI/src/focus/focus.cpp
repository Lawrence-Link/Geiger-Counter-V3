/*
 * Copyright (C) 2025 Lawrence Link
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "focus/focus.h"
#include <iostream>

void FocusManager::clearActiveWidget() {
    if (m_activeWidget) {
        m_activeWidget->onDeactivate();
        m_activeWidget = nullptr;
        
        // 恢复焦点显示状态
        if (index >= 0 && index < (int)m_Widgets.size()) {
            m_state = State::FOCUSED;
            last_focus_change_time = m_ui.getCurrentTime();
            
            // 同步焦点框位置到当前控件
            m_current_focus_box = m_Widgets[index]->getFocusBox();
            m_target_focus_box = m_current_focus_box;
        } else {
            m_state = State::IDLE;
        }
    }
}

void FocusManager::checkActiveWidgetTimeout() {
    if (!m_activeWidget) {
        return;
    }

    uint32_t timeout = m_activeWidget->getTimeout();
    if (timeout == 0) {
        return;
    }

    uint32_t currentTime = m_ui.getCurrentTime();
    uint32_t lastInteractionTime = m_activeWidget->getLastInteractionTime();

    if (currentTime - lastInteractionTime >= timeout) {
        clearActiveWidget();
        m_ui.markDirty();
    }
}

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
        last_focus_change_time = m_ui.getCurrentTime();

        FocusBox target = m_Widgets[index]->getFocusBox();
        m_ui.animate(m_current_focus_box.x, target.x, 100, EasingType::EASE_OUT_QUAD);
        m_ui.animate(m_current_focus_box.y, target.y, 100, EasingType::EASE_OUT_QUAD);
        m_ui.animate(m_current_focus_box.w, target.w, 100, EasingType::EASE_OUT_QUAD);
        m_ui.animate(m_current_focus_box.h, target.h, 100, EasingType::EASE_OUT_QUAD);
    }
}

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

        FocusBox target = m_Widgets[index]->getFocusBox();
        m_ui.animate(m_current_focus_box.x, target.x, 100, EasingType::EASE_OUT_QUAD);
        m_ui.animate(m_current_focus_box.y, target.y, 100, EasingType::EASE_OUT_QUAD);
        m_ui.animate(m_current_focus_box.w, target.w, 100, EasingType::EASE_OUT_QUAD);
        m_ui.animate(m_current_focus_box.h, target.h, 100, EasingType::EASE_OUT_QUAD);
    }
}

void FocusManager::selectCurrent() {
    last_focus_change_time = m_ui.getCurrentTime();
    if (index >= 0 && index < (int)m_Widgets.size()) {
        IWidget* selectedWidget = m_Widgets[index];
        if (selectedWidget->onSelect()) {
            m_activeWidget = selectedWidget;
            m_activeWidget->onActivate(m_ui.getCurrentTime());  // 调用 onActivate
            m_state = State::IDLE;
        }
    }
}

void FocusManager::draw() {
    // 首先检查活动控件的超时
    checkActiveWidgetTimeout();

    // 检查焦点框是否需要收缩
    if (m_state != State::IDLE && m_ui.getCurrentTime() - last_focus_change_time > 2500) {
        if (m_state != State::ANIMATING_SHRINK) {
            m_state = State::ANIMATING_SHRINK;

            int32_t center_x = m_current_focus_box.x + m_current_focus_box.w / 2;
            int32_t center_y = m_current_focus_box.y + m_current_focus_box.h / 2;

            m_ui.animate(m_current_focus_box.w, 0, 100, EasingType::EASE_IN_QUAD);
            m_ui.animate(m_current_focus_box.h, 0, 100, EasingType::EASE_IN_QUAD);
            m_ui.animate(m_current_focus_box.x, center_x, 100, EasingType::EASE_IN_QUAD);
            m_ui.animate(m_current_focus_box.y, center_y, 100, EasingType::EASE_IN_QUAD);
        }
    }

    if (m_state != State::IDLE) {
        if (m_state == State::ANIMATING_SHRINK) {
            if (m_current_focus_box.w <= 1 && m_current_focus_box.h <= 1) {
                m_state = State::IDLE;
                index = -1;
                // m_current_focus_box = {0, 64, 0, 0}; 
                return;
            }
        }
        
        U8G2& u8g2 = m_ui.getU8G2();
        u8g2.setDrawColor(2);

        if (index >= 0 && index < (int)m_Widgets.size()) {
            m_target_focus_box = m_Widgets[index]->getFocusBox();
        }

        if (m_state == State::ANIMATING && m_current_focus_box == m_target_focus_box) {
            m_state = State::FOCUSED;
        }

        switch (m_state) {
            case State::ANIMATING:
            case State::FOCUSED:
            case State::ANIMATING_SHRINK:
                u8g2.drawBox(m_current_focus_box.x, m_current_focus_box.y, 
                            m_current_focus_box.w, m_current_focus_box.h);
                break;
            default:
                break;
        }
        u8g2.setDrawColor(1);
    }
}

void FocusManager::addWidget(IWidget* w) {
    m_Widgets.push_back(w);
}

void FocusManager::removeWidget(IWidget* w) {
    auto it = etl::find(m_Widgets.begin(), m_Widgets.end(), w);
    if (it != m_Widgets.end()) {
        m_Widgets.erase(it);
    }

    if (m_Widgets.empty()) {
        index = -1;
        m_state = State::IDLE;
    } else if (index >= (int)m_Widgets.size()) {
        index = m_Widgets.size() - 1;
        m_state = State::FOCUSED;
    }
}
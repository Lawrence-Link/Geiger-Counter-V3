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

#include "ui/Popup/PopupManager.h"
#include <algorithm>

/**
 * @brief Add a popup to the manager
 * @param popup Shared pointer to an IPopup
 *
 * Maintains the _popups container in ascending priority order
 * (low priority first, high priority last). If the container is full,
 * removes the lowest priority popup to make room for the new one.
 */
void PopupManager::addPopup(std::shared_ptr<IPopup> popup) {
    if (!popup) return;

    // 如果达到最大容量，移除最低优先级弹窗
    if (_popups.size() >= _popups.max_size()) {
        if (!_popups.empty()) {
            auto minPriorityIt = _popups.begin();
            for (auto it = _popups.begin(); it != _popups.end(); ++it) {
                if ((*it)->getPriority() < (*minPriorityIt)->getPriority()) {
                    minPriorityIt = it;
                }
            }
            _popups.erase(minPriorityIt);
        }
    }

    // 找到正确的插入位置以保持按优先级升序排序
    auto insertPos = _popups.begin();
    for (; insertPos != _popups.end(); ++insertPos) {
        if ((*insertPos)->getPriority() > popup->getPriority()) {
            break;
        }
    }
    _popups.insert(insertPos, popup);
}

/**
 * @brief Remove a specific popup from the manager
 * @param popup Shared pointer to the popup to remove
 *
 * Finds the popup in the container and erases it.
 */
void PopupManager::removePopup(std::shared_ptr<IPopup> popup) {
    auto it = std::find(_popups.begin(), _popups.end(), popup);
    if (it != _popups.end()) {
        _popups.erase(it);
    }
}

/**
 * @brief Clear all popups from the manager
 */
void PopupManager::clearPopups() {
    _popups.clear();
}

/**
 * @brief Draw all popups in order
 *
 * Lower priority popups are drawn first, higher priority popups
 * are drawn on top.
 */
void PopupManager::drawPopups() {
    for (auto& popup : _popups) {
        popup->draw();
    }
}

/**
 * @brief Update all popups with the current time
 * @param currentTime Current system time in milliseconds
 *
 * Calls each popup's update() method. Removes popups that
 * return false (finished animation or expired).
 */
void PopupManager::updatePopups(uint32_t currentTime) {
    auto it = _popups.begin();
    while (it != _popups.end()) {
        if (!(*it)->update(currentTime)) {
            it = _popups.erase(it);  // remove finished popup
        } else {
            ++it;
        }
    }
}

/**
 * @brief Handle input for the topmost popup first
 * @param event Input event
 * @return true if any popup consumed the event, false otherwise
 *
 * Iterates from highest priority to lowest, giving topmost popup
 * the first chance to consume the input.
 */
bool PopupManager::handleTopPopupInput(InputEvent event) {
    for (auto it = _popups.rbegin(); it != _popups.rend(); ++it) {
        if ((*it)->handleInput(event)) {
            return true; 
        }
    }
    return false;
}

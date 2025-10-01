/*
 * Copyright (C) 2025 Lawrence Link
 */

#include "ui/Popup/PopupManager.h"
#include <algorithm>

void PopupManager::addPopup(std::shared_ptr<IPopup> popup) {
    if (!popup) return;
    // 检查是否达到最大弹窗数量限制
    if (_popups.size() >= _popups.max_size()) {
        // 移除最低优先级的弹窗为新弹窗腾出空间
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
    // 找到正确的插入位置以维护按优先级排序的列表（低优先级在前，高优先级在后）
    auto insertPos = _popups.begin();
    for (; insertPos != _popups.end(); ++insertPos) {
        if ((*insertPos)->getPriority() > popup->getPriority()) {
            break;
        }
    }
    _popups.insert(insertPos, popup);
}

void PopupManager::removePopup(std::shared_ptr<IPopup> popup) {
    auto it = std::find(_popups.begin(), _popups.end(), popup);
    if (it != _popups.end()) {
        _popups.erase(it);
    }
}

void PopupManager::clearPopups() {
    _popups.clear();
}

void PopupManager::drawPopups() {
    for (auto& popup : _popups) {
        popup->draw();
    }
}

void PopupManager::updatePopups(uint32_t currentTime) {
    auto it = _popups.begin();
    while (it != _popups.end()) {
        if (!(*it)->update(currentTime)) {
            it = _popups.erase(it);
        } else {
            ++it;
        }
    }
}

bool PopupManager::handleTopPopupInput(InputEvent event) {
    for (auto it = _popups.rbegin(); it != _popups.rend(); ++it) {
        if ((*it)->handleInput(event)) {
            return true; 
        }
    }
    return false;
}
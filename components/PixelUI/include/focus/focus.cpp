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

#include "focus/focus.h"
#include "PixelUI.h"

void FocusManager::enterIdle(bool clearSelection) {
    m_state = State::IDLE;
    if (clearSelection) m_currentWidget = nullptr;
}

void FocusManager::beginFocusAnimation(IWidget* widget) {
    if (!widget) return;
    cancelOwnAnimations();
    m_currentWidget = widget;
    m_state = State::ANIMATING;
    last_focus_change_time = m_ui.getCurrentTime();
    m_target_focus_box = widget->getFocusBox();
    const bool xStarted = m_ui.animate(
        m_current_focus_box.x, m_target_focus_box.x, 100,
        EasingType::EASE_OUT_QUAD, PROTECTION::NOT_PROTECTED,
        &animationHandles_[0]);
    const bool yStarted = m_ui.animate(
        m_current_focus_box.y, m_target_focus_box.y, 100,
        EasingType::EASE_OUT_QUAD, PROTECTION::NOT_PROTECTED,
        &animationHandles_[1]);
    const bool wStarted = m_ui.animate(
        m_current_focus_box.w, m_target_focus_box.w, 100,
        EasingType::EASE_OUT_QUAD, PROTECTION::NOT_PROTECTED,
        &animationHandles_[2]);
    const bool hStarted = m_ui.animate(
        m_current_focus_box.h, m_target_focus_box.h, 100,
        EasingType::EASE_OUT_QUAD, PROTECTION::NOT_PROTECTED,
        &animationHandles_[3]);
    if (!xStarted || !yStarted || !wStarted || !hStarted) {
        cancelOwnAnimations();
        m_current_focus_box = m_target_focus_box;
        enterFocused(false);
    }
}

void FocusManager::enterFocused(bool synchronizeBox) {
    if (!isNavigable(m_currentWidget)) {
        enterIdle(true);
        return;
    }
    m_state = State::FOCUSED;
    if (synchronizeBox) {
        last_focus_change_time = m_ui.getCurrentTime();
        m_current_focus_box = m_currentWidget->getFocusBox();
        m_target_focus_box = m_current_focus_box;
    }
}

void FocusManager::beginShrinkAnimation() {
    cancelOwnAnimations();
    m_state = State::ANIMATING_SHRINK;
    const int32_t centerX = m_current_focus_box.x + m_current_focus_box.w / 2;
    const int32_t centerY = m_current_focus_box.y + m_current_focus_box.h / 2;
    const bool wStarted = m_ui.animate(
        m_current_focus_box.w, 0, 100, EasingType::EASE_IN_QUAD,
        PROTECTION::NOT_PROTECTED, &animationHandles_[0]);
    const bool hStarted = m_ui.animate(
        m_current_focus_box.h, 0, 100, EasingType::EASE_IN_QUAD,
        PROTECTION::NOT_PROTECTED, &animationHandles_[1]);
    const bool xStarted = m_ui.animate(
        m_current_focus_box.x, centerX, 100, EasingType::EASE_IN_QUAD,
        PROTECTION::NOT_PROTECTED, &animationHandles_[2]);
    const bool yStarted = m_ui.animate(
        m_current_focus_box.y, centerY, 100, EasingType::EASE_IN_QUAD,
        PROTECTION::NOT_PROTECTED, &animationHandles_[3]);
    if (!wStarted || !hStarted || !xStarted || !yStarted) {
        cancelOwnAnimations();
        m_current_focus_box = {centerX, centerY, 0, 0};
        enterIdle(true);
    }
}

bool FocusManager::isRegisteredRoot(const IWidget* widget) const {
    for (IWidget* root : roots_) if (root == widget) return true;
    return false;
}

bool FocusManager::hasRegisteredAncestor(const IWidget* widget) const {
    for (const IWidget* parent = widget ? widget->parent() : nullptr; parent; parent = parent->parent()) {
        if (isRegisteredRoot(parent)) return true;
    }
    return false;
}

bool FocusManager::isNavigable(const IWidget* widget) const {
    if (!widget || !widget->isFocusable() || !widget->isVisible() || !widget->isEnabled()) return false;
    for (const IWidget* parent = widget->parent(); parent; parent = parent->parent()) {
        if (!parent->isVisible() || !parent->isEnabled()) return false;
    }
    return isRegisteredRoot(widget) || hasRegisteredAncestor(widget);
}

IWidget* FocusManager::firstTreeNode() const {
    for (IWidget* root : roots_) if (root && !hasRegisteredAncestor(root)) return root;
    return nullptr;
}

IWidget* FocusManager::lastTreeNode() const {
    for (size_t i = roots_.size(); i > 0; --i) {
        IWidget* node = roots_[i - 1];
        if (!node || hasRegisteredAncestor(node)) continue;
        while (node->lastChild()) node = node->lastChild();
        return node;
    }
    return nullptr;
}

IWidget* FocusManager::nextTreeNode(IWidget* widget) const {
    if (!widget) return firstTreeNode();
    if (widget->firstChild()) return widget->firstChild();
    while (widget) {
        if (widget->nextSibling()) return widget->nextSibling();
        IWidget* parent = widget->parent();
        if (!parent) {
            for (size_t i = 0; i < roots_.size(); ++i) {
                if (roots_[i] != widget) continue;
                for (++i; i < roots_.size(); ++i) {
                    if (roots_[i] && !hasRegisteredAncestor(roots_[i])) return roots_[i];
                }
                return nullptr;
            }
        }
        widget = parent;
    }
    return nullptr;
}

IWidget* FocusManager::previousTreeNode(IWidget* widget) const {
    if (!widget) return lastTreeNode();
    if (widget->previousSibling()) {
        widget = widget->previousSibling();
        while (widget->lastChild()) widget = widget->lastChild();
        return widget;
    }
    if (widget->parent()) return widget->parent();
    for (size_t i = 0; i < roots_.size(); ++i) {
        if (roots_[i] != widget) continue;
        while (i > 0) {
            IWidget* root = roots_[--i];
            if (!root || hasRegisteredAncestor(root)) continue;
            while (root->lastChild()) root = root->lastChild();
            return root;
        }
    }
    return nullptr;
}

IWidget* FocusManager::nextFocusable(IWidget* widget) const {
    IWidget* candidate = nextTreeNode(widget);
    if (!candidate) candidate = firstTreeNode();
    IWidget* firstCandidate = candidate;
    do {
        if (isNavigable(candidate)) return candidate;
        candidate = nextTreeNode(candidate);
        if (!candidate) candidate = firstTreeNode();
    } while (candidate && candidate != firstCandidate);
    return nullptr;
}

IWidget* FocusManager::previousFocusable(IWidget* widget) const {
    IWidget* candidate = previousTreeNode(widget);
    if (!candidate) candidate = lastTreeNode();
    IWidget* firstCandidate = candidate;
    do {
        if (isNavigable(candidate)) return candidate;
        candidate = previousTreeNode(candidate);
        if (!candidate) candidate = lastTreeNode();
    } while (candidate && candidate != firstCandidate);
    return nullptr;
}

size_t FocusManager::focusableCount() const {
    size_t count = 0;
    for (IWidget* node = firstTreeNode(); node; node = nextTreeNode(node)) {
        if (isNavigable(node)) ++count;
    }
    return count;
}

void FocusManager::onWidgetSubtreeDetaching(IWidget& subtree) {
    if (subtree.contains(m_activeWidget)) {
        m_activeWidget->onDeactivate();
        m_activeWidget = nullptr;
    }
    if (subtree.contains(m_currentWidget)) enterIdle(true);
    for (auto it = roots_.begin(); it != roots_.end();) {
        if (subtree.contains(*it)) it = roots_.erase(it);
        else ++it;
    }
}

void FocusManager::onWidgetDestroyed(IWidget& widget) {
    onWidgetSubtreeDetaching(widget);
}

void FocusManager::resetState() {
    cancelOwnAnimations();
    if (m_activeWidget) m_activeWidget->onDeactivate();
    m_activeWidget = nullptr;
    m_current_focus_box = {0, 64, 0, 0};
    m_target_focus_box = m_current_focus_box;
    enterIdle(true);
}

void FocusManager::clear() {
    resetState();
    for (IWidget* root : roots_) {
        if (root) root->setTreeObserver(nullptr);
    }
    roots_.clear();
}

void FocusManager::cancelOwnAnimations() {
    for (AnimationHandle& handle : animationHandles_) {
        m_ui.cancelAnimation(handle);
        handle = INVALID_ANIMATION_HANDLE;
    }
}

void FocusManager::clearActiveWidget() {
    if (!m_activeWidget) return;
    m_activeWidget->onDeactivate();
    m_activeWidget = nullptr;
    enterFocused(true);
}

bool FocusManager::handleInput(InputEvent event) {
    if (m_activeWidget) {
        if (m_activeWidget->handleEvent(event)) clearActiveWidget();
        return true;
    }
    if (focusableCount() == 0U) return false;
    if (event == FOCUS_MANAGER_NAVI_NEXT) { moveNext(); return true; }
    if (event == FOCUS_MANAGER_NAVI_PREV) { movePrev(); return true; }
    if (event == FOCUS_MANAGER_NAVI_SELECT) { selectCurrent(); return true; }
    return false;
}

void FocusManager::checkActiveWidgetTimeout() {
    if (!m_activeWidget) return;
    const uint32_t timeout = m_activeWidget->getTimeout();
    if (timeout == 0U) return;
    if (m_ui.getCurrentTime() - m_activeWidget->getLastInteractionTime() >= timeout) {
        clearActiveWidget();
        m_ui.markDirty();
    }
}

void FocusManager::moveNext() {
    IWidget* next = nextFocusable(m_currentWidget);
    if (!next || next == m_currentWidget) return;
    beginFocusAnimation(next);
}

void FocusManager::movePrev() {
    IWidget* previous = previousFocusable(m_currentWidget);
    if (!previous || previous == m_currentWidget) return;
    beginFocusAnimation(previous);
}

void FocusManager::selectCurrent() {
    last_focus_change_time = m_ui.getCurrentTime();
    if (!isNavigable(m_currentWidget)) return;
    if (m_currentWidget->onSelect()) {
        m_activeWidget = m_currentWidget;
        m_activeWidget->onActivate(m_ui.getCurrentTime());
        enterIdle(false);
        return;
    }
    beginFocusAnimation(m_currentWidget);
}

void FocusManager::draw() {
    checkActiveWidgetTimeout();
    if (m_currentWidget && !isNavigable(m_currentWidget)) enterIdle(true);
    if (m_state != State::IDLE && m_ui.getCurrentTime() - last_focus_change_time > 2500U &&
        m_state != State::ANIMATING_SHRINK) beginShrinkAnimation();

    if (m_state == State::IDLE) return;
    if (m_state == State::ANIMATING_SHRINK && m_current_focus_box.w <= 1 && m_current_focus_box.h <= 1) {
        enterIdle(true);
        return;
    }
    if (isNavigable(m_currentWidget)) m_target_focus_box = m_currentWidget->getFocusBox();
    if (m_state == State::ANIMATING && m_current_focus_box == m_target_focus_box) enterFocused(false);

    U8G2& u8g2 = m_ui.getU8G2();
    u8g2.setDrawColor(2);
    u8g2.drawBox(m_current_focus_box.x, m_current_focus_box.y,
                 m_current_focus_box.w, m_current_focus_box.h);
    u8g2.setDrawColor(1);
}

bool FocusManager::addWidget(IWidget* widget) {
    if (!widget || isRegisteredRoot(widget)) return widget != nullptr;
    if (widget->treeObserver_ && widget->treeObserver_ != this) return false;
    if (hasRegisteredAncestor(widget)) return true;

    for (auto it = roots_.begin(); it != roots_.end();) {
        if (widget->contains(*it)) it = roots_.erase(it);
        else ++it;
    }
    if (roots_.full()) return false;
    roots_.push_back(widget);
    widget->setTreeObserver(this);
    return true;
}

void FocusManager::removeWidget(IWidget* widget) {
    if (!widget) return;
    for (auto it = roots_.begin(); it != roots_.end(); ++it) {
        if (*it == widget) {
            onWidgetSubtreeDetaching(*widget);
            widget->setTreeObserver(nullptr);
            return;
        }
    }
}

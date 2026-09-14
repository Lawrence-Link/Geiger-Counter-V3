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

#include <stdint.h>
#include "core/canvas/Canvas.h"
#include "core/Callbacks.h"
#include "core/CommonTypes.h"

class PixelUI;
class FocusManager;
class IWidget;

class IWidgetTreeObserver {
public:
/** @brief ~IWidgetTreeObserver. */
    virtual ~IWidgetTreeObserver() = default;
/** @brief onWidgetSubtreeDetaching. */
    virtual void onWidgetSubtreeDetaching(IWidget& subtree) = 0;
/** @brief onWidgetDestroyed. */
    virtual void onWidgetDestroyed(IWidget& widget) = 0;
};

struct WidgetRenderContext {
    int32_t originX = 0;
    int32_t originY = 0;
    FocusBox clip = {0, 0, 0, 0};
};

// Selects whether loading begins with an entrance animation or its final frame.
enum class LoadTransition : uint8_t {
    Animated,
    Immediate
};

class IWidget {
    friend class FocusManager;
private:
    bool focusable_ = false;
    bool visible_ = true;
    bool enabled_ = true;
    bool clipChildren_ = true;
    FocusInsets focusInsets_ = {};
    FocusBox bounds_ = {0, 0, 0, 0};

    IWidget* parent_ = nullptr;
    IWidget* firstChild_ = nullptr;
    IWidget* lastChild_ = nullptr;
    IWidget* previousSibling_ = nullptr;
    IWidget* nextSibling_ = nullptr;
    IWidgetTreeObserver* treeObserver_ = nullptr;

/** @brief isAncestorOf. */
    bool isAncestorOf(const IWidget& widget) const;
/** @brief setTreeObserver. */
    void setTreeObserver(IWidgetTreeObserver* observer);
/** @brief drawTree. */
    void drawTree(const WidgetRenderContext& parentContext);

protected:
    bool m_is_active = false;
    uint32_t m_last_interaction_time = 0;

/** @brief setFocusInsets. */
    void setFocusInsets(const FocusInsets& insets) { focusInsets_ = insets; }
/** @brief setFocusBox. */
    void setFocusBox(const FocusBox& box);
/** @brief setWidgetBounds. */
    void setWidgetBounds(const FocusBox& bounds) { bounds_ = bounds; }
/** @brief drawSelf. */
    virtual void drawSelf(const WidgetRenderContext& context) = 0;
/** @brief drawOverlay. */
    virtual void drawOverlay(const WidgetRenderContext&) {}
/** @brief getChildrenClipBounds. */
    virtual FocusBox getChildrenClipBounds() const { return bounds_; }
/** @brief display. */
    virtual Canvas& display() = 0;

/** @brief setClipWindow. */
    void setClipWindow(const WidgetRenderContext& context, const FocusBox& localClip);
/** @brief restoreClipWindow. */
    void restoreClipWindow(const WidgetRenderContext& context);

public:
/** @brief IWidget. */
    IWidget() = default;
/** @brief ~IWidget. */
    virtual ~IWidget();

/** @brief IWidget. */
    IWidget(const IWidget&) = delete;
    IWidget& operator=(const IWidget&) = delete;
/** @brief IWidget. */
    IWidget(IWidget&&) = delete;
    IWidget& operator=(IWidget&&) = delete;

/** @brief draw. */
    void draw();
/** @brief draw. */
    void draw(const WidgetRenderContext& context);

/** @brief addChild. */
    bool addChild(IWidget& child);
/** @brief removeChild. */
    bool removeChild(IWidget& child);
/** @brief setParent. */
    bool setParent(IWidget* parent);
/** @brief removeFromParent. */
    void removeFromParent();
/** @brief removeAllChildren. */
    void removeAllChildren();

/** @brief parent. */
    IWidget* parent() const { return parent_; }
/** @brief firstChild. */
    IWidget* firstChild() const { return firstChild_; }
/** @brief lastChild. */
    IWidget* lastChild() const { return lastChild_; }
/** @brief previousSibling. */
    IWidget* previousSibling() const { return previousSibling_; }
/** @brief nextSibling. */
    IWidget* nextSibling() const { return nextSibling_; }
/** @brief contains. */
    bool contains(const IWidget* widget) const;

/** @brief handleEvent. */
    virtual bool handleEvent(InputEvent) { return false; }
/** @brief onLoad. */
    virtual void onLoad() = 0;
/** @brief onOffload. */
    virtual void onOffload() = 0;
/** @brief onSelect. */
    virtual bool onSelect() { return false; }
/** @brief getTimeout. */
    virtual uint32_t getTimeout() const { return 0; }

/** @brief onActivate. */
    virtual void onActivate(uint32_t currentTime) {
        m_is_active = true;
        m_last_interaction_time = currentTime;
    }

/** @brief onDeactivate. */
    virtual void onDeactivate() { m_is_active = false; }

/** @brief updateInteractionTime. */
    void updateInteractionTime(uint32_t currentTime) { m_last_interaction_time = currentTime; }
/** @brief getLastInteractionTime. */
    uint32_t getLastInteractionTime() const { return m_last_interaction_time; }
/** @brief isActive. */
    bool isActive() const { return m_is_active; }

/** @brief isFocusable. */
    bool isFocusable() const { return focusable_; }
/** @brief setFocusable. */
    void setFocusable(bool state) { focusable_ = state; }
/** @brief isVisible. */
    bool isVisible() const { return visible_; }
/** @brief setVisible. */
    void setVisible(bool visible) { visible_ = visible; }
/** @brief isEnabled. */
    bool isEnabled() const { return enabled_; }
/** @brief setEnabled. */
    void setEnabled(bool enabled) { enabled_ = enabled; }
/** @brief clipsChildren. */
    bool clipsChildren() const { return clipChildren_; }
/** @brief setClipChildren. */
    void setClipChildren(bool clip) { clipChildren_ = clip; }

/** @brief getLocalBounds. */
    FocusBox getLocalBounds() const { return bounds_; }
/** @brief getScreenBounds. */
    FocusBox getScreenBounds() const;
/** @brief getFocusInsets. */
    FocusInsets getFocusInsets() const { return focusInsets_; }
/** @brief getFocusBox. */
    FocusBox getFocusBox() const;
};

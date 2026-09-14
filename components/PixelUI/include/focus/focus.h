#pragma once

#include "widgets/IWidget.h"
#include "core/animation/animation.h"
#include <etl/array.h>
#include <etl/vector.h>
#include "config.h"

class FocusManager : private IWidgetTreeObserver {
private:
    PixelUI& m_ui;
    uint32_t last_focus_change_time = 0;
    FocusBox m_target_focus_box = {0, 0, 0, 0};
    FocusBox m_current_focus_box = {0, 64, 0, 0};
    IWidget* m_currentWidget = nullptr;
    IWidget* m_activeWidget = nullptr;

    enum class State {
        IDLE,
        ANIMATING,
        FOCUSED,
        ANIMATING_SHRINK
    } m_state = State::IDLE;

    etl::vector<IWidget*, MAX_ONSCREEN_WIDGET_NUM> roots_;
    etl::array<AnimationHandle, 4> animationHandles_{};

    /** @brief Enters the idle state and optionally clears the current selection. */
    void enterIdle(bool clearSelection);
    /** @brief Animates the focus box toward a widget's focus bounds. */
    void beginFocusAnimation(IWidget* widget);
    /** @brief Enters the focused state after validating the current widget. */
    void enterFocused(bool synchronizeBox);
    /** @brief Animates the focus box toward zero size before idling. */
    void beginShrinkAnimation();
    /** @brief Deactivates the active widget after its interaction timeout. */
    void checkActiveWidgetTimeout();

    /** @return Whether widget is registered as a focus-tree root. */
    bool isRegisteredRoot(const IWidget* widget) const;
    /** @return Whether widget has a registered root among its ancestors. */
    bool hasRegisteredAncestor(const IWidget* widget) const;
    /** @return Whether widget and all its parents are visible, enabled, and focusable. */
    bool isNavigable(const IWidget* widget) const;
    /** @return First node in the registered widget tree. */
    IWidget* firstTreeNode() const;
    /** @return Last node in the registered widget tree. */
    IWidget* lastTreeNode() const;
    /** @return Next node in depth-first widget-tree order. */
    IWidget* nextTreeNode(IWidget* widget) const;
    /** @return Previous node in depth-first widget-tree order. */
    IWidget* previousTreeNode(IWidget* widget) const;
    /** @return Next navigable widget, wrapping at the end of the tree. */
    IWidget* nextFocusable(IWidget* widget) const;
    /** @return Previous navigable widget, wrapping at the start of the tree. */
    IWidget* previousFocusable(IWidget* widget) const;
    /** @return Number of currently navigable widgets. */
    size_t focusableCount() const;
    /** @brief Cancels all focus-box animations owned by the manager. */
    void cancelOwnAnimations();
    /** @brief Clears focus references when a widget subtree is detached. */
    void onWidgetSubtreeDetaching(IWidget& subtree) override;
    /** @brief Clears focus references when a widget is destroyed. */
    void onWidgetDestroyed(IWidget& widget) override;

public:
    /** @brief Creates an empty focus manager bound to a UI instance. */
    explicit FocusManager(PixelUI& ui) : m_ui(ui) {}
    /** @brief Clears registrations and focus state before destruction. */
    ~FocusManager() { clear(); }

    /** @brief Cancels focus animations and resets selection and focus geometry. */
    void resetState();
    /** @brief Resets state and removes all non-owning widget registrations. */
    void clear();
    /** @return Number of navigable widgets currently registered. */
    size_t widgetCount() const { return focusableCount(); }

    /** @return Widget currently receiving active interaction, or nullptr. */
    IWidget* getActiveWidget() const { return m_activeWidget; }
    /** @brief Deactivates the active widget and restores focused navigation. */
    void clearActiveWidget();
    /** @brief Moves focus to the next navigable widget. */
    void moveNext();
    /** @brief Moves focus to the previous navigable widget. */
    void movePrev();
    /** @brief Updates focus timeouts and draws the focus indicator. */
    void draw();
    /** @return Whether the event was consumed by active or focus navigation. */
    bool handleInput(InputEvent event);
    /** @brief Registers a non-owning widget root for focus navigation. */
    bool addWidget(IWidget* widget);
    /** @brief Unregisters a widget root and its subtree from focus navigation. */
    void removeWidget(IWidget* widget);
    /** @brief Activates the current widget or restarts its focus animation. */
    void selectCurrent();
};

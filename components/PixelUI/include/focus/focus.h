#pragma once

#include "widgets/IWidget.h"
#include <etl/vector.h>

/**
 * @class FocusManager
 * @brief Manages the focus state and animated transitions between UI widgets.
 *
 * This class uses a state machine to handle the different phases of focus:
 * idle, moving, stable, and shrinking. It also manages the list of widgets
 * that can receive focus.
 */
class FocusManager {
private:
    int index = -1; /**< The index of the currently focused widget. -1 if no widget is focused. */
    PixelUI& m_ui; /**< Reference to the main UI class for drawing and animation. */

    uint32_t last_focus_change_time = 0; /**< The timestamp of the last focus change or user input. */
    FocusBox m_target_focus_box;         /**< The target coordinates and dimensions for the animation. */
    FocusBox m_current_focus_box = {0,64,0,0};        /**< The current coordinates and dimensions being animated. */
    
    /** @brief A pointer to the widget that is currently handling all input events. nullptr if no widget has control. */
    IWidget* m_activeWidget = nullptr;

    /**
     * @enum State
     * @brief Represents the current state of the focus manager.
     */
    enum class State {
        IDLE,               /**< No widget is currently focused. */
        ANIMATING,          /**< The focus box is animating towards a new widget. */
        FOCUSED,            /**< The focus box has settled on a widget. */
        ANIMATING_SHRINK    /**< The focus box is shrinking to a point. */
    } m_state = State::IDLE;

public:
    /**
     * @brief Constructs the FocusManager.
     * @param ui A reference to the main PixelUI instance.
     */
    FocusManager(PixelUI& ui) : m_ui(ui) {};
    ~FocusManager() = default;

    etl::vector<IWidget*, MAX_ONSCREEN_WIDGET_NUM> m_Widgets; /**< The list of all widgets managed by the FocusManager. */
    
    /** @brief Returns a pointer to the widget currently handling input, or nullptr if none. */
    IWidget* getActiveWidget() const { return m_activeWidget; }
    
    /** @brief Clears the active widget, returning input control to the FocusManager. */
    void clearActiveWidget();
    
    /** @brief Moves the focus to the next widget. */
    void moveNext();
    /** @brief Moves the focus to the previous widget. */
    void movePrev();
    /** @brief Handles all drawing and animation logic for the focus box. */
    void draw();

    /**
     * @brief Adds a widget to the manager's list.
     * @param w A pointer to the IWidget to add.
     */
    void addWidget(IWidget* w);
    /**
     * @brief Removes a widget from the manager's list.
     * @param w A pointer to the IWidget to remove.
     */
    void removeWidget(IWidget* w);
    /** @brief Selects the currently focused widget, triggering its onSelect action. */
    void selectCurrent();
};
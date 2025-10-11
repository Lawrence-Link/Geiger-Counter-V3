#pragma once

#include "widgets/IWidget.h"
#include <etl/vector.h>

/**
 * @class FocusManager
 * @brief Manages the focus state and animated transitions between UI widgets.
 *
 * This class uses a state machine to handle the different phases of focus:
 * idle, moving, stable, and shrinking. It also manages the list of widgets
 * that can receive focus and handles timeout for active widgets.
 */
class FocusManager {
private:
    int index = -1;
    PixelUI& m_ui;

    uint32_t last_focus_change_time = 0;
    FocusBox m_target_focus_box;
    FocusBox m_current_focus_box = {0,64,0,0};
    
    IWidget* m_activeWidget = nullptr;

    enum class State {
        IDLE,
        ANIMATING,
        FOCUSED,
        ANIMATING_SHRINK
    } m_state = State::IDLE;

    /**
     * @brief Checks if the active widget has timed out and handles deactivation.
     */
    void checkActiveWidgetTimeout();

public:
    FocusManager(PixelUI& ui) : m_ui(ui) {};
    ~FocusManager() {clearActiveWidget();};

    etl::vector<IWidget*, MAX_ONSCREEN_WIDGET_NUM> m_Widgets;
    
    IWidget* getActiveWidget() const { return m_activeWidget; }
    void clearActiveWidget();
    
    void moveNext();
    void movePrev();
    void draw();

    void addWidget(IWidget* w);
    void removeWidget(IWidget* w);
    void selectCurrent();
};
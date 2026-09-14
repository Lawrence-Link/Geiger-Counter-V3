/*
 * Copyright (C) 2025 Lawrence Link
 */

#pragma once

#include "PopupInterface.h"

class PixelUI;

struct PopupContentBounds {
    int16_t x;
    int16_t y;
    int16_t width;
    int16_t height;
    int16_t centerX;
    int16_t centerY;
};

/**
 * @class PopupBase
 * @brief Base class for all popups, providing common functionality.
 */
class PopupBase : public IPopup {
protected:
    /** @return The UI instance used by this popup. */
    PixelUI& ui() const { return m_ui; }
    /** @return Whether the popup is in its closing transition. */
    bool isClosing() const { return m_state == PopupState::CLOSING; }
    /** @brief Starts closing when the derived popup permits it. */
    void requestClose();
    /** @brief Restarts the auto-close timer while the popup is shown. */
    void resetAutoCloseTimer();
    /** @param height New popup content height in pixels. */
    void setContentHeight(uint16_t height) { m_height = height; }
    /** @return Popup width in pixels. */
    uint16_t popupWidth() const { return m_width; }
    /** @return Popup content height in pixels. */
    uint16_t popupHeight() const { return m_height; }
    /** @brief Clips drawing to the supplied popup content bounds. */
    void setContentClip(const PopupContentBounds& bounds);

    /** @brief Draws derived popup content inside the supplied bounds. */
    virtual void drawContent(const PopupContentBounds& bounds) = 0;
    /** @return Whether derived content consumed the input event. */
    virtual bool handleContentInput(InputEvent event) { (void)event; return false; }
    /** @brief Called once after the appearing transition completes. */
    virtual void onShown() {}
    // Return false when the Popup cannot safely begin closing yet.
    /** @return Whether the derived popup permits beginning its close transition. */
    virtual bool onClosing() { return true; }

public:
    /** @brief Creates an appearing popup with fixed dimensions and auto-close duration. */
    PopupBase(PixelUI& ui, uint16_t width, uint16_t height, uint16_t duration);
    /** @brief Destroys the popup through its polymorphic interface. */
    virtual ~PopupBase() = default;
    
    /** @return Auto-close duration in milliseconds, or zero when disabled. */
    uint16_t getDuration() const override { return m_duration; }
    /** @return false once the closing transition has completed. */
    bool update(uint32_t currentTime) final;
    /** @brief Draws the animated popup frame and derived content. */
    void draw() final;
    /** @return Always true while the popup is active and owns the input. */
    bool handleInput(InputEvent event) final;
    /** @return Delay until the next animation or auto-close update. */
    uint32_t nextWakeupMs(
        uint32_t currentTime, uint32_t frameIntervalMs) const final;

private:
    static constexpr uint32_t TRANSITION_DURATION = 300U;
    static constexpr int16_t BORDER_OFFSET = 2;
    static constexpr int16_t BORDER_WIDTH = 1;

    /** @brief Draws the popup frame and filled interior. */
    void drawPopupBox(const PopupContentBounds& bounds);
    /** @brief Restores the full display clipping window and draw color. */
    void resetClipWindow();
    /** @brief Initializes the appearing transition at the supplied time. */
    void beginAppearing(uint32_t currentTime);
    /** @return Whether the appearing transition remains active. */
    bool updateAppearing(uint32_t currentTime);
    /** @return Whether the closing transition remains active. */
    bool updateClosing(uint32_t currentTime);

    PixelUI& m_ui;
    uint16_t m_width;
    uint16_t m_height;
    uint16_t m_duration;
    uint32_t m_stateStartTime = 0;
    uint32_t m_transitionStartTime = 0;
    int32_t m_transitionStartSize = 0;
    int32_t m_currentBoxSize = 0;
    int32_t m_targetBoxSize = 0;
    PopupState m_state = PopupState::APPEARING;
    bool m_started = false;
};

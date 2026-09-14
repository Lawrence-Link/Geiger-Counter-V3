/*
 * Copyright (C) 2025 Lawrence Link
 */

#pragma once

#include "PopupBase.h"
#include "config.h"
#include "core/Callbacks.h"
#include "focus/focus.h"
#include "widgets/text_button/text_button.h"
#include <etl/array.h>
#include <etl/pool.h>
#include <stddef.h>

/**
 * @class PopupKeyboard
 * @brief Fixed-capacity multi-tap text editor for compact displays.
 */
class PopupKeyboard : public PopupBase {
private:
    static constexpr uint8_t KEY_COUNT = 9U;
    static constexpr uint8_t KEY_COLUMNS = 3U;
    static constexpr uint8_t KEY_HEIGHT = 11U;
    static constexpr uint8_t KEY_GAP = 1U;
    static constexpr uint8_t ACTION_HEIGHT = 11U;
    static constexpr uint8_t DELETE_WIDTH = 24U;
    static constexpr uint8_t OK_WIDTH = 16U;
    static constexpr uint8_t ACTION_GAP = 2U;
    static constexpr uint8_t CONTENT_PADDING = 4U;
    static constexpr uint8_t KEY_TOP = 21U;
    static constexpr uint32_t MULTI_TAP_TIMEOUT_MS = 800U;

    class ControlGroup final : public IWidget {
    public:
        explicit ControlGroup(PixelUI& ui);
        void setSize(uint16_t width, uint16_t height);
        void onLoad() override {}
        void onOffload() override {}

    private:
        void drawSelf(const WidgetRenderContext&) override {}
        Canvas& display() override;
        PixelUI& ui_;
    };

    enum class FinalizationState : uint8_t {
        Editing,
        Committed,
        Cancelled,
    };

    // output_ is non-owning and must outlive this Popup.
    char* output_ = nullptr;
    size_t outputCapacity_ = 0U;
    VoidCallback commitCallback_;
    char draft_[MAX_TEXT_LENGTH + 1]{};
    size_t length_ = 0U;
    int8_t lastKey_ = -1;
    uint8_t lastCharIndex_ = 0U;
    uint32_t lastPressTime_ = 0U;
    etl::pool<TextButton, KEY_COUNT> keyPool_;
    etl::array<TextButton*, KEY_COUNT> keys_{};
    ControlGroup controls_;
    TextButton deleteButton_;
    TextButton okButton_;
    FocusManager focusManager_;
    FinalizationState finalizationState_ = FinalizationState::Editing;

    void initializeControls();
    void destroyControls();
    void pressKey(uint8_t index);
    void eraseLast();
    void commitEditing();
    void cancelEditing();

protected:
    void drawContent(const PopupContentBounds& bounds) override;
    bool handleContentInput(InputEvent event) override;
    bool onClosing() override;

public:
    /**
     * @pre isValidLayout(width, height) is true.
     * @pre isValidBuffer(output, outputCapacity) is true.
     * @pre width and height fit the bound display.
     */
    PopupKeyboard(PixelUI& ui, uint16_t width, uint16_t height,
                  char* output, size_t outputCapacity,
                  uint16_t duration = 0U,
                  VoidCallback commitCallback = nullptr);
    ~PopupKeyboard();

    static constexpr bool isValidLayout(uint16_t width, uint16_t height) {
        return width >= 100U && height >= 58U;
    }
    static bool isValidBuffer(const char* output, size_t outputCapacity);
};

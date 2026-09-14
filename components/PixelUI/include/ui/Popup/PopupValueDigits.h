/*
 * Copyright (C) 2025 Lawrence Link
 */

#pragma once

#include "PopupBase.h"
#include "config.h"
#include "core/Callbacks.h"
#include "core/NumericFormatter.h"
#include "core/NumericRange.h"
#include "core/ValueEdit.h"
#include "focus/focus.h"
#include "widgets/num_scroll/num_scroll.h"
#include "widgets/text_button/text_button.h"
#include <etl/array.h>
#include <etl/pool.h>

class PopupValueDigits : public PopupBase {
private:
    static_assert(MAX_INT_FIXED_WIDTH > 0, "PopupValueDigits needs at least one digit");
    static_assert(MAX_INT_FIXED_WIDTH <= 9, "PopupValueDigits must fit in int32_t");
    static constexpr uint8_t DIGIT_WIDTH = 12;
    static constexpr uint8_t DIGIT_HEIGHT = 16;
    static constexpr uint8_t DIGIT_GAP = 2;
    static constexpr uint8_t ACTION_HEIGHT = 13;
    static constexpr uint8_t OK_WIDTH = 24;
    static constexpr uint8_t CANCEL_WIDTH = 44;
    static constexpr uint8_t ACTION_GAP = 4;
    static constexpr uint8_t CONTENT_PADDING = 4;

    /** @return Minimum popup height required by the digit editor. */
    static constexpr uint16_t minimumHeight() { return 56U; }
    /** @return Minimum popup width required for the requested digit count. */
    static constexpr uint16_t minimumWidth(uint8_t digitCount) {
        const uint16_t digitWidth = digitCount == 0U
            ? 0U
            : static_cast<uint16_t>(
                  digitCount * DIGIT_WIDTH + (digitCount - 1U) * DIGIT_GAP);
        const uint16_t actionWidth = OK_WIDTH + ACTION_GAP + CANCEL_WIDTH;
        const uint16_t contentWidth = digitWidth > actionWidth
            ? digitWidth
            : actionWidth;
        return static_cast<uint16_t>(contentWidth + 2U * CONTENT_PADDING);
    }

    class ControlGroup final : public IWidget {
    public:
        /** @brief Creates the widget group used to position editor controls. */
        explicit ControlGroup(PixelUI& ui);
        /** @param width Group width in pixels. @param height Group height in pixels. */
        void setSize(uint16_t width, uint16_t height);
        /** @brief Handles loading; the group has no transient resources. */
        void onLoad() override {}
        /** @brief Handles offloading; the group has no transient resources. */
        void onOffload() override {}

    private:
        /** @brief Intentionally draws no content; child controls render themselves. */
        void drawSelf(const WidgetRenderContext&) override {}
        /** @return Canvas used by the control group. */
        Canvas& display() override;
        PixelUI& ui_;
    };

    enum class FinalizationState : uint8_t {
        Editing,
        Committed,
        Cancelled,
    };

    // title_, binding contexts in ownedSession_, and an injected session are
    // non-owning and must outlive this Popup.
    const char* title_;
    ValueCallback compatibilityCallback_;
    ValueEditSession ownedSession_;
    ValueEditSession* session_ = nullptr;
    NumericRange digitRange_{};
    static constexpr IntegerFormat DIGIT_FORMAT{1U, nullptr};
    etl::pool<NumScroll, MAX_INT_FIXED_WIDTH> digitPool_;
    etl::array<NumScroll*, MAX_INT_FIXED_WIDTH> digits_{};
    uint8_t digitCount_ = 0;
    ControlGroup controls_;
    TextButton okButton_;
    TextButton cancelButton_;
    FocusManager focusManager_;
    FinalizationState finalizationState_ = FinalizationState::Editing;

    /** @return Largest signed value representable with digitCount decimal digits. */
    static int32_t maximumValue(uint8_t digitCount);
    /** @return Value reconstructed from the currently displayed digits. */
    int32_t collectValue() const;
    /** @brief Writes the digit editor's current value into its session. */
    bool synchronizeValue();
    /** @brief Updates each digit widget from the session's draft value. */
    void restoreDigitsFromDraft();
    /** @brief Commits the current session and marks the popup committed. */
    bool commitEditing();
    /** @brief Cancels editing and optionally starts popup closing. */
    bool cancelEditing(bool closePopup = true);
    /** @brief Allocates and configures digit and action controls. */
    void initializeControls();
    /** @brief Destroys all controls allocated from the fixed pool. */
    void destroyControls();

protected:
    /** @brief Draws the digit controls and action buttons. */
    void drawContent(const PopupContentBounds& bounds) override;
    /** @return Whether the event was consumed by digit focus or actions. */
    bool handleContentInput(InputEvent event) override;
    /** @return Whether editing has reached a state safe for popup closure. */
    bool onClosing() override;

public:
    /**
     * @pre isValidLayout(width, height, digitCount) is true.
     * @pre session.valid() is true and session outlives this Popup.
     * @pre A Live session can restore its original value before destruction.
     */
    PopupValueDigits(PixelUI& ui, uint16_t width, uint16_t height,
                     ValueEditSession& session, uint8_t digitCount,
                     const char* title = "", uint16_t duration = 3000);
    /**
     * @pre isValidLayout(width, height, digitCount) is true.
     * @pre binding can be read and its contexts outlive this Popup.
     * @pre A Live binding can restore its original value before destruction.
     */
    PopupValueDigits(PixelUI& ui, uint16_t width, uint16_t height,
                     ValueEditorBinding binding, uint8_t digitCount,
                     const char* title, uint16_t duration,
                     ValueCallback callback = nullptr,
                     ValueEditPolicy policy = ValueEditPolicy::CommitOnConfirm);
    /** @pre isValidLayout(width, height, digitCount) is true. */
    PopupValueDigits(PixelUI& ui, uint16_t width, uint16_t height,
                     int32_t& value, uint8_t digitCount,
                     const char* title = "", uint16_t duration = 3000,
                     ValueCallback callback = nullptr);
    /** @brief Commits or restores the edit session and releases pooled controls. */
    ~PopupValueDigits();

    /** @return Whether digitCount fits the configured fixed storage. */
    static constexpr bool isValidDigitCount(uint8_t digitCount) {
        return digitCount > 0U && digitCount <= MAX_INT_FIXED_WIDTH;
    }
    /** @return Whether dimensions are sufficient for the requested digit count. */
    static constexpr bool isValidLayout(
        uint16_t width, uint16_t height, uint8_t digitCount) {
        return isValidDigitCount(digitCount) &&
               width >= minimumWidth(digitCount) &&
               height >= minimumHeight();
    }
};

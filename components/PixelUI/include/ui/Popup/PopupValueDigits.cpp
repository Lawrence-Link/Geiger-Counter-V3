/*
 * Copyright (C) 2025 Lawrence Link
 */

#include "ui/Popup/PopupValueDigits.h"
#include "PixelUI.h"
#include <assert.h>

PopupValueDigits::ControlGroup::ControlGroup(PixelUI& ui) : ui_(ui) {
    setClipChildren(false);
}

void PopupValueDigits::ControlGroup::setSize(
    uint16_t width, uint16_t height) {
    setWidgetBounds({0, 0, width, height});
}

Canvas& PopupValueDigits::ControlGroup::display() {
    return ui_.getCanvas();
}

/** @brief PopupValueDigits::maximumValue. */
int32_t PopupValueDigits::maximumValue(uint8_t digitCount) {
    int32_t maximum = 0;
    for (uint8_t i = 0; i < digitCount; ++i) maximum = maximum * 10 + 9;
    return maximum;
}

PopupValueDigits::PopupValueDigits(PixelUI& ui, uint16_t width, uint16_t height,
                                   ValueEditSession& session, uint8_t digitCount,
                                   const char* title, uint16_t duration)
    : PopupBase(ui, width, height, duration),
      title_(title),
      ownedSession_(0),
      session_(&session),
      digitCount_(digitCount),
      controls_(ui),
      okButton_(ui, 0, 0, OK_WIDTH, ACTION_HEIGHT, "OK"),
      cancelButton_(ui, 0, 0, CANCEL_WIDTH, ACTION_HEIGHT, "CANCEL"),
      focusManager_(ui) {
    assert(isValidLayout(width, height, digitCount));
    assert(session.valid());
    initializeControls();
}

PopupValueDigits::PopupValueDigits(PixelUI& ui, uint16_t width, uint16_t height,
                                   ValueEditorBinding binding, uint8_t digitCount,
                                   const char* title, uint16_t duration,
                                   ValueCallback callback, ValueEditPolicy policy)
    : PopupBase(ui, width, height, duration),
      title_(title),
      compatibilityCallback_(etl::move(callback)),
      ownedSession_(binding, policy),
      session_(&ownedSession_),
      digitCount_(digitCount),
      controls_(ui),
      okButton_(ui, 0, 0, OK_WIDTH, ACTION_HEIGHT, "OK"),
      cancelButton_(ui, 0, 0, CANCEL_WIDTH, ACTION_HEIGHT, "CANCEL"),
      focusManager_(ui) {
    assert(isValidLayout(width, height, digitCount));
    assert(ownedSession_.valid());
    initializeControls();
}

PopupValueDigits::PopupValueDigits(PixelUI& ui, uint16_t width, uint16_t height,
                                   int32_t& value, uint8_t digitCount,
                                   const char* title, uint16_t duration,
                                   ValueCallback callback)
    : PopupBase(ui, width, height, duration),
      title_(title),
      compatibilityCallback_(etl::move(callback)),
      ownedSession_(
          ValueEditorBinding::reference(value),
          ValueEditPolicy::CommitOnConfirm),
      session_(&ownedSession_),
      digitCount_(digitCount),
      controls_(ui),
      okButton_(ui, 0, 0, OK_WIDTH, ACTION_HEIGHT, "OK"),
      cancelButton_(ui, 0, 0, CANCEL_WIDTH, ACTION_HEIGHT, "CANCEL"),
      focusManager_(ui) {
    assert(isValidLayout(width, height, digitCount));
    initializeControls();
}

/** @brief PopupValueDigits::initializeControls. */
void PopupValueDigits::initializeControls() {
    const bool rangeCreated = NumericRange::tryCreate(0, 9, 1, digitRange_);
    assert(rangeCreated);
    (void)rangeCreated;
    const int32_t maximum = maximumValue(digitCount_);
    const int32_t sourceValue = session_ != nullptr && session_->valid()
        ? session_->draftValue()
        : 0;
    const int32_t displayedValue = sourceValue < 0
        ? 0
        : sourceValue > maximum ? maximum : sourceValue;

    controls_.setSize(ui().getDisplayWidth(), ui().getDisplayHeight());

    const int32_t totalWidth =
        digitCount_ * DIGIT_WIDTH + (digitCount_ - 1U) * DIGIT_GAP;
    const int32_t startX = ui().getDisplayWidth() > totalWidth
        ? (ui().getDisplayWidth() - totalWidth) / 2
        : 0;
    const int32_t popupTop = ui().getDisplayHeight() > popupHeight()
        ? (ui().getDisplayHeight() - popupHeight()) / 2
        : 0;
    const bool hasTitle = title_ != nullptr && title_[0] != '\0';
    const int32_t digitY = popupTop + (hasTitle ? 18 : 8);
    const int32_t actionWidth = OK_WIDTH + ACTION_GAP + CANCEL_WIDTH;
    const int32_t actionX = ui().getDisplayWidth() > actionWidth
        ? (ui().getDisplayWidth() - actionWidth) / 2
        : 0;
    const int32_t actionY =
        popupTop + popupHeight() - ACTION_HEIGHT - CONTENT_PADDING;

    int32_t divisor = 1;
    for (uint8_t i = 1; i < digitCount_; ++i) divisor *= 10;
    for (uint8_t index = 0; index < digitCount_; ++index) {
        NumScroll* digit = digitPool_.create(
            ui(),
            static_cast<uint16_t>(startX + index * (DIGIT_WIDTH + DIGIT_GAP)),
            static_cast<uint16_t>(digitY),
            static_cast<uint16_t>(DIGIT_WIDTH),
            static_cast<uint16_t>(DIGIT_HEIGHT),
            digitRange_,
            NumericFormatter::integer(DIGIT_FORMAT));
        assert(digit != nullptr);
        digits_[index] = digit;
        digit->setPresentation(NumScroll::Presentation::Bare);
        digit->setValueImmediate((displayedValue / divisor) % 10);
        digit->onLoadNoAnim();
        const bool digitAdded = controls_.addChild(*digit);
        assert(digitAdded);
        (void)digitAdded;
        divisor /= 10;
    }

    okButton_.setPosition(
        static_cast<uint16_t>(actionX), static_cast<uint16_t>(actionY));
    cancelButton_.setPosition(
        static_cast<uint16_t>(actionX + OK_WIDTH + ACTION_GAP),
        static_cast<uint16_t>(actionY));
    okButton_.setCallback([this]() { commitEditing(); });
    cancelButton_.setCallback([this]() { cancelEditing(); });
    okButton_.onLoadNoAnim();
    cancelButton_.onLoadNoAnim();
    const bool okAdded = controls_.addChild(okButton_);
    const bool cancelAdded = controls_.addChild(cancelButton_);
    const bool controlsAdded = focusManager_.addWidget(&controls_);
    assert(okAdded && cancelAdded && controlsAdded);
    (void)okAdded;
    (void)cancelAdded;
    (void)controlsAdded;
}

PopupValueDigits::~PopupValueDigits() {
    if (finalizationState_ == FinalizationState::Editing) {
        const bool cancelled = cancelEditing(false);
        assert(cancelled);
        (void)cancelled;
    }
    destroyControls();
    ui().markDirty();
}

/** @brief PopupValueDigits::destroyControls. */
void PopupValueDigits::destroyControls() {
    focusManager_.clear();
    controls_.removeAllChildren();
    for (uint8_t i = 0; i < digitCount_; ++i) {
        if (digits_[i]) {
            digitPool_.destroy(digits_[i]);
            digits_[i] = nullptr;
        }
    }
}

/** @brief PopupValueDigits::collectValue. */
int32_t PopupValueDigits::collectValue() const {
    int32_t result = 0;
    for (uint8_t i = 0; i < digitCount_; ++i) {
        result = result * 10 + digits_[i]->getValue();
    }
    return result;
}

/** @brief PopupValueDigits::synchronizeValue. */
bool PopupValueDigits::synchronizeValue() {
    if (session_ == nullptr || !session_->valid()) return false;
    const int32_t newValue = collectValue();
    if (newValue == session_->draftValue()) return true;
    if (!session_->setDraftValue(newValue)) {
        restoreDigitsFromDraft();
        return false;
    }
    if (compatibilityCallback_ && session_->policy() == ValueEditPolicy::Live) {
        compatibilityCallback_(newValue);
    }
    ui().markDirty();
    return true;
}

/** @brief PopupValueDigits::restoreDigitsFromDraft. */
void PopupValueDigits::restoreDigitsFromDraft() {
    if (session_ == nullptr || !session_->valid()) return;
    const int32_t maximum = maximumValue(digitCount_);
    const int32_t draft = session_->draftValue();
    const int32_t displayedValue = draft < 0
        ? 0
        : draft > maximum ? maximum : draft;
    int32_t divisor = 1;
    for (uint8_t i = 1; i < digitCount_; ++i) divisor *= 10;
    for (uint8_t i = 0; i < digitCount_; ++i) {
        if (digits_[i] != nullptr) {
            digits_[i]->setValueImmediate((displayedValue / divisor) % 10);
        }
        divisor /= 10;
    }
    ui().markDirty();
}

/** @brief PopupValueDigits::commitEditing. */
bool PopupValueDigits::commitEditing() {
    if (finalizationState_ != FinalizationState::Editing) return true;
    if (!synchronizeValue()) return false;
    const bool changed = session_->draftValue() != session_->originalValue();
    if (!session_->commit()) return false;
    finalizationState_ = FinalizationState::Committed;
    if (changed && compatibilityCallback_ &&
        session_->policy() == ValueEditPolicy::CommitOnConfirm) {
        compatibilityCallback_(session_->draftValue());
    }
    requestClose();
    return true;
}

/** @brief PopupValueDigits::cancelEditing. */
bool PopupValueDigits::cancelEditing(bool closePopup) {
    if (finalizationState_ != FinalizationState::Editing) return true;
    if (session_ == nullptr) return false;
    const bool changed = session_->draftValue() != session_->originalValue();
    const int32_t original = session_->originalValue();
    if (!session_->cancel()) return false;
    finalizationState_ = FinalizationState::Cancelled;
    restoreDigitsFromDraft();
    if (changed && compatibilityCallback_ &&
        session_->policy() == ValueEditPolicy::Live) {
        compatibilityCallback_(original);
    }
    if (closePopup) requestClose();
    return true;
}

/** @brief PopupValueDigits::onClosing. */
bool PopupValueDigits::onClosing() {
    return finalizationState_ != FinalizationState::Editing ||
           cancelEditing(false);
}

/** @brief PopupValueDigits::drawContent. */
void PopupValueDigits::drawContent(const PopupContentBounds& bounds) {
    U8G2& u8g2 = ui().getU8G2();
    if (title_ && title_[0] != '\0') {
        u8g2.setFont(PIXELUI_FONT_TEXT);
        const int16_t titleWidth = u8g2.getUTF8Width(title_);
        const int16_t popupTop = static_cast<int16_t>(
            (ui().getDisplayHeight() - popupHeight()) / 2);
        u8g2.drawUTF8(bounds.centerX - titleWidth / 2, popupTop + 13, title_);
    }

    // Popup geometry is screen-fixed, while NumScroll draws through the
    // camera-aware Canvas. Express the screen bounds in canvas coordinates so
    // Canvas translates the digits back onto the popup without changing the
    // underlying application's camera state.
    const CanvasCamera& camera = ui().getCanvas().camera();
    const WidgetRenderContext context{
        camera.x(), camera.y(),
        {bounds.x + camera.x(), bounds.y + camera.y(),
         bounds.width, bounds.height}};
    controls_.draw(context);
    setContentClip(bounds);
    focusManager_.draw();
}

/** @brief PopupValueDigits::handleContentInput. */
bool PopupValueDigits::handleContentInput(InputEvent event) {
    IWidget* activeWidget = focusManager_.getActiveWidget();
    if (activeWidget) {
        if (event == InputEvent::BACK) {
            cancelEditing();
            return true;
        }
        const bool finished = activeWidget->handleEvent(event);
        if (!synchronizeValue()) return true;
        if (finished) {
            focusManager_.clearActiveWidget();
        }
        return true;
    }

    switch (event) {
        case InputEvent::BACK:
            cancelEditing();
            break;
        case InputEvent::RIGHT:
            focusManager_.moveNext();
            break;
        case InputEvent::LEFT:
            focusManager_.movePrev();
            break;
        case InputEvent::SELECT:
            focusManager_.selectCurrent();
            break;
        default:
            break;
    }
    return true;
}

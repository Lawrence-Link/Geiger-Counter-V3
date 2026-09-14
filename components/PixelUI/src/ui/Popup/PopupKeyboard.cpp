/*
 * Copyright (C) 2025 Lawrence Link
 */

#include "ui/Popup/PopupKeyboard.h"
#include "PixelUI.h"
#include <assert.h>
#include <string.h>

namespace {

constexpr const char* KEY_LABELS[] = {
    "01,@.", "2!ABC", "3@DEF", "4#GHI", "5$JKL", "6%MNO",
    "7^PQRS", "8&TUV", "9*WXYZ"
};

constexpr const char* KEY_CHARS[] = {
    "01,@.", "2!ABCabc", "3@DEFdef", "4#GHIghi", "5$JKLjkl", "6%MNOmno",
    "7^PQRSpqrs", "8&TUVtuv", "9*WXYZwxyz"
};

} // namespace

PopupKeyboard::ControlGroup::ControlGroup(PixelUI& ui) : ui_(ui) {
    setClipChildren(false);
}

void PopupKeyboard::ControlGroup::setSize(uint16_t width, uint16_t height) {
    setWidgetBounds({0, 0, width, height});
}

Canvas& PopupKeyboard::ControlGroup::display() {
    return ui_.getCanvas();
}

bool PopupKeyboard::isValidBuffer(
    const char* output, size_t outputCapacity) {
    return output != nullptr && outputCapacity > 0U &&
           outputCapacity <= static_cast<size_t>(MAX_TEXT_LENGTH + 1) &&
           strnlen(output, outputCapacity) < outputCapacity;
}

PopupKeyboard::PopupKeyboard(
    PixelUI& ui, uint16_t width, uint16_t height,
    char* output, size_t outputCapacity, uint16_t duration,
    VoidCallback commitCallback)
    : PopupBase(ui, width, height, duration),
      output_(output),
      outputCapacity_(outputCapacity),
      commitCallback_(etl::move(commitCallback)),
      controls_(ui),
      deleteButton_(ui, 0, 0, DELETE_WIDTH, ACTION_HEIGHT,
                    "DEL", PIXELUI_FONT_TINY),
      okButton_(ui, 0, 0, OK_WIDTH, ACTION_HEIGHT,
                "OK", PIXELUI_FONT_TINY),
      focusManager_(ui) {
    assert(isValidLayout(width, height));
    assert(isValidBuffer(output, outputCapacity));
    assert(width <= ui.getDisplayWidth() && height <= ui.getDisplayHeight());
    length_ = strnlen(output_, outputCapacity_);
    memcpy(draft_, output_, length_ + 1U);
    initializeControls();
}

PopupKeyboard::~PopupKeyboard() {
    destroyControls();
    ui().markDirty();
}

void PopupKeyboard::initializeControls() {
    controls_.setSize(ui().getDisplayWidth(), ui().getDisplayHeight());
    const int32_t popupLeft = (ui().getDisplayWidth() - popupWidth()) / 2;
    const int32_t popupTop = (ui().getDisplayHeight() - popupHeight()) / 2;
    const int32_t keyAreaWidth = popupWidth() - 2 * CONTENT_PADDING;
    const int32_t keyWidth =
        (keyAreaWidth - (KEY_COLUMNS - 1U) * KEY_GAP) / KEY_COLUMNS;
    const int32_t keyStartX = popupLeft +
        (popupWidth() - (KEY_COLUMNS * keyWidth +
                         (KEY_COLUMNS - 1U) * KEY_GAP)) / 2;

    for (uint8_t index = 0U; index < KEY_COUNT; ++index) {
        const uint8_t row = index / KEY_COLUMNS;
        const uint8_t column = index % KEY_COLUMNS;
        TextButton* key = keyPool_.create(
            ui(),
            static_cast<uint16_t>(keyStartX + column * (keyWidth + KEY_GAP)),
            static_cast<uint16_t>(popupTop + KEY_TOP +
                                  row * (KEY_HEIGHT + KEY_GAP)),
            static_cast<uint16_t>(keyWidth), KEY_HEIGHT,
            KEY_LABELS[index], PIXELUI_FONT_TINY);
        assert(key != nullptr);
        keys_[index] = key;
        key->setCallback([this, index]() { pressKey(index); });
        key->onLoadNoAnim();
        const bool added = controls_.addChild(*key);
        assert(added);
        (void)added;
    }

    const int32_t actionWidth = DELETE_WIDTH + ACTION_GAP + OK_WIDTH;
    const int32_t actionX = popupLeft + popupWidth() -
                            CONTENT_PADDING - actionWidth;
    const int32_t actionY = popupTop + CONTENT_PADDING;
    deleteButton_.setPosition(
        static_cast<uint16_t>(actionX), static_cast<uint16_t>(actionY));
    okButton_.setPosition(
        static_cast<uint16_t>(actionX + DELETE_WIDTH + ACTION_GAP),
        static_cast<uint16_t>(actionY));
    deleteButton_.setCallback([this]() { eraseLast(); });
    okButton_.setCallback([this]() { commitEditing(); });
    deleteButton_.onLoadNoAnim();
    okButton_.onLoadNoAnim();
    const bool deleteAdded = controls_.addChild(deleteButton_);
    const bool okAdded = controls_.addChild(okButton_);
    const bool controlsAdded = focusManager_.addWidget(&controls_);
    assert(deleteAdded && okAdded && controlsAdded);
    (void)deleteAdded;
    (void)okAdded;
    (void)controlsAdded;
}

void PopupKeyboard::destroyControls() {
    focusManager_.clear();
    controls_.removeAllChildren();
    for (TextButton*& key : keys_) {
        if (key != nullptr) {
            keyPool_.destroy(key);
            key = nullptr;
        }
    }
}

void PopupKeyboard::pressKey(uint8_t index) {
    const char* chars = KEY_CHARS[index];
    const uint8_t count = static_cast<uint8_t>(strlen(chars));
    const uint32_t now = ui().getCurrentTime();
    const bool continuing = lastKey_ == static_cast<int8_t>(index) &&
                            length_ != 0U &&
                            now - lastPressTime_ <= MULTI_TAP_TIMEOUT_MS;

    if (continuing) {
        lastCharIndex_ = static_cast<uint8_t>((lastCharIndex_ + 1U) % count);
        draft_[length_ - 1U] = chars[lastCharIndex_];
    } else {
        if (length_ + 1U >= outputCapacity_) return;
        lastCharIndex_ = 0U;
        draft_[length_++] = chars[0];
        draft_[length_] = '\0';
    }

    lastKey_ = static_cast<int8_t>(index);
    lastPressTime_ = now;
    ui().markDirty();
}

void PopupKeyboard::eraseLast() {
    if (length_ == 0U) return;
    do {
        --length_;
    } while (length_ > 0U &&
             (static_cast<uint8_t>(draft_[length_]) & 0xC0U) == 0x80U);
    draft_[length_] = '\0';
    lastKey_ = -1;
    lastPressTime_ = 0U;
    ui().markDirty();
}

void PopupKeyboard::commitEditing() {
    if (finalizationState_ != FinalizationState::Editing) return;
    memcpy(output_, draft_, length_ + 1U);
    finalizationState_ = FinalizationState::Committed;
    if (commitCallback_) commitCallback_();
    requestClose();
}

void PopupKeyboard::cancelEditing() {
    if (finalizationState_ != FinalizationState::Editing) return;
    finalizationState_ = FinalizationState::Cancelled;
    requestClose();
}

bool PopupKeyboard::onClosing() {
    if (finalizationState_ == FinalizationState::Editing) {
        finalizationState_ = FinalizationState::Cancelled;
    }
    return true;
}

void PopupKeyboard::drawContent(const PopupContentBounds& bounds) {
    U8G2& u8g2 = ui().getU8G2();
    const int32_t popupLeft = (ui().getDisplayWidth() - popupWidth()) / 2;
    const int32_t popupTop = (ui().getDisplayHeight() - popupHeight()) / 2;
    const int32_t actionWidth = DELETE_WIDTH + ACTION_GAP + OK_WIDTH;
    const int32_t textWidth = popupWidth() - 3 * CONTENT_PADDING - actionWidth;
    const char* visibleText = draft_;
    u8g2.setFont(PIXELUI_FONT_SMALL);
    while (*visibleText != '\0' && u8g2.getUTF8Width(visibleText) > textWidth) {
        do {
            ++visibleText;
        } while ((static_cast<uint8_t>(*visibleText) & 0xC0U) == 0x80U);
    }
    u8g2.drawUTF8(popupLeft + CONTENT_PADDING,
                  popupTop + CONTENT_PADDING + u8g2.getAscent(), visibleText);

    const CanvasCamera& camera = ui().getCanvas().camera();
    const WidgetRenderContext context{
        camera.x(), camera.y(),
        {bounds.x + camera.x(), bounds.y + camera.y(),
         bounds.width, bounds.height}};
    controls_.draw(context);
    setContentClip(bounds);
    focusManager_.draw();
}

bool PopupKeyboard::handleContentInput(InputEvent event) {
    if (event == InputEvent::BACK) {
        cancelEditing();
        return true;
    }
    focusManager_.handleInput(event);
    return true;
}

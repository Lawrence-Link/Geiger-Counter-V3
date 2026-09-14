/*
 * Copyright (C) 2025 Lawrence Link
 */

#include "ui/Popup/PopupProgress.h"
#include "PixelUI.h"

PopupProgress::PopupProgress(
    PixelUI& ui, uint16_t width, uint16_t height,
    const NumericRange& range, NumericFormatter formatter,
    ValueEditSession& session, const char* title, uint16_t duration,
    PopupProgressMode mode)
    : PopupBase(ui, width, height, duration),
      range_(range),
      defaultPercentage_{&range_, "%"},
      formatter_(formatter.valid()
          ? formatter
          : NumericFormatter::percentage(defaultPercentage_)),
      title_(title),
      ownedSession_(0),
      session_(&session),
      mode_(mode) {}

PopupProgress::PopupProgress(
    PixelUI& ui, uint16_t width, uint16_t height,
    const NumericRange& range, NumericFormatter formatter,
    ValueEditorBinding binding, const char* title, uint16_t duration,
    ValueCallback callback, ValueEditPolicy policy, PopupProgressMode mode)
    : PopupBase(ui, width, height, duration),
      range_(range),
      defaultPercentage_{&range_, "%"},
      formatter_(formatter.valid()
          ? formatter
          : NumericFormatter::percentage(defaultPercentage_)),
      title_(title),
      compatibilityCallback_(etl::move(callback)),
      ownedSession_(binding, policy),
      session_(&ownedSession_),
      sourceBinding_(binding),
      mode_(mode) {}

PopupProgress::PopupProgress(
    PixelUI& ui, uint16_t width, uint16_t height,
    const NumericRange& range, NumericFormatter formatter,
    int32_t initialValue, const char* title, uint16_t duration,
    PopupProgressMode mode)
    : PopupBase(ui, width, height, duration),
      range_(range),
      defaultPercentage_{&range_, "%"},
      formatter_(formatter.valid()
          ? formatter
          : NumericFormatter::percentage(defaultPercentage_)),
      title_(title),
      ownedSession_(initialValue),
      session_(&ownedSession_),
      mode_(mode) {}

bool PopupProgress::updateDraft(int32_t value) {
    if (session_ == nullptr || !session_->valid()) return false;
    value = range_.clamp(value);
    if (value == session_->draftValue()) return true;
    if (!session_->setDraftValue(value)) return false;
    if (compatibilityCallback_ && session_->policy() == ValueEditPolicy::Live) {
        compatibilityCallback_(value);
    }
    resetAutoCloseTimer();
    ui().markDirty();
    return true;
}

bool PopupProgress::commitEditing() {
    if (session_ == nullptr) return false;
    const bool changed = session_->draftValue() != session_->originalValue();
    if (!session_->commit()) return false;
    if (changed && compatibilityCallback_ &&
        session_->policy() == ValueEditPolicy::CommitOnConfirm) {
        compatibilityCallback_(session_->draftValue());
    }
    return true;
}

bool PopupProgress::cancelEditing() {
    if (session_ == nullptr) return false;
    const bool changed = session_->draftValue() != session_->originalValue();
    const int32_t original = session_->originalValue();
    if (!session_->cancel()) return false;
    if (changed && compatibilityCallback_ &&
        session_->policy() == ValueEditPolicy::Live) {
        compatibilityCallback_(original);
    }
    return true;
}

void PopupProgress::drawContent(const PopupContentBounds& bounds) {
    U8G2& display = ui().getU8G2();
    if (title_ != nullptr && title_[0] != '\0') {
        display.setFont(PIXELUI_FONT_TEXT);
        const int16_t titleWidth = display.getUTF8Width(title_);
        display.drawUTF8(bounds.centerX - titleWidth / 2,
                         bounds.centerY - 7, title_);
    }

    const int32_t barWidth = bounds.width > 20 ? bounds.width - 20 : 0;
    constexpr int32_t barHeight = 8;
    const int32_t barX = bounds.centerX - barWidth / 2;
    const int32_t barY = bounds.centerY - 3;
    if (barWidth > 0) display.drawFrame(barX, barY, barWidth, barHeight);

    int32_t value = session_ != nullptr ? session_->draftValue() : 0;
    if (mode_ == PopupProgressMode::ReadOnly) {
        sourceBinding_.read(value);
    }
    if (barWidth > 2) {
        const uint32_t fillWidth = normalizeToExtent(
            range_, value, static_cast<uint32_t>(barWidth - 2));
        if (fillWidth != 0U) {
            display.drawBox(barX + 1, barY + 1,
                            static_cast<int32_t>(fillWidth), barHeight - 2);
        }
    }

    char valueBuffer[32]{};
    if (formatter_.format(value, valueBuffer, sizeof(valueBuffer))) {
        const int16_t textWidth = display.getStrWidth(valueBuffer);
        display.drawStr(bounds.centerX - textWidth / 2,
                        bounds.centerY + 17, valueBuffer);
    }
}

bool PopupProgress::handleContentInput(InputEvent event) {
    if (session_ == nullptr || !session_->valid()) return false;
    if (mode_ == PopupProgressMode::ReadOnly) {
        switch (event) {
            case InputEvent::RIGHT:
            case InputEvent::LEFT:
                return true;
            case InputEvent::SELECT:
            case InputEvent::BACK:
                requestClose();
                return true;
            default:
                return false;
        }
    }
    switch (event) {
        case InputEvent::RIGHT:
            if (range_.canIncrement(session_->draftValue())) {
                updateDraft(range_.incremented(session_->draftValue()));
            }
            return true;
        case InputEvent::LEFT:
            if (range_.canDecrement(session_->draftValue())) {
                updateDraft(range_.decremented(session_->draftValue()));
            }
            return true;
        case InputEvent::SELECT:
            if (commitEditing()) requestClose();
            return true;
        case InputEvent::BACK:
            if (cancelEditing()) requestClose();
            return true;
        default:
            return false;
    }
}

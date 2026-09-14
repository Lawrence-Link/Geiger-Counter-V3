/*
 * Copyright (C) 2025 Lawrence Link
 */

#include "ui/Popup/PopupManager.h"
#if PIXELUI_USE_POPUP
#include "PixelUI.h"
#include "core/TimeUtils.h"

PopupManager::~PopupManager() {
    clearPopups();
}

bool PopupManager::validEnvelope(const RequestEnvelope& envelope) {
    return envelope.width != 0U && envelope.height != 0U;
}

bool PopupManager::enqueue(PopupRequest&& request) {
    if (dispatching_ || (getPopupCounts() >= MAX_POPUP_NUM)) {
        return false;
    }

    requests_.push(etl::move(request));
    if (active_ == nullptr) {
        activateNext();
    }
    return active_ != nullptr;
}

#if PIXELUI_USE_POPUP_INFO
bool PopupManager::enqueueInfo(uint16_t width, uint16_t height,
                               const char* text, const char* title,
                               uint16_t duration, const uint8_t* font) {
    const RequestEnvelope envelope{width, height, duration};
    if (!validEnvelope(envelope) || text == nullptr || font == nullptr) return false;
    PopupRequest request{InfoRequest{envelope, text, title, font}};
    return enqueue(etl::move(request));
}
#endif

#if PIXELUI_USE_POPUP_PROGRESS
bool PopupManager::enqueueProgress(uint16_t width, uint16_t height,
                                   ValueEditorBinding binding,
                                   const NumericRange& range,
                                   NumericFormatter formatter,
                                   const char* title, uint16_t duration,
                                   ValueCallback callback,
                                   ValueEditPolicy policy,
                                   PopupProgressMode mode) {
    const RequestEnvelope envelope{width, height, duration};
    int32_t currentValue = 0;
    if (!validEnvelope(envelope) || !binding.read(currentValue)) return false;
    PopupRequest request{ProgressRequest{
        envelope, policy, mode, range, formatter, binding, title,
        etl::move(callback)}};
    return enqueue(etl::move(request));
}

bool PopupManager::enqueueProgress(uint16_t width, uint16_t height,
                                   int32_t& value, int32_t minValue,
                                   int32_t maxValue, const char* title,
                                   uint16_t duration, ValueCallback callback,
                                   PopupProgressMode mode) {
    NumericRange range;
    if (!NumericRange::tryCreate(minValue, maxValue, 1, range)) return false;
    return enqueueProgress(
        width, height, ValueEditorBinding::reference(value), range,
        NumericFormatter{}, title, duration, etl::move(callback),
        ValueEditPolicy::CommitOnConfirm, mode);
}
#endif

#if PIXELUI_USE_POPUP_VALUE_DIGITS
bool PopupManager::enqueueValueDigits(uint16_t width, uint16_t height,
                                      ValueEditorBinding binding,
                                      uint8_t digitCount,
                                      const char* title, uint16_t duration,
                                      ValueCallback callback,
                                      ValueEditPolicy policy) {
    const RequestEnvelope envelope{width, height, duration};
    int32_t currentValue = 0;
    if (!validEnvelope(envelope) ||
        !PopupValueDigits::isValidLayout(width, height, digitCount) ||
        width > ui_.getDisplayWidth() || height > ui_.getDisplayHeight() ||
        !binding.read(currentValue)) return false;
    PopupRequest request{ValueDigitsRequest{
        envelope, digitCount, policy, binding, title, etl::move(callback)}};
    return enqueue(etl::move(request));
}

bool PopupManager::enqueueValueDigits(uint16_t width, uint16_t height,
                                      int32_t& value, uint8_t digitCount,
                                      const char* title, uint16_t duration,
                                      ValueCallback callback) {
    return enqueueValueDigits(
        width, height, ValueEditorBinding::reference(value), digitCount,
        title, duration, etl::move(callback),
        ValueEditPolicy::CommitOnConfirm);
}
#endif

void PopupManager::activateNext() {
    if ((active_ != nullptr) || requests_.empty()) {
        return;
    }

    PopupRequest request = etl::move(requests_.front());
    requests_.pop();

    dispatching_ = true;
    etl::visit([this](auto& payload) { activate(payload); }, request);
    dispatching_ = false;
}

#if PIXELUI_USE_POPUP_INFO
void PopupManager::activate(InfoRequest& request) {
    active_ = activePool_.create<PopupInfo>(
        ui_, request.envelope.width, request.envelope.height, request.text,
        request.title, request.envelope.duration, request.font);
}
#endif

#if PIXELUI_USE_POPUP_PROGRESS
void PopupManager::activate(ProgressRequest& request) {
    active_ = activePool_.create<PopupProgress>(
        ui_, request.envelope.width, request.envelope.height,
        request.range, request.formatter, request.binding, request.title,
        request.envelope.duration, etl::move(request.callback), request.policy,
        request.mode);
}
#endif

#if PIXELUI_USE_POPUP_VALUE_DIGITS
void PopupManager::activate(ValueDigitsRequest& request) {
    active_ = activePool_.create<PopupValueDigits>(
        ui_, request.envelope.width, request.envelope.height,
        request.binding, request.digitCount, request.title,
        request.envelope.duration, etl::move(request.callback), request.policy);
}
#endif

void PopupManager::destroyActive() {
    if (active_ == nullptr) {
        return;
    }

    IPopup* popup = active_;
    active_ = nullptr;
    dispatching_ = true;
    activePool_.destroy<IPopup>(popup);
    dispatching_ = false;
}

void PopupManager::clearPopups() {
    if (dispatching_) {
        return;
    }

    requests_.clear();
    destroyActive();
}

void PopupManager::drawPopups() {
    if (active_ == nullptr) {
        return;
    }

    dispatching_ = true;
    active_->draw();
    dispatching_ = false;
}

void PopupManager::updatePopups(uint32_t currentTime) {
    if (active_ == nullptr) {
        activateNext();
        return;
    }

    dispatching_ = true;
    const bool active = active_->update(currentTime);
    dispatching_ = false;
    if (!active) {
        destroyActive();
        activateNext();
        ui_.markDirty();
    }
}

uint32_t PopupManager::nextWakeupMs(
    uint32_t currentTime, uint32_t frameIntervalMs) const {
    if (active_ != nullptr) {
        return active_->nextWakeupMs(currentTime, frameIntervalMs);
    }
    return requests_.empty() ? PixelUITime::NO_WAKEUP : 0U;
}

bool PopupManager::handleTopPopupInput(InputEvent event) {
    if (active_ == nullptr) {
        return false;
    }

    dispatching_ = true;
    const bool handled = active_->handleInput(event);
    dispatching_ = false;
    return handled;
}
#endif

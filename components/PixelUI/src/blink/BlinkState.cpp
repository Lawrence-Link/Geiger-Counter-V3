/*
 * Copyright (C) 2025 Lawrence Link
 */

#include "blink/BlinkState.h"
#include "PixelUI.h"
#include "core/TimeUtils.h"

BlinkState::BlinkState(PixelUI& ui, uint32_t intervalMs)
    : UiDeadlineSource(ui.m_deadlineScheduler)
    , ui_(ui)
    , intervalMs_(PixelUITime::normalizeDeadlineInterval(intervalMs))
    , lastToggleMs_(ui.getCurrentTime()) {}

void BlinkState::start() {
    if (running_) return;

    running_ = true;
    stopWhenVisible_ = false;
    lastToggleMs_ = ui_.getCurrentTime();
    visible_ = true;
    ui_.markDirty();
}

void BlinkState::stop() {
    const bool changed = visible_;
    running_ = false;
    stopWhenVisible_ = false;
    visible_ = false;
    if (changed) ui_.markDirty();
}

void BlinkState::stopWhenVisible() {
    if (visible_) {
        running_ = false;
        stopWhenVisible_ = false;
        return;
    }
    stopWhenVisible_ = true;
}

void BlinkState::setInterval(uint32_t intervalMs) {
    intervalMs_ = PixelUITime::normalizeDeadlineInterval(intervalMs);
    if (running_) lastToggleMs_ = ui_.getCurrentTime();
}

uint32_t BlinkState::nextWakeupMs(uint32_t currentTime) const {
    if (!running_) return PixelUITime::NO_WAKEUP;
    return PixelUITime::untilDeadline(currentTime, lastToggleMs_ + intervalMs_);
}

bool BlinkState::update(uint32_t currentTime) {
    if (!running_) return false;

    const uint32_t elapsed = currentTime - lastToggleMs_;
    if (elapsed < intervalMs_) return false;

    const uint32_t toggles = elapsed / intervalMs_;
    lastToggleMs_ += toggles * intervalMs_;
    if ((toggles & 1U) != 0U) visible_ = !visible_;

    if (stopWhenVisible_ && visible_) {
        running_ = false;
        stopWhenVisible_ = false;
    }
    return (toggles & 1U) != 0U;
}

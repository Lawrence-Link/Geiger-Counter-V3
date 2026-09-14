/*
 * Copyright (C) 2025 Lawrence Link
 */

#include "core/scheduling/UiDeadlineScheduler.h"
#include "core/TimeUtils.h"

UiDeadlineSource::UiDeadlineSource(UiDeadlineScheduler& scheduler)
    : scheduler_(&scheduler) {
    scheduler.attach(*this);
}

UiDeadlineSource::~UiDeadlineSource() {
    if (scheduler_) scheduler_->detach(*this);
}

UiDeadlineScheduler::~UiDeadlineScheduler() {
    while (first_) {
        UiDeadlineSource* source = first_;
        first_ = source->next_;
        source->scheduler_ = nullptr;
        source->next_ = nullptr;
    }
}

void UiDeadlineScheduler::attach(UiDeadlineSource& source) {
    source.next_ = first_;
    first_ = &source;
}

void UiDeadlineScheduler::detach(UiDeadlineSource& source) {
    UiDeadlineSource** link = &first_;
    while (*link && *link != &source) link = &((*link)->next_);
    if (*link == &source) *link = source.next_;
    source.scheduler_ = nullptr;
    source.next_ = nullptr;
}

bool UiDeadlineScheduler::update(uint32_t currentTime) {
    bool redraw = false;
    for (UiDeadlineSource* source = first_; source; source = source->next_) {
        if (source->nextWakeupMs(currentTime) == 0U) {
            redraw = source->update(currentTime) || redraw;
        }
    }
    return redraw;
}

uint32_t UiDeadlineScheduler::nextWakeupMs(uint32_t currentTime) const {
    uint32_t next = PixelUITime::NO_WAKEUP;
    for (const UiDeadlineSource* source = first_; source; source = source->next_) {
        next = PixelUITime::earlier(next, source->nextWakeupMs(currentTime));
    }
    return next;
}

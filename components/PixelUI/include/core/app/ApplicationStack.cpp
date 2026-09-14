/*
 * Copyright (C) 2025 Lawrence Link
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES ARE DISCLAIMED.
 */

#include "core/app/ApplicationStack.h"

ApplicationStackResult ApplicationStack::prepareLayout(
    size_t objectSize,
    size_t alignment,
    Layout& layout) const {
    if (full()) {
        return ApplicationStackResult::StackFull;
    }
    if ((objectSize == 0U) || (alignment == 0U) ||
        ((alignment & (alignment - 1U)) != 0U) ||
        (alignment > APPLICATION_ARENA_MAX_ALIGNMENT)) {
        return ApplicationStackResult::ConstructionFailed;
    }

    const size_t paddingMask = alignment - 1U;
    if (offset_ > (APPLICATION_ARENA_SIZE - paddingMask)) {
        return ApplicationStackResult::ArenaFull;
    }

    const size_t alignedOffset = (offset_ + paddingMask) & ~paddingMask;
    if ((alignedOffset > APPLICATION_ARENA_SIZE) ||
        (objectSize > (APPLICATION_ARENA_SIZE - alignedOffset))) {
        return ApplicationStackResult::ArenaFull;
    }

    layout = Layout{offset_, alignedOffset, alignedOffset + objectSize};
    return ApplicationStackResult::Ok;
}

ApplicationStackResult ApplicationStack::emplace(
    const ApplicationFactory& factory,
    PixelUI& ui,
    void* parameters,
    IApplication*& application) {
    application = nullptr;
    if ((factory.construct_ == nullptr) || (factory.destroy_ == nullptr)) {
        return ApplicationStackResult::ConstructionFailed;
    }

    Layout layout{};
    const ApplicationStackResult layoutResult =
        prepareLayout(factory.objectSize_, factory.objectAlignment_, layout);
    if (layoutResult != ApplicationStackResult::Ok) {
        return layoutResult;
    }

    IApplication* constructed =
        factory.construct_(arena_ + layout.alignedOffset, ui, parameters);
    if (constructed == nullptr) {
        return ApplicationStackResult::ConstructionFailed;
    }

    const uintptr_t objectBegin = reinterpret_cast<uintptr_t>(arena_ + layout.alignedOffset);
    const uintptr_t objectEnd = reinterpret_cast<uintptr_t>(arena_ + layout.endOffset);
    const uintptr_t applicationAddress = reinterpret_cast<uintptr_t>(constructed);
    if ((applicationAddress < objectBegin) || (applicationAddress >= objectEnd)) {
        factory.destroy_(constructed);
        return ApplicationStackResult::ConstructionFailed;
    }

    commit(constructed, layout, factory.destroy_);
    application = constructed;
    return ApplicationStackResult::Ok;
}

void ApplicationStack::commit(
    IApplication* application,
    const Layout& layout,
    DestroyFunction destroy) {
    entries_[depth_] = Entry{application, layout.marker, layout.endOffset, destroy};
    ++depth_;
    offset_ = layout.endOffset;
}

void ApplicationStack::pop() {
    if (empty()) {
        return;
    }

    Entry& entry = entries_[depth_ - 1U];
    if ((entry.application != nullptr) && (entry.destroy != nullptr)) {
        entry.destroy(entry.application);
    }
    offset_ = entry.marker;
    entry = Entry{};
    --depth_;
}

void ApplicationStack::clear() {
    while (!empty()) {
        pop();
    }
}

IApplication* ApplicationStack::top() const noexcept {
    return empty() ? nullptr : entries_[depth_ - 1U].application;
}

IApplication* ApplicationStack::previous() const noexcept {
    return depth_ < 2U ? nullptr : entries_[depth_ - 2U].application;
}

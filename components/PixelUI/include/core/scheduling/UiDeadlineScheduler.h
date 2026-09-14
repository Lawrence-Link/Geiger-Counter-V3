/*
 * Copyright (C) 2025 Lawrence Link
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include <stdint.h>

class UiDeadlineScheduler;

class UiDeadlineSource {
public:
    /** @brief Detaches this source from its scheduler, if attached. */
    virtual ~UiDeadlineSource();

    /** @brief Copy construction is disabled because the scheduler owns the linkage. */
    UiDeadlineSource(const UiDeadlineSource&) = delete;
    /** @brief Copy assignment is disabled because the scheduler owns the linkage. */
    UiDeadlineSource& operator=(const UiDeadlineSource&) = delete;
    /** @brief Move construction is disabled because the scheduler owns the linkage. */
    UiDeadlineSource(UiDeadlineSource&&) = delete;
    /** @brief Move assignment is disabled because the scheduler owns the linkage. */
    UiDeadlineSource& operator=(UiDeadlineSource&&) = delete;

protected:
    /** @brief Attaches this source to a scheduler's deadline list. */
    explicit UiDeadlineSource(UiDeadlineScheduler& scheduler);

private:
    friend class UiDeadlineScheduler;

    /** @return Delay until this source needs an update, or NO_WAKEUP. */
    virtual uint32_t nextWakeupMs(uint32_t currentTime) const = 0;
    /** @return Whether updating this source changed the drawable state. */
    virtual bool update(uint32_t currentTime) = 0;

    UiDeadlineScheduler* scheduler_ = nullptr;
    UiDeadlineSource* next_ = nullptr;
};

class UiDeadlineScheduler {
public:
    /** @brief Creates an empty deadline source list. */
    UiDeadlineScheduler() = default;
    /** @brief Detaches all remaining sources without destroying them. */
    ~UiDeadlineScheduler();

    /** @brief Copy construction is disabled because the scheduler owns source linkage. */
    UiDeadlineScheduler(const UiDeadlineScheduler&) = delete;
    /** @brief Copy assignment is disabled because the scheduler owns source linkage. */
    UiDeadlineScheduler& operator=(const UiDeadlineScheduler&) = delete;

    /**
     * @brief Updates every source whose deadline has arrived.
     * @return Whether any source requested a redraw.
     */
    bool update(uint32_t currentTime);
    /** @return Earliest wakeup delay among all attached sources. */
    uint32_t nextWakeupMs(uint32_t currentTime) const;

private:
    friend class UiDeadlineSource;

    /** @brief Adds a source to the scheduler's non-owning linked list. */
    void attach(UiDeadlineSource& source);
    /** @brief Removes a source from the scheduler's non-owning linked list. */
    void detach(UiDeadlineSource& source);

    UiDeadlineSource* first_ = nullptr;
};

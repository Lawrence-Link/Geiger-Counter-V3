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

#include "core/app/app_system.h"
#include <etl/utility.h>

class ViewManager {
public:
    enum class LaunchResult {
        Ok,
        StackFull,
        ArenaFull,
        ConstructionFailed,
        TransitionInProgress,
    };

    /** @brief Creates a view manager bound to the supplied UI instance. */
    explicit ViewManager(PixelUI& ui);
    /** @brief Detaches the current view and destroys all stacked applications. */
    ~ViewManager();

    /** @brief Copy construction is disabled because the manager owns stack state. */
    ViewManager(const ViewManager&) = delete;
    /** @brief Copy assignment is disabled because the manager owns application state. */
    ViewManager& operator=(const ViewManager&) = delete;

    template <typename T, typename... Args>
    /**
     * @brief Constructs and pushes an application into the fixed application stack.
     * @tparam T Application type to construct.
     * @param args Constructor arguments forwarded to T.
     * @return Launch status, including stack, arena, and transition failures.
     */
    LaunchResult push(Args&&... args) {
        TransitionGuard transition(*this);
        if (!transition.acquired()) {
            return LaunchResult::TransitionInProgress;
        }

        T* application = nullptr;
        const ApplicationStackResult result =
            m_applicationStack.emplace<T>(application, etl::forward<Args>(args)...);
        if (result != ApplicationStackResult::Ok) {
            return toLaunchResult(result);
        }

        activatePushedApplication(application);
        return LaunchResult::Ok;
    }

    /**
     * @brief Constructs and pushes the application described by an AppItem.
     * @param item Factory and metadata for the application.
     * @param parameters Optional caller parameter passed to the factory.
     * @return Launch status.
     */
    LaunchResult launch(const AppItem& item, void* parameters = nullptr);
    /** @brief Exits and removes the current application, if one is active. */
    bool pop();
    /** @return Whether an application transition is currently pending or active. */
    bool isTransitioning() const noexcept {
        return m_pendingEnter != nullptr || m_isTransitioning;
    }

    /** @return The application at the top of the stack, or nullptr when empty. */
    IApplication* getCurrentApp() const;
    /** @return Number of applications currently in the stack. */
    size_t getViewDepth() const noexcept { return m_applicationStack.depth(); }
    /** @return Bytes currently used by the application arena. */
    size_t getArenaUsed() const noexcept { return m_applicationStack.used(); }
    /** @return Total bytes available in the application arena. */
    static constexpr size_t getArenaCapacity() noexcept { return ApplicationStack::capacity(); }

private:
    friend class PixelUI;

    class TransitionGuard {
    public:
        /** @brief Acquires exclusive ownership of an in-progress transition. */
        explicit TransitionGuard(ViewManager& manager) : manager_(manager) {
            if (manager_.m_pendingEnter != nullptr) {
                return;
            }
            if (manager_.m_isTransitioning) {
                return;
            }
            manager_.m_isTransitioning = true;
            acquired_ = true;
        }

        /** @brief Releases the transition marker when this guard acquired it. */
        ~TransitionGuard() {
            if (acquired_) {
                manager_.m_isTransitioning = false;
            }
        }

        /** @return Whether this guard acquired the transition slot. */
        bool acquired() const noexcept { return acquired_; }

    private:
        ViewManager& manager_;
        bool acquired_ = false;
    };

    /** @brief Converts an application-stack result to a view launch result. */
    static LaunchResult toLaunchResult(ApplicationStackResult result);
    /** @brief Routes input to popups, the current application, or its camera. */
    void attachInputRouter();
    /** @brief Pauses the previous application and prepares the pushed application. */
    void activatePushedApplication(IApplication* application);
    /** @brief Completes an application entry deferred until fading finishes. */
    void completePendingEnter();
    /** @brief Enters an application and optionally starts its horizontal transition. */
    void enterApplication(IApplication* application);
    /** @brief Clears UI references owned by the outgoing application. */
    void clearNonOwningReferences();
    /** @brief Restores the saved camera state for the current application. */
    void restoreCurrentCameraState();
    /** @return Whether a transition commit is currently in progress. */
    bool isTransitionCommitInProgress() const noexcept {
        return m_isTransitioning;
    }

    PixelUI &m_ui;
    ApplicationStack m_applicationStack;
    struct CameraState {
        int32_t y = 0;
        int32_t contentHeight = 0;
    };
    CameraState m_cameraStates[MAX_VIEW_DEPTH]{};
    IApplication* m_pendingEnter = nullptr;
    bool m_isTransitioning = false;
};

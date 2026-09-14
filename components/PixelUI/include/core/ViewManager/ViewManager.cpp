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

#include "core/ViewManager/ViewManager.h"
#include "PixelUI.h"

namespace {
constexpr int32_t ENTER_TRANSITION_START_X = -5;
constexpr uint32_t ENTER_TRANSITION_DURATION_MS = 150U;
}

ViewManager::ViewManager(PixelUI& ui) : m_ui(ui) {}

void ViewManager::attachInputRouter() {
    m_ui.setInputCallback([this](InputEvent event) -> bool {
        if (isTransitioning()) {
            return false;
        }

#if PIXELUI_USE_POPUP
        if (m_ui.popupCount() > 0U) {
            return m_ui.m_popupManager.handleTopPopupInput(event);
        }
#endif

        IApplication* current = m_applicationStack.top();
        if (current == nullptr) return false;
        if (current->handleInput(event)) return true;
        if (current->useVerticalScroll && m_ui.getCanvas().camera().handleInput(event)) {
            m_ui.markDirty();
            return true;
        }
        return false;
    });
}

ViewManager::~ViewManager() {
    m_isTransitioning = true;
    m_ui.setDrawable(nullptr);
    m_ui.clearInputCallback();
    clearNonOwningReferences();

    while (!m_applicationStack.empty()) {
        m_applicationStack.top()->clearExitCallback();
        m_applicationStack.pop();
    }
}

ViewManager::LaunchResult ViewManager::launch(const AppItem& item, void* parameters) {
    TransitionGuard transition(*this);
    if (!transition.acquired()) {
        return LaunchResult::TransitionInProgress;
    }

    IApplication* application = nullptr;
    const ApplicationStackResult result =
        m_applicationStack.emplace(item.factory, m_ui, parameters, application);
    if (result != ApplicationStackResult::Ok) {
        return toLaunchResult(result);
    }

    activatePushedApplication(application);
    return LaunchResult::Ok;
}

bool ViewManager::pop() {
    TransitionGuard transition(*this);
    if (!transition.acquired() || m_applicationStack.empty()) {
        return false;
    }

    IApplication* application = m_applicationStack.top();
    m_ui.setDrawable(nullptr);
    application->onExit();
    application->clearExitCallback();

    m_ui.markFading();
    clearNonOwningReferences();
    m_applicationStack.pop();

    IApplication* previousApplication = m_applicationStack.top();
    if (previousApplication != nullptr) {
        restoreCurrentCameraState();
        m_ui.setDrawable(previousApplication);
        previousApplication->onResume();
    } else {
        m_ui.getCanvas().camera().setEnabled(false);
        m_ui.getCanvas().camera().setX(0);
        m_ui.getCanvas().camera().setY(0);
    }

    m_ui.markDirty();
    return true;
}

IApplication* ViewManager::getCurrentApp() const {
    return m_applicationStack.top();
}

ViewManager::LaunchResult ViewManager::toLaunchResult(ApplicationStackResult result) {
    switch (result) {
        case ApplicationStackResult::Ok: return LaunchResult::Ok;
        case ApplicationStackResult::StackFull: return LaunchResult::StackFull;
        case ApplicationStackResult::ArenaFull: return LaunchResult::ArenaFull;
        case ApplicationStackResult::ConstructionFailed: return LaunchResult::ConstructionFailed;
        default: return LaunchResult::ConstructionFailed;
    }
}

void ViewManager::activatePushedApplication(IApplication* application) {
    IApplication* previousApplication = m_applicationStack.previous();
    if (previousApplication != nullptr) {
        const size_t currentDepth = m_applicationStack.depth();
        if (currentDepth >= 2U) {
            m_cameraStates[currentDepth - 2U].y = m_ui.getCanvas().camera().storedY();
            m_cameraStates[currentDepth - 2U].contentHeight =
                m_ui.getCanvas().camera().contentHeight();
        }
        clearNonOwningReferences();
        previousApplication->onPause();
    }

    m_ui.setDrawable(application);
    m_cameraStates[m_applicationStack.depth() - 1U] = CameraState{};
    m_ui.getCanvas().camera().setEnabled(application->useVerticalScroll);
    m_ui.getCanvas().camera().setX(0);
    m_ui.getCanvas().camera().setContentHeight(0);
    m_ui.getCanvas().camera().setY(0);
    m_ui.clearFocusManager();
    if (previousApplication != nullptr && m_ui.isFading()) {
        m_pendingEnter = application;
    } else {
        enterApplication(application);
    }
    m_ui.markDirty();
}

void ViewManager::completePendingEnter() {
    if (m_pendingEnter == nullptr) {
        return;
    }

    IApplication* application = m_pendingEnter;
    enterApplication(application);
    m_pendingEnter = nullptr;
    m_ui.markDirty();
}

void ViewManager::enterApplication(IApplication* application) {
    CanvasCamera& camera = m_ui.getCanvas().camera();
    const int32_t startX = application->m_enterTransitionEnabled
        ? ENTER_TRANSITION_START_X
        : 0;
    camera.setX(startX);
    if (application->m_enterTransitionEnabled &&
        !m_ui.animateCallback(
            startX, 0, ENTER_TRANSITION_DURATION_MS,
            EasingType::EASE_OUT_CUBIC,
            [this](int32_t value) {
                m_ui.getCanvas().camera().setX(value);
            })) {
        camera.setX(0);
    }

    application->onEnter([this]() { pop(); });
    m_ui.getCanvas().camera().setEnabled(application->useVerticalScroll);
}

void ViewManager::clearNonOwningReferences() {
    m_ui.clearAllAnimations();
    m_ui.clearAllCoroutines();
    m_ui.clearFocusManager();
    m_ui.clearPopups();
    m_ui.getCanvas().camera().setX(0);
}

void ViewManager::restoreCurrentCameraState() {
    IApplication* application = m_applicationStack.top();
    if (application == nullptr) return;
    const CameraState& state = m_cameraStates[m_applicationStack.depth() - 1U];
    m_ui.getCanvas().camera().setEnabled(application->useVerticalScroll);
    m_ui.getCanvas().camera().setX(0);
    m_ui.getCanvas().camera().setContentHeight(state.contentHeight);
    m_ui.getCanvas().camera().setY(state.y);
}

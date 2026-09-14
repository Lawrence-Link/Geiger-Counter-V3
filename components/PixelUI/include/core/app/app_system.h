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
#include <etl/vector.h>
#include "ApplicationStack.h"
#include "config.h"

enum class MenuItemType {
    Action,
    App,
};

struct AppItem {
    /* Name of the app */
    const char* title;
    /* Bitmap to the app icon */
    const uint8_t* bitmap;
    /* Factory function of the app */
    ApplicationFactory factory;

    template <typename T>
    /**
     * @brief Creates an application item using T's default factory.
     * @param title Non-owning application title.
     * @param bitmap Non-owning application icon bitmap.
     */
    static constexpr AppItem make(const char* title, const uint8_t* bitmap) {
        return AppItem{title, bitmap, ApplicationFactory::make<T>()};
    }

    template <typename T>
    /**
     * @brief Creates an application item using a custom construction callback.
     * @param title Non-owning application title.
     * @param bitmap Non-owning application icon bitmap.
     * @param customConstruct Callback that constructs T in supplied storage.
     */
    static constexpr AppItem make(
        const char* title,
        const uint8_t* bitmap,
        ApplicationFactory::ConstructFunction customConstruct) {
        return AppItem{title, bitmap, ApplicationFactory::makeCustom<T>(customConstruct)};
    }
};

class AppManager {
public:
    /** @return The process-wide application registry. */
    static AppManager& getInstance() {
        static AppManager instance;
        return instance;
    }
    /** @brief Adds an application unless the fixed registry is full. */
    void registerApp(const AppItem& item);

    /** @return Read-only access to the registered application items. */
    const etl::vector<AppItem, MAX_APP_NUM>& getAppVector() const;

    /** @return Number of application items currently registered. */
    size_t getRegisteredCount() const { return appItems_.size(); }
    /** @brief Copy construction is disabled for the process-wide registry. */
    AppManager(const AppManager&) = delete;
    /** @brief Copy assignment is disabled for the process-wide registry. */
    AppManager& operator=(const AppManager&) = delete;
private:
    /** @brief Creates an empty application registry. */
    AppManager() = default;
    etl::vector<AppItem, MAX_APP_NUM> appItems_;
};

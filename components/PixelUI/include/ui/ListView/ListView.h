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

#include "PixelUI.h"
#include <etl/algorithm.h>
#include "etl/vector.h"
#include "etl/delegate.h"
#include "core/animation/animation.h"
#include "core/app/IApplication.h"
#include "core/ValueBinding.h"
#if PIXELUI_USE_LABEL_SCROLL
#include "widgets/label/label.h"
#endif
#include <stdint.h>

class ListItemAccessory {
public:
    enum class Kind : uint8_t {
        None,
        Text,
        Toggle,
        Check,
        Value,
    };

    // Accessory targets are non-owning and must outlive the containing ListView.
/** @brief text. */
    static constexpr ListItemAccessory text(const char* text) {
        ListItemAccessory accessory;
        accessory.kind_ = Kind::Text;
        accessory.payload_.text = text;
        return accessory;
    }

/** @brief toggle. */
    static constexpr ListItemAccessory toggle(bool& value) {
        ListItemAccessory accessory;
        accessory.kind_ = Kind::Toggle;
        accessory.payload_.toggle = &value;
        return accessory;
    }

    /** @brief Displays a read-only check state; the item callback owns changes. */
    static constexpr ListItemAccessory check(bool& value) {
        ListItemAccessory accessory;
        accessory.kind_ = Kind::Check;
        accessory.payload_.toggle = &value;
        return accessory;
    }

/** @brief value. */
    static constexpr ListItemAccessory value(PixelUIValue::Binding binding) {
        ListItemAccessory accessory;
        accessory.kind_ = Kind::Value;
        accessory.payload_.value = {
            binding.object(),
            binding.formatter(),
            binding.suffix(),
        };
        return accessory;
    }

/** @brief kind. */
    constexpr Kind kind() const { return kind_; }

    constexpr const char* textValue() const {
        return kind_ == Kind::Text ? payload_.text : nullptr;
    }

/** @brief toggleValue. */
    constexpr bool* toggleValue() const {
        return kind_ == Kind::Toggle ? payload_.toggle : nullptr;
    }

    /** @brief Returns the non-owning check-state binding. */
    constexpr bool* checkValue() const {
        return kind_ == Kind::Check ? payload_.toggle : nullptr;
    }

/** @brief formatValue. */
    bool formatValue(char* buffer, size_t bufferSize) const {
        if (buffer != nullptr && bufferSize != 0U) buffer[0] = '\0';
        if (kind_ != Kind::Value || payload_.value.formatter == nullptr) {
            return false;
        }
        const bool formatted = payload_.value.formatter(
            payload_.value.object,
            payload_.value.suffix,
            buffer,
            bufferSize);
        if (!formatted && buffer != nullptr && bufferSize != 0U) {
            buffer[0] = '\0';
        }
        return formatted;
    }

private:
    struct ValuePayload {
        const void* object;
        PixelUIValue::Binding::FormatFunction formatter;
        const char* suffix;
    };

    union Payload {
/** @brief Payload. */
        constexpr Payload() : text(nullptr) {}

        const char* text;
        bool* toggle;
        ValuePayload value;
    };

    Kind kind_ = Kind::None;
    Payload payload_{};
};

// Represents a single item in a list view.
struct ListItem{
    mutable char title[MAX_LISTITEM_NAME_NUM]; // The display title of the item. 'mutable' allows it to be changed even if the struct is 'const'.
    ListItem * nextList = nullptr;                       // Pointer to a sub-menu (another list).
    int32_t nextListLength = 0;                     // The number of items in the sub-menu. (signed to avoid mixed-signedness)
    // The callback object is owned; references captured by it are non-owning.
    VoidCallback pFunc = nullptr;               // A function to execute when the item is selected.
    ListItemAccessory accessory{};
    bool use_fade = false; // Whether render fade animation when navigate to new app.
};

// The main class for handling a list-based user interface.
class ListView : public IApplication {
public:
    // Constructor to initialize the list view with a UI handler and a list of items.
    // itemList, every submenu, accessory target, and callback capture are
    // non-owning and must remain valid for this ListView's lifetime.
    // NOTE: length is signed to match internal indices.
/** @brief ListView. */
    ListView(PixelUI& ui, ListItem *itemList, int length);
/** @brief ~ListView. */
    ~ListView() override;

    // --- Application Lifecycle and Input Handlers ---
/** @brief draw. */
    void draw() override;
/** @brief handleInput. */
    bool handleInput(InputEvent event) override;
/** @brief onEnter. */
    void onEnter(ExitCallback exitCallback) override;
/** @brief onResume. */
    void onResume() override ;
/** @brief onPause. */
    void onPause() override;
/** @brief onExit. */
    void onExit() override;

/** @brief onLoad. */
    virtual void onLoad() = 0;
/** @brief onSave. */
    virtual void onSave() = 0;

    // --- Public Utility Methods ---
/** @brief resizeLength. */
    void resizeLength(int itemLength) { m_itemLength = itemLength; }
/** @brief getUI. */
    PixelUI& getUI() { return m_ui; }
    
    PixelUI& m_ui; // Reference to the main UI class.
protected:
/** @brief toggleBoxXFor. */
    int32_t toggleBoxXFor(const ListItem& item) const;

private:
    static constexpr int32_t SPACING = 7;
    static constexpr int32_t TOP_MARGIN = 3;
    static constexpr int32_t CURSOR_X = 1;
    static constexpr int32_t TITLE_X = 4;
    static constexpr int32_t TITLE_ACCESSORY_GAP = 3;
    static constexpr int32_t VISIBLE_ITEM_COUNT = LISTVIEW_ITEMS_PER_PAGE;

    ListItem* m_itemList;
    int32_t m_itemLength; // signed index length (last index). Avoid mixing signed/unsigned.
#if PIXELUI_USE_LABEL_SCROLL
    Label selectedItemTitle_;
#endif
    
    struct SwitchAnimState {
        ListItem* item = nullptr;
        int32_t boxX = 0;
        AnimationHandle handle = INVALID_ANIMATION_HANDLE;
    } switchAnimState_;

    static constexpr size_t MAX_VIEW_ANIMATIONS = 5U;
    static constexpr size_t MAX_LOAD_ANIMATIONS = LISTVIEW_ITEMS_PER_PAGE + 1U;
    etl::vector<AnimationHandle, MAX_VIEW_ANIMATIONS> viewAnimations_;
    etl::vector<AnimationHandle, MAX_LOAD_ANIMATIONS> loadAnimations_;

    // History stack to support nested menus (for back navigation).
    etl::vector<etl::pair<etl::pair<ListItem*, int32_t>, int32_t>, MAX_LISTVIEW_DEPTH> m_history_stack;

    // --- Cursor Variables ---
    int32_t CursorY = -6;
    int32_t CursorWidth = 0;
    
    // --- Scroll Variables ---
    int32_t topVisibleIndex_ = 0;           // Index of the first item visible on screen.
    
    // --- Load Animation Variables ---
    int32_t itemLoadAnimations_[LISTVIEW_ITEMS_PER_PAGE + 1]; // Tracks animation progress for each item.
    bool isInitialLoad_ = true;
    uint8_t FontHeight = 0;
    
    // --- Progress Bar Variables ---
    int32_t progress_bar_top = 0;
    int32_t progress_bar_bottom = 0;

    // --- Navigation and Drawing Methods ---
/** @brief navigateLeft. */
    void navigateLeft();
/** @brief navigateRight. */
    void navigateRight();
/** @brief navigateUp. */
    void navigateUp();
/** @brief navigateDown. */
    void navigateDown();

/** @brief drawCursor. */
    void drawCursor();
/** @brief calculateTitleRight. */
    int32_t calculateTitleRight(const ListItem& item, char* valueBuffer,
                                size_t valueBufferSize) const;
/** @brief updateSelectedItemTitle. */
    int32_t updateSelectedItemTitle();
/** @brief scrollToTarget. */
    void scrollToTarget();
/** @brief updateScrollPosition. */
    void updateScrollPosition();
/** @brief startLoadAnimation. */
    void startLoadAnimation();
/** @brief shouldScroll. */
    bool shouldScroll(int newCursor);
/** @brief calculateItemY. */
    int32_t calculateItemY(int itemIndex);
    
/** @brief selectCurrent. */
    void selectCurrent();
/** @brief returnToPreviousContext. */
    void returnToPreviousContext();

/** @brief clearNonInitialAnimations. */
    void clearNonInitialAnimations();
/** @brief cancelLoadAnimations. */
    void cancelLoadAnimations();
/** @brief cancelToggleAnimation. */
    void cancelToggleAnimation();
/** @brief cancelAllOwnedAnimations. */
    void cancelAllOwnedAnimations();
/** @brief animateOwned. */
    bool animateOwned(int32_t& value, int32_t target, uint32_t duration,
                      EasingType easing,
                      PROTECTION protection = PROTECTION::NOT_PROTECTED);
/** @brief animateOwnedCallback. */
    bool animateOwnedCallback(int32_t start, int32_t target, uint32_t duration,
                              EasingType easing, ValueCallback callback,
                              PROTECTION protection = PROTECTION::NOT_PROTECTED);
    
    int32_t currentCursor = 0; // The index of the currently selected item. (signed)
};

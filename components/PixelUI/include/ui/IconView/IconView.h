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
#include "core/app/IApplication.h"
#include "widgets/label/label.h"
#include <etl/inplace_function.h>
#include <etl/string.h>
#include <etl/vector.h>
#include <etl/array.h>

// A generic interface for an icon-based item.
struct IconItem {
    // All pointers are non-owning and must outlive the IconView storing this item.
    const char* title;
    const uint8_t* bitmap;
    void* userData; // Used to store type-specific data.
    
    /**
     * @brief Creates a non-owning icon item.
     * @param t Item title, which must outlive the containing IconView.
     * @param b 24x24 bitmap, which must outlive the containing IconView.
     * @param data Optional caller data forwarded to the selection callback.
     */
    IconItem(const char* t, const uint8_t* b, void* data = nullptr) 
        : title(t), bitmap(b), userData(data) {}
};

template <size_t Capacity>
using IconItemList = etl::vector<IconItem, Capacity>;
// The callback object is owned; references captured by it are non-owning.
using SelectionCallback = etl::inplace_function<void(int index, const IconItem& item), CALLBACK_STORAGE_SIZE>;

struct IconViewLayout {
    int32_t centerX = 0;
    int32_t selectorY = 0;
    int32_t iconY = 0;
    int32_t iconSpacing = 0;
    int32_t progressY = 0;
    int32_t statusBaseline = 0;
    int32_t selectedTitleBaseline = 0;
    int32_t selectedTitleX = 0;
    int32_t selectedTitleWidth = 0;
    etl::array<int32_t, 3> slotPositionsX{};
};

// All division uses integer truncation toward zero. Inputs are non-negative
// display dimensions, so this is an explicit floor rule; odd leftovers stay on
// the right/bottom edge.
/**
 * @brief Calculates fixed icon-view coordinates for a display size.
 * @param displayWidth Non-negative display width in pixels.
 * @param displayHeight Non-negative display height in pixels.
 * @return Layout using integer division; odd remainders stay at the right or bottom.
 */
IconViewLayout calculateIconViewLayout(int32_t displayWidth, int32_t displayHeight);

namespace icon_view_detail {

template <size_t Capacity>
class IconItemStorage {
    static_assert(Capacity > 0, "IconView capacity must be positive");

protected:
    IconItemList<Capacity> items_;
};

class IconViewBase : public IApplication {
protected:
    /**
     * @brief Creates the icon-view behavior over caller-owned fixed storage.
     * @param ui UI instance used for drawing and animations.
     * @param items Item storage that must outlive this base object.
     * @param font Font used by the selected-item title.
     */
    IconViewBase(PixelUI& ui, etl::ivector<IconItem>& items,
                 const uint8_t* font);

public:
    /** @brief Cancels animations owned by the view before destruction. */
    ~IconViewBase() override;

    // --- IApplication Interface Implementation ---
    /** @brief Draws the enabled icon-view elements. */
    void draw() override;

    /**
     * @brief Handles icon navigation, selection, and exit input.
     * @param event Input event to process.
     * @return true when the event maps to an icon-view action.
     */
    bool handleInput(InputEvent event) override;

    /**
     * @brief Initializes layout and starts the entrance animations.
     * @param exitCallback Callback used to request removal of the view.
     */
    void onEnter(ExitCallback exitCallback) override;

    /** @brief Restores the active view and resumes its visual state. */
    void onResume() override;

    /** @brief Offloads the selected title and cancels owned animations. */
    void onPause() override;

    // Returns false without changing the current items when capacity is exceeded.
    // External animation registration starts in onEnter().
    /**
     * @brief Replaces the displayed items within the fixed storage capacity.
     * @param items Items to copy into this view.
     * @return false when @p items exceeds capacity; the existing items then remain unchanged.
     */
    bool setItems(const etl::ivector<IconItem>& items);

    /** @param callback Callback invoked with the selected index and item. */
    void setSelectionCallback(SelectionCallback callback);

    /**
     * @brief Sets the view title.
     * @param title Null-terminated title, or nullptr to clear it.
     */
    void setTitle(const char* title);
    
    // Control additional UI elements.
    /** @param enable Whether to draw the selection progress bar. */
    void enableProgressBar(bool enable);

    /** @param enable Whether to draw the selected and total item counts. */
    void enableStatusText(bool enable);

    /**
     * @brief Enables or disables the selected-item title widget.
     * @param enable Whether the title should be loaded and drawn.
     */
    void enableSelectedItemTitle(bool enable);

private:
    static constexpr int32_t TITLE_Y = 10;
    static constexpr int32_t ICON_WIDTH = 24;
    static constexpr int32_t ICON_HEIGHT = 24;
    static constexpr int32_t SELECTOR_LENGTH = 32;

    PixelUI& ui_;
    etl::ivector<IconItem>& items_;
    IconViewLayout layout_{};
    Label selectedItemTitle_;
    SelectionCallback selectionCallback_;
    
    // Title-related members.
    etl::string<MAX_TEXT_LENGTH> title_;
    
    // State.
    int32_t currentIndex_ = 0;
    
    // Toggles for UI elements.
    bool progressBarEnabled_ = false;
    bool statusTextEnabled_ = false;
    bool selectedItemTitleEnabled_ = false;
    bool active_ = false;

    // Animation variables.
    int32_t scrollOffset_ = 0;
    int32_t animation_selector_coord_x = 0;
    int32_t animation_selector_length = 10;
    int32_t animation_item_title_Y = 0;
    int32_t animation_pixel_dots = 0;
    int32_t animation_scroll_bar = 0;

    enum class AnimationSlot : uint8_t {
        PixelDots,
        SelectorLength,
        SelectorX,
        Scroll,
        ItemTitle,
        Progress,
        Count,
    };
    etl::array<AnimationHandle, static_cast<size_t>(AnimationSlot::Count)>
        animationHandles_{};
    
    // --- Private Methods ---
    /** @brief Recalculates layout and applies it to the selected-title widget. */
    void initializeSlotPositions();

    /** @brief Cancels every animation registered by this view. */
    void cancelOwnAnimations();

    /**
     * @brief Replaces the animation in one owned slot.
     * @return true when registration succeeds; otherwise applies the target immediately.
     */
    bool animateOwned(AnimationSlot slot, int32_t& value, int32_t target,
                      uint32_t duration, EasingType easing,
                      PROTECTION protection = PROTECTION::NOT_PROTECTED);

    /** @brief Selects the previous item, wrapping at the beginning. */
    void navigateLeft();

    /** @brief Selects the next item, wrapping at the end. */
    void navigateRight();

    /** @brief Invokes the selection callback for the current item when available. */
    void selectCurrentItem();

    /** @param newIndex Item index to select and animate into a visible slot. */
    void scrollToIndex(int newIndex);

    /** @brief Animates the progress bar to represent the current selection. */
    void updateProgressBar();

    /** @brief Copies the current item's title into the title widget. */
    void updateSelectedItemTitle();

    // Drawing logic.
    /** @brief Draws the view title centered at the top of the display. */
    void drawTitle();

    /** @brief Draws the corner selector at the supplied center and size. */
    void drawSelector(int32_t x, int32_t y, int32_t length);

    /** @brief Draws the visible portion of the horizontal icon list. */
    void drawHorizontalIconList();

    /** @brief Draws an item's bitmap or its placeholder at the supplied position. */
    void drawIcon(const IconItem& item, int32_t x, int32_t y);

    /** @brief Draws the animated progress dots and line. */
    void drawProgressBar();

    /** @brief Draws the current item index and item count. */
    void drawStatusText();

    /** @brief Draws the selected-item title at its animated vertical position. */
    void drawSelectedItemTitle();
    
    /** @return The scrolled horizontal coordinate for @p index. */
    int32_t calculateIconX(int32_t index) const;

    /** @return First item index needed to cover the left viewport edge. */
    int32_t getVisibleStartIndex() const;

    /** @return Last item index needed to cover the right viewport edge. */
    int32_t getVisibleEndIndex() const;
};

} // namespace icon_view_detail

// Capacity is selected per view while storage remains fixed and allocation-free.
template <size_t Capacity>
class IconView : private icon_view_detail::IconItemStorage<Capacity>,
                 public icon_view_detail::IconViewBase {
    using Storage = icon_view_detail::IconItemStorage<Capacity>;

public:
    /**
     * @brief Creates an allocation-free icon view with compile-time capacity.
     * @param ui UI instance used by the view.
     * @param font Font used by the selected-item title.
     */
    explicit IconView(PixelUI& ui, const uint8_t* font = PIXELUI_FONT_TEXT)
        : Storage(), icon_view_detail::IconViewBase(ui, Storage::items_, font) {}

    /** @brief Copy construction is disabled because the base references this object's storage. */
    IconView(const IconView&) = delete;

    /** @brief Copy assignment is disabled because the base references this object's storage. */
    IconView& operator=(const IconView&) = delete;

    /** @brief Move construction is disabled because the base references this object's storage. */
    IconView(IconView&&) = delete;

    /** @brief Move assignment is disabled because the base references this object's storage. */
    IconView& operator=(IconView&&) = delete;
};

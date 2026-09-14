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

#include "ui/IconView/IconView.h"
#include "core/NumericFormatter.h"
#include <etl/algorithm.h>

IconViewLayout calculateIconViewLayout(
    int32_t displayWidth, int32_t displayHeight) {
    constexpr int32_t iconWidth = 24;
    constexpr int32_t iconHeight = 24;
    IconViewLayout layout;
    layout.centerX = displayWidth / 2;
    layout.selectorY = (displayHeight - 4) / 2;
    layout.iconY = layout.selectorY - iconHeight / 2;
    const int32_t available = displayWidth - 3 * iconWidth;
    layout.iconSpacing = available > 0 ? available / 4 : 0;
    layout.progressY = displayHeight > 15 ? displayHeight - 15 : 0;
    layout.statusBaseline = displayHeight > 4 ? displayHeight - 4 : 0;
    layout.selectedTitleBaseline = displayHeight > 2 ? displayHeight - 2 : 0;
    constexpr int32_t selectedTitleMargin = 19;
    const int32_t titleMargin = displayWidth > 2 * selectedTitleMargin
        ? selectedTitleMargin
        : 0;
    layout.selectedTitleX = titleMargin;
    layout.selectedTitleWidth = displayWidth - 2 * titleMargin;
    const int32_t firstSlot =
        layout.centerX - (3 * iconWidth) / 2 - layout.iconSpacing;
    layout.slotPositionsX = {
        firstSlot,
        firstSlot + iconWidth + layout.iconSpacing,
        firstSlot + 2 * (iconWidth + layout.iconSpacing),
    };
    return layout;
}

/**
 * @brief Construct an IconView instance and initialize slot positions.
 * @param ui Reference to the PixelUI context.
 */
icon_view_detail::IconViewBase::IconViewBase(
    PixelUI& ui, etl::ivector<IconItem>& items, const uint8_t* font)
    : ui_(ui)
    , items_(items)
    , layout_(calculateIconViewLayout(
          ui.getDisplayWidth(), ui.getDisplayHeight()))
    , selectedItemTitle_(
          ui, static_cast<uint16_t>(layout_.selectedTitleX),
          static_cast<uint16_t>(layout_.selectedTitleBaseline),
          static_cast<uint16_t>(layout_.selectedTitleWidth), "", POS::BOTTOM,
          font) {
    selectedItemTitle_.setOverflow(Label::Overflow::AutoScroll);
    selectedItemTitle_.setTextAlignment(TextAlignX::Center);
    scrollOffset_ = -ui_.getDisplayWidth();
    animation_selector_coord_x = ui_.getDisplayWidth();
    animation_item_title_Y = ui_.getDisplayHeight() + 6;
}

icon_view_detail::IconViewBase::~IconViewBase() {
    cancelOwnAnimations();
}

void icon_view_detail::IconViewBase::cancelOwnAnimations() {
    for (AnimationHandle& handle : animationHandles_) {
        ui_.cancelAnimation(handle);
        handle = INVALID_ANIMATION_HANDLE;
    }
}

bool icon_view_detail::IconViewBase::animateOwned(
    AnimationSlot slot, int32_t& value, int32_t target,
    uint32_t duration, EasingType easing, PROTECTION protection) {
    AnimationHandle& handle = animationHandles_[static_cast<size_t>(slot)];
    ui_.cancelAnimation(handle);
    handle = INVALID_ANIMATION_HANDLE;
    if (!ui_.animate(value, target, duration, easing, protection, &handle)) {
        value = target;
        return false;
    }
    return true;
}

// -----------------------------------------------------------------------------
// IApplication Interface Implementation
// -----------------------------------------------------------------------------

/**
 * @brief Called when entering the IconView application.
 * @param exitCallback Callback function to trigger when exiting the view.
 */
void icon_view_detail::IconViewBase::onEnter(ExitCallback exitCallback) {
    IApplication::onEnter(exitCallback);  // Store the exit callback.
    cancelOwnAnimations();
    initializeSlotPositions();
    scrollOffset_ = -ui_.getDisplayWidth();
    active_ = true;

    // Start entry animations for visual transition.
    animateOwned(AnimationSlot::PixelDots, animation_pixel_dots,
                 (ui_.getDisplayWidth() - 2) / 2, 700,
                 EasingType::EASE_IN_OUT_CUBIC, PROTECTION::PROTECTED);
    animateOwned(AnimationSlot::SelectorLength, animation_selector_length,
                 SELECTOR_LENGTH, 700, EasingType::EASE_IN_OUT_CUBIC,
                 PROTECTION::PROTECTED);
    scrollToIndex(currentIndex_);
    if (selectedItemTitleEnabled_) selectedItemTitle_.onLoadImmediately();
    ui_.markDirty();  // Trigger initial redraw.
}

/**
 * @brief Called when the view is resumed from a paused state.
 */
void icon_view_detail::IconViewBase::onResume() {
    active_ = true;
    animation_scroll_bar = 0;
    scrollOffset_ -= ICON_WIDTH + 2 * layout_.iconSpacing;
    animateOwned(AnimationSlot::PixelDots, animation_pixel_dots,
                 (ui_.getDisplayWidth() - 2) / 2, 300,
                 EasingType::EASE_IN_OUT_CUBIC, PROTECTION::PROTECTED);
    updateProgressBar();
    scrollToIndex(currentIndex_);
    if (selectedItemTitleEnabled_) selectedItemTitle_.onLoadImmediately();
    ui_.markDirty();
}

/**
 * @brief Called when the view is paused.
 */
void icon_view_detail::IconViewBase::onPause() {
    if (selectedItemTitleEnabled_) selectedItemTitle_.onOffload();
    active_ = false;
    ui_.markFading();
    cancelOwnAnimations();
    animation_selector_length = SELECTOR_LENGTH;
}

/**
 * @brief Handle input events for navigation and selection.
 * @param event The input event type (LEFT, RIGHT, SELECT, BACK, etc.).
 * @return true if the event was handled, false otherwise.
 */
bool icon_view_detail::IconViewBase::handleInput(InputEvent event) {
    switch (event) {
        case ICONVIEW_NAVI_LEFT:   navigateLeft(); return true;
        case ICONVIEW_NAVI_RIGHT:  navigateRight(); return true;
        case ICONVIEW_NAVI_SELECT: selectCurrentItem(); return true;
        case ICONVIEW_NAVI_BACK:   requestExit(); return true; // Trigger exit callback.
        default: return false;
    }
}

/**
 * @brief Render all elements of the IconView UI.
 */
void icon_view_detail::IconViewBase::draw() {
    if (!title_.empty()) drawTitle();
    drawSelector(
        animation_selector_coord_x, layout_.selectorY,
        animation_selector_length);
    drawHorizontalIconList();
    if (progressBarEnabled_) drawProgressBar();
    if (statusTextEnabled_) drawStatusText();
    if (selectedItemTitleEnabled_) drawSelectedItemTitle();
}

// -----------------------------------------------------------------------------
// Configuration Methods
// -----------------------------------------------------------------------------

/**
 * @brief Set the list of icons to be displayed.
 * @param items A vector of IconItem structures.
 */
bool icon_view_detail::IconViewBase::setItems(
    const etl::ivector<IconItem>& items) {
    if (items.size() > items_.max_size()) return false;
    items_.assign(items.begin(), items.end());
    currentIndex_ = 0;
    if (selectedItemTitleEnabled_) updateSelectedItemTitle();
    return true;
}

/**
 * @brief Set callback to be triggered when an icon is selected.
 * @param callback The function to call on selection.
 */
void icon_view_detail::IconViewBase::setSelectionCallback(SelectionCallback callback) {
    selectionCallback_ = callback;
}

/**
 * @brief Set the title text displayed at the top of the view.
 * @param title Pointer to a C-string title (can be nullptr).
 */
void icon_view_detail::IconViewBase::setTitle(const char* title) {
    title_ = title ? title : "";
}

void icon_view_detail::IconViewBase::enableProgressBar(bool enable) { progressBarEnabled_ = enable; }
void icon_view_detail::IconViewBase::enableStatusText(bool enable) { statusTextEnabled_ = enable; }
void icon_view_detail::IconViewBase::enableSelectedItemTitle(bool enable) {
    if (selectedItemTitleEnabled_ == enable) return;
    selectedItemTitleEnabled_ = enable;
    if (enable) {
        updateSelectedItemTitle();
        if (active_) selectedItemTitle_.onLoadImmediately();
    } else {
        selectedItemTitle_.onOffload();
    }
    ui_.markDirty();
}

// -----------------------------------------------------------------------------
// Navigation and Interaction
// -----------------------------------------------------------------------------

/**
 * @brief Navigate to the previous icon in the list.
 */
void icon_view_detail::IconViewBase::navigateLeft() {
    if (items_.empty()) return;
    currentIndex_ = (currentIndex_ - 1 + items_.size()) % items_.size();
    scrollToIndex(currentIndex_);
}

/**
 * @brief Navigate to the next icon in the list.
 */
void icon_view_detail::IconViewBase::navigateRight() {
    if (items_.empty()) return;
    currentIndex_ = (currentIndex_ + 1) % items_.size();
    scrollToIndex(currentIndex_);
}

/**
 * @brief Select the currently focused icon and trigger the callback.
 */
void icon_view_detail::IconViewBase::selectCurrentItem() {
    if (selectionCallback_ && !items_.empty()) {
        selectionCallback_(currentIndex_, items_[currentIndex_]);
    }
}

/**
 * @brief Smoothly scroll the view to center the given icon index.
 * @param newIndex The index of the icon to scroll to.
 * @note This method triggers selector and scroll animations.
 */
void icon_view_detail::IconViewBase::scrollToIndex(int newIndex) {
    const int32_t totalItems = static_cast<int32_t>(items_.size());
    if (totalItems == 0) return;

    // Determine target slot position for current index.
    int32_t targetSlot;
    if (newIndex == 0 && totalItems > 1) targetSlot = 0;
    else if (newIndex == totalItems - 1 && totalItems > 1) targetSlot = 2;
    else targetSlot = 1;
    
    // Compute target scroll offset.
    const int32_t targetSelectorX =
        layout_.slotPositionsX[targetSlot] + ICON_WIDTH / 2;
    const int32_t iconTargetCenterX = targetSelectorX;
    const int32_t iconOriginalCenterX =
        newIndex * (ICON_WIDTH + layout_.iconSpacing) + ICON_WIDTH / 2;
    const int32_t targetScrollOffset =
        iconTargetCenterX - iconOriginalCenterX;

    // Animate selector and scroll transitions.
    animateOwned(AnimationSlot::SelectorX, animation_selector_coord_x,
                 targetSelectorX, 550, EasingType::EASE_OUT_CUBIC);
    animateOwned(AnimationSlot::Scroll, scrollOffset_, targetScrollOffset,
                 350, EasingType::EASE_OUT_CUBIC);
    
    // Animate selected item title rise effect.
    if (selectedItemTitleEnabled_) {
        animation_item_title_Y = ui_.getDisplayHeight() + 6;
        animateOwned(AnimationSlot::ItemTitle, animation_item_title_Y,
                     layout_.selectedTitleBaseline, 300,
                     EasingType::EASE_OUT_CUBIC);
    }
    
    currentIndex_ = newIndex;
    if (selectedItemTitleEnabled_) updateSelectedItemTitle();
    updateProgressBar();
    ui_.markDirty();  // Request redraw after animation update.
}

/**
 * @brief Update the progress bar animation according to current selection.
 */
void icon_view_detail::IconViewBase::updateProgressBar() {
    if (progressBarEnabled_ && !items_.empty()) {
        const int32_t target = static_cast<int32_t>(
            (static_cast<int64_t>(currentIndex_ + 1) * ui_.getDisplayWidth()) /
            static_cast<int32_t>(items_.size()));
        animateOwned(AnimationSlot::Progress, animation_scroll_bar, target,
                     300, EasingType::EASE_OUT_QUAD);
    }
}

void icon_view_detail::IconViewBase::updateSelectedItemTitle() {
    const char* title = items_.empty() || items_[currentIndex_].title == nullptr
        ? ""
        : items_[currentIndex_].title;
    selectedItemTitle_.setText(title);
}

// -----------------------------------------------------------------------------
// Drawing Logic
// -----------------------------------------------------------------------------

/**
 * @brief Draw the title text centered at the top of the display.
 */
void icon_view_detail::IconViewBase::drawTitle() {
    Canvas& display = ui_.getCanvas();
    display.setFont(PIXELUI_FONT_SMALL);
    int titleWidth = display.getStrWidth(title_.c_str());
    display.drawStr((ui_.getDisplayWidth() - titleWidth) / 2, TITLE_Y, title_.c_str());
}

/**
 * @brief Draw the animated progress bar at the bottom.
 */
void icon_view_detail::IconViewBase::drawProgressBar() {
    Canvas& display = ui_.getCanvas();
    for (int i = 0; i <= static_cast<int>(animation_pixel_dots); i++) {
        display.drawPixel(i * 2, layout_.progressY);
    }
    display.drawHLine(0, layout_.progressY, animation_scroll_bar);
}

/**
 * @brief Draw current item index and total count as status text.
 */
void icon_view_detail::IconViewBase::drawStatusText() {
    if (items_.empty()) return;
    Canvas& display = ui_.getCanvas();
    char statusText[16]{};
    FixedBufferWriter writer(statusText, sizeof(statusText));
    if (!writer.appendInteger(currentIndex_ + 1) || !writer.append("/") ||
        !writer.appendInteger(static_cast<int32_t>(items_.size())) ||
        !writer.finish()) return;
    display.setFont(PIXELUI_FONT_TINY);
    display.drawStr(2, layout_.statusBaseline, statusText);
}

/**
 * @brief Draw the title of the currently selected item below the icons.
 */
void icon_view_detail::IconViewBase::drawSelectedItemTitle() {
    if (items_.empty()) return;
    const WidgetRenderContext context{
        0,
        animation_item_title_Y - layout_.selectedTitleBaseline,
        {0, 0, ui_.getDisplayWidth(), ui_.getDisplayHeight()}
    };
    selectedItemTitle_.draw(context);
}

/**
 * @brief Draw visible icons in a horizontal scroll layout.
 */
void icon_view_detail::IconViewBase::drawHorizontalIconList() {
    if (items_.empty()) {
        // Show a fallback message if no icons exist.
        Canvas& display = ui_.getCanvas();
        display.setFont(PIXELUI_FONT_TINY);
        display.drawStr(layout_.centerX - 20, layout_.iconY + 16, "No Items");
        return;
    }
    
    // Determine visible range based on current scroll position.
    const int32_t startIndex = getVisibleStartIndex();
    const int32_t endIndex = getVisibleEndIndex();

    for (int32_t i = startIndex;
         i <= endIndex && i < static_cast<int32_t>(items_.size()); ++i) {
        const int32_t iconX = calculateIconX(i);
        drawIcon(items_[i], iconX, layout_.iconY);
    }
}

/**
 * @brief Draw a single icon (bitmap or fallback box).
 * @param item Icon item data containing bitmap pointer and metadata.
 * @param x X coordinate of icon.
 * @param y Y coordinate of icon.
 */
void icon_view_detail::IconViewBase::drawIcon(const IconItem& item, int32_t x, int32_t y) {
    Canvas& display = ui_.getCanvas();
    if (item.bitmap) {
        // Center 24x24 bitmap within icon area.
        int iconX = x + (ICON_WIDTH - 24) / 2;
        int iconY = y + (ICON_HEIGHT - 24) / 2;
        display.drawXBM(iconX, iconY, 24, 24, item.bitmap);
    } else {
        // Draw placeholder rounded box.
        display.drawRBox(x + 4, y + 4, ICON_WIDTH - 8, ICON_HEIGHT - 8, 2);
    }
}

/**
 * @brief Precompute X-coordinates for icon slots based on display width.
 */
void icon_view_detail::IconViewBase::initializeSlotPositions() {
    layout_ = calculateIconViewLayout(
        ui_.getDisplayWidth(), ui_.getDisplayHeight());
    selectedItemTitle_.setPosition(
        static_cast<uint16_t>(layout_.selectedTitleX),
        static_cast<uint16_t>(layout_.selectedTitleBaseline));
    selectedItemTitle_.setViewportWidth(
        static_cast<uint16_t>(layout_.selectedTitleWidth));
}

/**
 * @brief Draw a selector frame around the focused icon.
 * @param x X coordinate of selector center.
 * @param y Y coordinate of selector center.
 * @param length Total side length of the selector square.
 */
void icon_view_detail::IconViewBase::drawSelector(int32_t x, int32_t y, int32_t length) {
    Canvas& display = ui_.getCanvas();
    const int32_t half_length = length / 2;

    // Draw corner-style selector lines.
    display.drawLine(x - half_length + 1, y - half_length, x - half_length + 5, y - half_length);
    display.drawLine(x - half_length, y + 1 - half_length, x - half_length, y + 5 - half_length);
    display.drawLine(x - 1 + half_length, y - half_length, x - 5 + half_length, y - half_length);
    display.drawLine(x + half_length, y + 1 - half_length, x + half_length, y + 5 - half_length);
    display.drawLine(x + 1 - half_length, y - 1 + half_length, x + 5 - half_length, y - 1 + half_length);
    display.drawLine(x - half_length, y - 2 + half_length, x - half_length, y - 6 + half_length);
    display.drawLine(x - 1 + half_length, y - 6 + half_length, x - 1 + half_length, y - 2 + half_length);
    display.drawLine(x - 2 + half_length, y - 1 + half_length, x - 6 + half_length, y - 1 + half_length);
}

/**
 * @brief Calculate icon X position based on index and scroll offset.
 * @param index Index of the icon.
 * @return Computed X coordinate.
 */
int32_t icon_view_detail::IconViewBase::calculateIconX(int32_t index) const {
    return (index * (ICON_WIDTH + layout_.iconSpacing)) + scrollOffset_;
}

/**
 * @brief Determine first visible icon index based on scroll offset.
 * @return Index of the first visible icon.
 */
int32_t icon_view_detail::IconViewBase::getVisibleStartIndex() const {
    const int32_t leftmostX = -ICON_WIDTH;
    for (int32_t i = 0; i < static_cast<int32_t>(items_.size()); ++i) {
        if (calculateIconX(i) >= leftmostX) return etl::max(0, i - 1);
    }
    return 0;
}

/**
 * @brief Determine last visible icon index based on scroll offset.
 * @return Index of the last visible icon.
 */
int32_t icon_view_detail::IconViewBase::getVisibleEndIndex() const {
    const int32_t rightmostX = ui_.getDisplayWidth() + ICON_WIDTH;
    for (int32_t i = static_cast<int32_t>(items_.size()) - 1; i >= 0; --i) {
        if (calculateIconX(i) <= rightmostX) {
            return etl::min(static_cast<int32_t>(items_.size()) - 1, i + 1);
        }
    }
    return static_cast<int32_t>(items_.size()) - 1;
}

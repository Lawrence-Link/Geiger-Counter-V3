/*
 * Copyright (C) 2025 Lawrence Link
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ui/IconView/IconView.h"
#include <algorithm>

IconView::IconView(PixelUI& ui) : ui_(ui) {
    initializeSlotPositions();
}

// --- IApplication Interface Implementation ---

void IconView::onEnter(ExitCallback exitCallback) {
    IApplication::onEnter(exitCallback); // Store the exit callback.
    ui_.animate(animation_pixel_dots, 63, 700, EasingType::EASE_IN_OUT_CUBIC, PROTECTION::PROTECTED);
    ui_.animate(animation_selector_length, selector_length, 700, EasingType::EASE_IN_OUT_CUBIC, PROTECTION::PROTECTED);
    scrollOffset_ = -128;
    updateProgressBar();
    ui_.markDirty();
}

void IconView::onResume() {
    animation_scroll_bar = 0;
    scrollOffset_ -= 50;
    ui_.animate(animation_pixel_dots, 63, 300, EasingType::EASE_IN_OUT_CUBIC);
    updateProgressBar();
    scrollToIndex(currentIndex_);
    ui_.markDirty();
}

void IconView::onPause() {
    ui_.markFading();
}

bool IconView::handleInput(InputEvent event) {
    switch (event) {
        case InputEvent::LEFT: navigateLeft(); return true;
        case InputEvent::RIGHT: navigateRight(); return true;
        case InputEvent::SELECT: selectCurrentItem(); return true;
        case InputEvent::BACK: requestExit(); return true; // Use IApplication's exit mechanism.
        default: return false;
    }
}

void IconView::draw() {
    if (!title_.empty()) drawTitle();
    drawSelector(animation_selector_coord_x, 30, animation_selector_length);
    drawHorizontalIconList();
    if (progressBarEnabled_) drawProgressBar();
    if (statusTextEnabled_) drawStatusText();
    if (selectedItemTitleEnabled_) drawSelectedItemTitle();
}

// --- Configuration Methods ---

void IconView::setItems(const std::vector<IconItem>& items) {
    items_ = items;
    currentIndex_ = 0;
    scrollToIndex(0);
}

void IconView::setSelectionCallback(SelectionCallback callback) {
    selectionCallback_ = callback;
}

void IconView::setTitle(const char* title) {
    title_ = title ? title : "";
}

void IconView::enableProgressBar(bool enable) { progressBarEnabled_ = enable; }
void IconView::enableStatusText(bool enable) { statusTextEnabled_ = enable; }
void IconView::enableSelectedItemTitle(bool enable) { selectedItemTitleEnabled_ = enable; }

// --- Navigation and Interaction ---

void IconView::navigateLeft() {
    if (items_.empty()) return;
    currentIndex_ = (currentIndex_ - 1 + items_.size()) % items_.size();
    scrollToIndex(currentIndex_);
}

void IconView::navigateRight() {
    if (items_.empty()) return;
    currentIndex_ = (currentIndex_ + 1) % items_.size();
    scrollToIndex(currentIndex_);
}

void IconView::selectCurrentItem() {
    if (selectionCallback_ && !items_.empty()) {
        selectionCallback_(currentIndex_, items_[currentIndex_]);
    }
}

void IconView::scrollToIndex(int newIndex) {
    int totalItems = items_.size();
    if (totalItems == 0) return;

    ui_.getAnimationManPtr()->clearUnprotected();

    int targetSlot;
    if (newIndex == 0 && totalItems > 1) {
        targetSlot = 0;
    } else if (newIndex == totalItems - 1 && totalItems > 1) {
        targetSlot = 2;
    } else {
        targetSlot = 1;
    }
    
    float targetSelectorX = slotPositionsX_[targetSlot] + 0.5 * iconWidth_;
    float iconTargetCenterX = slotPositionsX_[targetSlot] + iconWidth_ / 2.0f;
    float iconOriginalCenterX = newIndex * (iconWidth_ + iconSpacing_) + iconWidth_ / 2.0f;
    float targetScrollOffset = iconTargetCenterX - iconOriginalCenterX;

    ui_.animate(animation_selector_coord_x, targetSelectorX, 550, EasingType::EASE_OUT_CUBIC);
    ui_.animate(scrollOffset_, targetScrollOffset, 350, EasingType::EASE_OUT_CUBIC);
    
    if (selectedItemTitleEnabled_) {
        animation_item_title_Y = 70;
        ui_.animate(animation_item_title_Y, 62, 300, EasingType::EASE_OUT_CUBIC);
    }
    
    currentIndex_ = newIndex;
    updateProgressBar();
    ui_.markDirty(); 
}

void IconView::updateProgressBar() {
    if (progressBarEnabled_ && !items_.empty()) {
        float progress = (static_cast<float>(currentIndex_ + 1)) / items_.size();
        ui_.animate(animation_scroll_bar, progress * ui_.getU8G2().getWidth(), 300, EasingType::EASE_OUT_QUAD);
    }
}

// --- Drawing Logic ---

void IconView::drawTitle() {
    U8G2& display = ui_.getU8G2();
    display.setFont(u8g2_font_5x7_mf);
    int titleWidth = display.getStrWidth(title_.c_str());
    display.drawStr((display.getWidth() - titleWidth) / 2, titleY_, title_.c_str());
}

void IconView::drawProgressBar() {
    U8G2& display = ui_.getU8G2();
    for (int i = 0; i <= static_cast<int>(animation_pixel_dots); i++) {
        display.drawPixel(i * 2, 49);
    }
    display.drawHLine(0, 49, animation_scroll_bar);
}

void IconView::drawStatusText() {
    if (items_.empty()) return;
    U8G2& display = ui_.getU8G2();
    char statusText[32];
    snprintf(statusText, sizeof(statusText), "%d/%d", currentIndex_ + 1, (int)items_.size());
    display.setFont(u8g2_font_tom_thumb_4x6_mf);
    display.drawStr(2, 60, statusText);
}

void IconView::drawSelectedItemTitle() {
    if (items_.empty()) return;
    U8G2& display = ui_.getU8G2();
    const auto& currentItem = items_[currentIndex_];
    display.setFont(u8g2_font_wqy12_t_gb2312);
    int titleWidth = display.getUTF8Width(currentItem.title);
    display.drawUTF8((display.getWidth() - titleWidth) / 2, animation_item_title_Y, currentItem.title);
}

void IconView::drawHorizontalIconList() {
    if (items_.empty()) {
        U8G2& display = ui_.getU8G2();
        display.setFont(u8g2_font_tom_thumb_4x6_mf);
        display.drawStr(centerX_ - 20, iconY_ + 16, "No Items");
        return;
    }
    
    int startIndex = getVisibleStartIndex();
    int endIndex = getVisibleEndIndex();

    for (int i = startIndex; i <= endIndex && i < static_cast<int>(items_.size()); ++i) {
        int iconX = calculateIconX(i);
        drawIcon(items_[i], iconX, iconY_);
    }
}

void IconView::drawIcon(const IconItem& item, int x, int y) {
    U8G2& display = ui_.getU8G2();
    if (item.bitmap) {
        int iconX = x + (iconWidth_ - 24) / 2;
        int iconY = y + (iconHeight_ - 24) / 2;
        display.drawXBM(iconX, iconY, 24, 24, item.bitmap);
    } else {
        display.drawRBox(x + 4, y + 4, iconWidth_ - 8, iconHeight_ - 8, 2);
    }
}

void IconView::initializeSlotPositions() {
    iconSpacing_ = (ui_.getU8G2().getWidth() - 3 * iconWidth_) * 0.25f;
    float firstSlotX = centerX_ - 1.5f * iconWidth_ - iconSpacing_;
    slotPositionsX_.clear();
    slotPositionsX_.push_back(firstSlotX);
    slotPositionsX_.push_back(firstSlotX + iconWidth_ + iconSpacing_);
    slotPositionsX_.push_back(firstSlotX + iconWidth_ * 2 + iconSpacing_ * 2);
}

void IconView::drawSelector(uint32_t x, uint32_t y, uint32_t length) {
    U8G2& display = ui_.getU8G2();
    int half_length = 0.5 * length;
    display.drawLine(x - half_length + 1, y - half_length, x - half_length + 5, y - half_length);
    display.drawLine(x - half_length, y + 1 - half_length, x - half_length, y + 5 - half_length);
    display.drawLine(x - 1 + half_length, y - half_length, x - 5 + half_length, y - half_length);
    display.drawLine(x + half_length, y + 1 - half_length, x + half_length, y + 5 - half_length);
    display.drawLine(x + 1 - half_length, y - 1 + half_length, x + 5 - half_length, y - 1 + half_length);
    display.drawLine(x - half_length, y - 2 + half_length, x - half_length, y - 6 + half_length);
    display.drawLine(x - 1 + half_length, y - 6 + half_length, x - 1 + half_length, y - 2 + half_length);
    display.drawLine(x - 2 + half_length, y - 1 + half_length, x - 6 + half_length, y - 1 + half_length);
}

int IconView::calculateIconX(int index) {
    return (index * (iconWidth_ + iconSpacing_)) + scrollOffset_;
}

int IconView::getVisibleStartIndex() {
    int leftmostX = -iconWidth_;
    for (int i = 0; i < static_cast<int>(items_.size()); ++i) {
        if (calculateIconX(i) >= leftmostX) { return std::max(0, i - 1); }
    }
    return 0;
}

int IconView::getVisibleEndIndex() {
    int rightmostX = 128 + iconWidth_;
    for (int i = static_cast<int>(items_.size()) - 1; i >= 0; --i) {
        if (calculateIconX(i) <= rightmostX) { return std::min(static_cast<int>(items_.size()) - 1, i + 1); }
    }
    return static_cast<int>(items_.size()) - 1;
}
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

#pragma once

#include "PixelUI.h"
#include "core/app/IApplication.h"
#include <functional>
#include <vector>
#include <string>

// A generic interface for an icon-based item.
struct IconItem {
    const char* title;
    const uint8_t* bitmap;
    void* userData; // Used to store type-specific data.
    
    IconItem(const char* t, const uint8_t* b, void* data = nullptr) 
        : title(t), bitmap(b), userData(data) {}
};

// Callback function type definitions.
using SelectionCallback = std::function<void(int index, const IconItem& item)>;

// IconView is now a complete, standalone view component.
class IconView : public IApplication {
public:
    IconView(PixelUI& ui);
    ~IconView() = default;

    // --- IApplication Interface Implementation ---
    void draw() override;
    bool handleInput(InputEvent event) override;
    void onEnter(ExitCallback exitCallback) override;
    void onResume() override;
    void onPause() override;

    // --- Configuration Methods ---
    void setItems(const std::vector<IconItem>& items);
    void setSelectionCallback(SelectionCallback callback);
    void setTitle(const char* title);
    
    // Control additional UI elements.
    void enableProgressBar(bool enable);
    void enableStatusText(bool enable);
    void enableSelectedItemTitle(bool enable);

private:
    PixelUI& ui_;
    std::vector<IconItem> items_;
    SelectionCallback selectionCallback_;
    
    // Title-related members.
    std::string title_;
    int titleY_ = 10;
    
    // State.
    int currentIndex_ = 0;
    
    // Toggles for UI elements.
    bool progressBarEnabled_ = false;
    bool statusTextEnabled_ = false;
    bool selectedItemTitleEnabled_ = false;

    // Animation variables.
    int32_t scrollOffset_ = -128;
    int32_t animation_selector_coord_x = 128;
    int32_t animation_selector_length = 10;
    int32_t selector_length = 30;
    int32_t animation_item_title_Y = 70;
    int32_t animation_pixel_dots = 0;
    int32_t animation_scroll_bar = 0;
    
    // Layout parameters.
    int iconWidth_ = 24;
    int iconHeight_ = 24;
    int iconSpacing_ = 14;
    int centerX_ = 64;
    int iconY_ = 18;
    
    std::vector<float> slotPositionsX_;
    
    // --- Private Methods ---
    void initializeSlotPositions();
    void navigateLeft();
    void navigateRight();
    void selectCurrentItem();
    void scrollToIndex(int newIndex);
    void updateProgressBar();

    // Drawing logic.
    void drawTitle();
    void drawSelector(uint32_t x, uint32_t y, uint32_t length);
    void drawHorizontalIconList();
    void drawIcon(const IconItem& item, int x, int y);
    void drawProgressBar();
    void drawStatusText();
    void drawSelectedItemTitle();
    
    int calculateIconX(int index);
    int getVisibleStartIndex();
    int getVisibleEndIndex();
};
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
#include <algorithm>
#include <cstring>
#include "etl/vector.h"
#include "etl/delegate.h"
#include "core/animation/animation.h"
#include "core/app/IApplication.h"

// Struct to hold extra data for a list item, like values for switches or sliders.
struct ListItemExtra{
    bool* switchValue = nullptr; // Pointer to a boolean value for a switch.
    int32_t* intValue = nullptr;     // Pointer to an integer value for a slider or counter.
};

// Represents a single item in a list view.
struct ListItem{
    mutable char Title[MAX_LISTITEM_NAME_NUM]; // The display title of the item. 'mutable' allows it to be changed even if the struct is 'const'.
    ListItem * nextList;                       // Pointer to a sub-menu (another list).
    size_t nextListLength;                     // The number of items in the sub-menu.
    std::function<void()> pFunc;               // A function to execute when the item is selected.
    ListItemExtra extra;                       // Extra data for dynamic UI elements.
private:
    // Animation values for visual effects on individual items.
    int32_t anim_val1 = 0;
    int32_t anim_val2 = 0;
public:
    // Constructor to initialize the list item.
    ListItem(const char* title,
             ListItem* next = nullptr,
             size_t nextLen = 0,
             std::function<void()> func = nullptr,
             ListItemExtra ex = {})
        : nextList(next), nextListLength(nextLen), pFunc(func), extra(ex)
    {
        strncpy(Title, title, sizeof(Title));
        Title[sizeof(Title)-1] = 0; // Ensure null-termination.
    }
    // int32_t getVal1 () const {return anim_val1;}
    // int32_t getVal2 () const {return anim_val2;}
    // void updateExtra() {}
};

// The main class for handling a list-based user interface.
class ListView : public IApplication {
public:
    // Constructor to initialize the list view with a UI handler and a list of items.
    ListView(PixelUI& ui, ListItem *itemList, size_t length) : m_ui(ui), m_itemList(itemList), m_itemLength(length - 1) {}
    ~ListView() = default;

    // --- Application Lifecycle and Input Handlers ---
    void draw() override;
    bool handleInput(InputEvent event) override;
    void onEnter(ExitCallback exitCallback) override;
    void onResume() override ;
    void onPause() override;
    void onExit() override;

    virtual void onSave() = 0;

    // --- Public Utility Methods ---
    void resizeLength(size_t itemLength) { m_itemLength = itemLength; }
    PixelUI& getUI() { return m_ui; }
    
    PixelUI& m_ui; // Reference to the main UI class.
private:
    ListItem* m_itemList;
    size_t m_itemLength;
    
    // --- Layout and Spacing Variables ---
    uint8_t spacing_ = 7;
    uint8_t topMargin_ = 3;
    uint8_t FontHeight = 0;
    
    // int8_t item_offset_y = 0;

    // History stack to support nested menus (for back navigation).
    etl::vector<etl::pair<etl::pair<ListItem*, size_t>, size_t>, MAX_LISTVIEW_DEPTH> m_history_stack;

    // --- Cursor Variables ---
    int32_t CursorY = -6;
    int32_t CursorX = 1;
    int32_t CursorWidth = 0;
    
    // --- Scroll Variables ---
    int32_t scrollOffset_ = 0;          // The current vertical scroll position.
    int topVisibleIndex_ = 0;           // Index of the first item visible on screen.
    int visibleItemCount_ = LISTVIEW_ITEMS_PER_PAGE; // Number of items that can be displayed at once.
    
    // --- Load Animation Variables ---
    int32_t itemLoadAnimations_[LISTVIEW_ITEMS_PER_PAGE + 1]; // Tracks animation progress for each item.
    bool isInitialLoad_ = true;
    int32_t animation_pixel_dots = 0;
    int32_t animation_scroll_bar = 0;
    
    // --- Transition Animation Variables ---
    bool isTransitioning_ = false;
    int32_t transitionProgress_ = 0;
    int selectedItemForTransition_ = -1;
    int32_t itemExitAnimations_[LISTVIEW_ITEMS_PER_PAGE + 1]; // Animations for items leaving the screen.
    int32_t itemEnterAnimations_[LISTVIEW_ITEMS_PER_PAGE + 1]; // Animations for items entering the screen.
    int32_t selectedItemY_ = 0;
    ListItem* oldItemList_ = nullptr;
    size_t oldItemLength_ = 0;
    int oldTopVisibleIndex_ = 0;
    int32_t switchBoxX = 0;

    // --- Progress Bar Variables ---
    int32_t progress_bar_top = 0;
    int32_t progress_bar_bottom = 0;

    // --- Navigation and Drawing Methods ---
    void navigateLeft();
    void navigateRight();
    void navigateUp();
    void navigateDown();

    void drawCursor();
    void scrollToTarget(size_t target);
    void updateScrollPosition();
    void startLoadAnimation();
    void startTransitionAnimation(int selectedItemIndex);
    int getVisibleItemIndex(int screenIndex);
    bool shouldScroll(int newCursor);
    int32_t calculateItemY(int itemIndex);
    
    void selectCurrent();
    void returnToPreviousContext();

    void clearNonInitialAnimations();
    
    size_t currentCursor = 0; // The index of the currently selected item.
};
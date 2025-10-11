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

// Maximum number of concurrent animations allowed in the system
constexpr int MAX_ANIMATION_COUNT = 35; 

// Maximum length of text strings for widgets that contains text
constexpr int MAX_TEXT_LENGTH = 30;

// Maximum number of apps that can be registered
constexpr int MAX_APP_NUM = 10;

// Maximum slots in a single AppView
constexpr int MAX_APPVIEW_SLOT_NUM = 10;

// Maximum slots in a ListView
constexpr int MAX_LISTVIEW_SLOT_NUM = 30;

// Maximum number of characters in a ListItem name
constexpr int MAX_LISTITEM_NAME_NUM = 30;

// Number of items shown per page in a ListView
constexpr int LISTVIEW_ITEMS_PER_PAGE = 4;

// Maximum depth of nested ListViews
constexpr int MAX_LISTVIEW_DEPTH = 6;

// Stack size for callback animations (for internal coroutine/animation safety)
constexpr int CALLBACK_ANIMATION_STACK_SIZE = 2;

// Maximum number of popups that can appear simultaneously
constexpr int MAX_POPUP_NUM = 3;

// Maximum number of widgets displayed on screen at the same time
constexpr int MAX_ONSCREEN_WIDGET_NUM = 6;

// Maximum width (number of digits) for fixed-width integer display
constexpr int MAX_INT_FIXED_WIDTH = 6;

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

// Maximum of concurrent animation going on.
constexpr int MAX_ANIMATION_COUNT = 35; 
constexpr int MAX_TEXT_LENGTH = 30;

// Maximum item that can be iterated during initialization.
constexpr int MAX_APP_NUM = 10;
constexpr int MAX_APPVIEW_SLOT_NUM = 10;

constexpr int MAX_LISTVIEW_SLOT_NUM = 30;
constexpr int MAX_LISTITEM_NAME_NUM = 30;
constexpr int LISTVIEW_ITEMS_PER_PAGE = 4;
constexpr int MAX_LISTVIEW_DEPTH = 6;

constexpr int CALLBACK_ANIMATION_STACK_SIZE = 2;
constexpr int MAX_POPUP_NUM = 3;
constexpr int MAX_ONSCREEN_WIDGET_NUM = 6;
constexpr int MAX_INT_FIXED_WIDTH = 6;
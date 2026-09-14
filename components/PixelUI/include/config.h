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

/* Define PIXELUI_CONFIG_PATH globally to use a complete external config file. */
#ifdef PIXELUI_CONFIG_PATH

#include PIXELUI_CONFIG_PATH

#else

/* Feature modules. Keep these values identical in the library and firmware. */
#define PIXELUI_USE_ANIMATION           1
#define PIXELUI_USE_POPUP               1
#define PIXELUI_USE_COROUTINE           1
#define PIXELUI_USE_FOCUS               1
#define PIXELUI_USE_LABEL_SCROLL        1

/* Power policy. 0 keeps the host's periodic timer; 1 allows idle tick suppression. */
#ifndef PIXELUI_ENABLE_TICKLESS
#define PIXELUI_ENABLE_TICKLESS          0
#endif

/* Popup types. PopupValueDigits and PopupKeyboard also require Focus. */
#define PIXELUI_USE_POPUP_INFO          1
#define PIXELUI_USE_POPUP_PROGRESS      1
#define PIXELUI_USE_POPUP_VALUE_DIGITS  1
#define PIXELUI_USE_POPUP_KEYBOARD      1

/* Font roles. Replace the WQY fonts when full Chinese coverage is unnecessary. */
#define PIXELUI_FONT_TEXT       u8g2_font_wqy12_t_gb2312
#define PIXELUI_FONT_SMALL      u8g2_font_5x7_mf
#define PIXELUI_FONT_TINY       u8g2_font_4x6_tr
#define PIXELUI_FONT_NUMERIC    u8g2_font_tenfatguys_tn
#define PIXELUI_FONT_CHART      PIXELUI_FONT_TINY      

/* Fixed-capacity storage. All memory is reserved statically. */
#define PIXELUI_MAX_ANIMATION_COUNT             35
#define PIXELUI_MAX_TEXT_LENGTH                 30
#define PIXELUI_MAX_APP_NUM                     11
#define PIXELUI_MAX_VIEW_DEPTH                   8
#define PIXELUI_APPLICATION_ARENA_SIZE        4096
#define PIXELUI_APPLICATION_ARENA_MAX_ALIGNMENT 64
#define PIXELUI_CALLBACK_STORAGE_SIZE           32
#define PIXELUI_MAX_APPVIEW_SLOT_NUM            10
#define PIXELUI_MAX_LISTITEM_NAME_NUM           30
#define PIXELUI_LISTVIEW_ITEMS_PER_PAGE          4
#define PIXELUI_MAX_LISTVIEW_DEPTH               6
#define PIXELUI_CALLBACK_ANIMATION_STACK_SIZE     2
#define PIXELUI_MAX_POPUP_NUM                     3
#define PIXELUI_MAX_ONSCREEN_WIDGET_NUM           6
#define PIXELUI_MAX_INT_FIXED_WIDTH               6
#define PIXELUI_MAX_COROUTINE_NUM                10

/* Input mapping. */
#define FOCUS_MANAGER_NAVI_PREV    InputEvent::LEFT
#define FOCUS_MANAGER_NAVI_NEXT    InputEvent::RIGHT
#define FOCUS_MANAGER_NAVI_SELECT  InputEvent::SELECT

#define LISTVIEW_NAVI_UP            InputEvent::LEFT
#define LISTVIEW_NAVI_DOWN          InputEvent::RIGHT
#define LISTVIEW_NAVI_SELECT        InputEvent::SELECT
#define LISTVIEW_NAVI_BACK          InputEvent::BACK

#define ICONVIEW_NAVI_LEFT          InputEvent::LEFT
#define ICONVIEW_NAVI_RIGHT         InputEvent::RIGHT
#define ICONVIEW_NAVI_SELECT        InputEvent::SELECT
#define ICONVIEW_NAVI_BACK          InputEvent::BACK

#define NUMSCROLL_NAVI_UP           InputEvent::RIGHT
#define NUMSCROLL_NAVI_DOWN         InputEvent::LEFT
#define NUMSCROLL_NAVI_SELECT       InputEvent::SELECT

#endif

#include "core/config_internal.h"

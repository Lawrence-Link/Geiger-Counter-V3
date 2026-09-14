/*
 * Copyright (C) 2025 Lawrence Link
 */

#pragma once

#include <stddef.h>

#ifndef PIXELUI_ENABLE_TICKLESS
#define PIXELUI_ENABLE_TICKLESS 0
#endif

#if PIXELUI_ENABLE_TICKLESS != 0 && PIXELUI_ENABLE_TICKLESS != 1
#error "PIXELUI_ENABLE_TICKLESS must be 0 or 1"
#endif

/* Feature dependencies. */
#if !PIXELUI_USE_POPUP && \
    (PIXELUI_USE_POPUP_INFO || PIXELUI_USE_POPUP_PROGRESS || \
     PIXELUI_USE_POPUP_VALUE_DIGITS || PIXELUI_USE_POPUP_KEYBOARD)
#error "Popup types require PIXELUI_USE_POPUP"
#endif

#if PIXELUI_USE_POPUP && \
    !(PIXELUI_USE_POPUP_INFO || PIXELUI_USE_POPUP_PROGRESS || \
      PIXELUI_USE_POPUP_VALUE_DIGITS || PIXELUI_USE_POPUP_KEYBOARD)
#error "PIXELUI_USE_POPUP requires at least one enabled popup type"
#endif

#if PIXELUI_USE_POPUP_VALUE_DIGITS && !PIXELUI_USE_FOCUS
#error "PopupValueDigits requires PIXELUI_USE_FOCUS"
#endif

#if PIXELUI_USE_POPUP_KEYBOARD && !PIXELUI_USE_FOCUS
#error "PopupKeyboard requires PIXELUI_USE_FOCUS"
#endif

/* Compatibility names used by the current public headers. */
constexpr int MAX_ANIMATION_COUNT = PIXELUI_MAX_ANIMATION_COUNT;
constexpr int MAX_TEXT_LENGTH = PIXELUI_MAX_TEXT_LENGTH;
constexpr int MAX_APP_NUM = PIXELUI_MAX_APP_NUM;
constexpr int MAX_VIEW_DEPTH = PIXELUI_MAX_VIEW_DEPTH;
constexpr size_t APPLICATION_ARENA_SIZE = PIXELUI_APPLICATION_ARENA_SIZE;
constexpr size_t APPLICATION_ARENA_MAX_ALIGNMENT = PIXELUI_APPLICATION_ARENA_MAX_ALIGNMENT;
constexpr int CALLBACK_STORAGE_SIZE = PIXELUI_CALLBACK_STORAGE_SIZE;
constexpr int MAX_APPVIEW_SLOT_NUM = PIXELUI_MAX_APPVIEW_SLOT_NUM;
constexpr int MAX_LISTITEM_NAME_NUM = PIXELUI_MAX_LISTITEM_NAME_NUM;
constexpr int LISTVIEW_ITEMS_PER_PAGE = PIXELUI_LISTVIEW_ITEMS_PER_PAGE;
constexpr int MAX_LISTVIEW_DEPTH = PIXELUI_MAX_LISTVIEW_DEPTH;
constexpr int CALLBACK_ANIMATION_STACK_SIZE = PIXELUI_CALLBACK_ANIMATION_STACK_SIZE;
constexpr int MAX_POPUP_NUM = PIXELUI_MAX_POPUP_NUM;
constexpr int MAX_ONSCREEN_WIDGET_NUM = PIXELUI_MAX_ONSCREEN_WIDGET_NUM;
constexpr int MAX_INT_FIXED_WIDTH = PIXELUI_MAX_INT_FIXED_WIDTH;
constexpr int MAX_COROUTINE_NUM = PIXELUI_MAX_COROUTINE_NUM;

/* Capacity validation. */
static_assert(MAX_APP_NUM > 0, "PixelUI needs at least one application slot");
static_assert(MAX_VIEW_DEPTH > 0, "PixelUI needs at least one view stack entry");
static_assert(APPLICATION_ARENA_SIZE > 0, "Application arena must not be empty");
static_assert(CALLBACK_STORAGE_SIZE > 0, "Callback storage must not be empty");
static_assert(MAX_TEXT_LENGTH > 0, "Text capacity must be positive");
static_assert(MAX_APPVIEW_SLOT_NUM > 0, "AppView capacity must be positive");
static_assert(MAX_LISTITEM_NAME_NUM > 0, "List item text capacity must be positive");
static_assert(LISTVIEW_ITEMS_PER_PAGE > 0, "ListView page size must be positive");
static_assert(MAX_LISTVIEW_DEPTH > 0, "ListView depth must be positive");
static_assert(CALLBACK_ANIMATION_STACK_SIZE > 0,
              "Callback animation stack must be positive");
static_assert(MAX_INT_FIXED_WIDTH > 0, "Fixed-width integers need at least one digit");
static_assert(MAX_ANIMATION_COUNT > 0, "Animation capacity must be positive");
static_assert(MAX_POPUP_NUM > 0, "Popup capacity must be positive");
static_assert(MAX_ONSCREEN_WIDGET_NUM > 0, "Focus capacity must be positive");
static_assert(MAX_COROUTINE_NUM > 0, "Coroutine capacity must be positive");

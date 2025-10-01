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

enum class PopupPosition {
    CENTER,
    BOTTOM
};

enum class PopupType {
    INFO,
    WARNING,
    ERROR,
    CONFIRM
};

enum class EasingType {
    LINEAR,
    EASE_IN_QUAD,
    EASE_OUT_QUAD,
    EASE_IN_OUT_QUAD,
    EASE_IN_CUBIC,
    EASE_OUT_CUBIC,
    EASE_IN_OUT_CUBIC,
    EASE_OUT_BOUNCE
};

enum class InputEvent {
    UP, DOWN, LEFT, RIGHT, SELECT, BACK
};

enum class PROTECTION {
    NOT_PROTECTED,
    PROTECTED
};

struct FocusBox {
    int32_t x;
    int32_t y;
    int32_t w;
    int32_t h;

    bool operator==(const FocusBox& other) const {
        return (x == other.x && y == other.y && w == other.w && h == other.h);
    }
};
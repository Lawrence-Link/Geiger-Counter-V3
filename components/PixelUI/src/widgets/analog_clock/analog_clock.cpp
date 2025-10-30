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

#include "widgets/analog_clock/analog_clock.h"
#include <cmath>

/**
 * @brief Construct a new Clock object
 * 
 * @param ui Reference to the PixelUI instance
 */
Clock::Clock(PixelUI& ui, uint16_t pos_x, uint16_t pos_y, uint16_t radius) : 
    m_ui(ui),
    m_x(pos_x),
    m_y(pos_y),
    m_radius(radius)
{
    setFocusable(false); /**< Clock is display-only, cannot be focused */
}

/**
 * @brief Initialize the clock widget and start animations
 */
void Clock::onLoad() {
    m_anim_state = AnimState::EXPANDING;

    m_dial_progress = 0; /**< Progress of dial animation (0-360 deg) */
    m_marks_progress = 0; /**< Number of hour marks drawn (0-12) */

    /**< Animate dial drawing */
    m_ui.animate(m_dial_progress, 360, ANIM_DURATION, EasingType::EASE_OUT_CUBIC, PROTECTION::PROTECTED);

    /**< Animate hour marks appearance */
    m_ui.animate(m_marks_progress, 14, 700, EasingType::EASE_IN_OUT_CUBIC, PROTECTION::PROTECTED);

    /**< Animate lengths of hour, minute, second hands */
    m_ui.animate(m_length_hand_h, m_radius - 10, 700, EasingType::EASE_IN_OUT_CUBIC, PROTECTION::PROTECTED);
    m_ui.animate(m_length_hand_m, m_radius - 4, 400, EasingType::EASE_IN_OUT_CUBIC, PROTECTION::PROTECTED);
    m_ui.animate(m_length_hand_s, m_radius - 2, 300, EasingType::EASE_IN_OUT_CUBIC, PROTECTION::PROTECTED);
}

/**
 * @brief Clear any unprotected animations when the widget is offloaded
 */
void Clock::onOffload() {
    m_ui.clearUnprotectedAnimations();
}

/**
 * @brief Clock is not interactive
 * @return false Always returns false
 */
bool Clock::onSelect() { return false; }

/**
 * @brief Clock does not handle input events
 * @param event Input event
 * @return false Always returns false
 */
bool Clock::handleEvent(InputEvent event) { return false; }

/**
 * @brief Draw the clock widget
 */
void Clock::draw() {
    /**< Check if the initial animations are complete */
    if (m_anim_state == AnimState::EXPANDING &&
        m_dial_progress >= 360 && m_marks_progress >= 12) {
        m_anim_state = AnimState::FINISHED;
    }

    /**< Draw components in order */
    drawDial();
    drawHourMarks();
    drawHands();
}

/**
 * @brief Draw the dial (circle) with animation progress
 */
void Clock::drawDial() {
    U8G2& u8g2 = m_ui.getU8G2();

    if (m_dial_progress >= 360) {
        u8g2.drawCircle(m_x, m_y, m_radius); /**< Full dial */
    } else if (m_dial_progress > 0) {
        const uint8_t TOP = 64; /**< Starting angle */
        uint8_t len = (uint8_t)((m_dial_progress * 256UL) / 360UL);

        uint8_t arcStart = TOP;
        uint8_t arcEnd = (uint8_t)(TOP + len);

        if (arcEnd < arcStart) {
            /**< Split arc if wrapping around */
            u8g2.drawArc(m_x, m_y, m_radius, arcStart, 255);
            u8g2.drawArc(m_x, m_y, m_radius, 0, arcEnd);
        } else {
            u8g2.drawArc(m_x, m_y, m_radius, arcStart, arcEnd);
        }
    }
}

/**
 * @brief Draw hour marks (ticks) around the dial
 */
void Clock::drawHourMarks() {
    U8G2& u8g2 = m_ui.getU8G2();

    int visible_marks = m_marks_progress;
    if (visible_marks > 12) visible_marks = 12;

    for (int i = 0; i < visible_marks; i++) {
        int angle = i * 30 - 90; /**< Start from top (-90 degrees) */
        int outer_x, outer_y, inner_x, inner_y;
        getPointOnCircle(angle, m_radius - 2, outer_x, outer_y);
        getPointOnCircle(angle, m_radius - 6, inner_x, inner_y);

        u8g2.drawLine(outer_x, outer_y, inner_x, inner_y);
    }
}

/**
 * @brief Draw hour, minute, and second hands
 */
void Clock::drawHands() {
    U8G2& u8g2 = m_ui.getU8G2();

    /**< Hour hand (with minute offset) */
    int hour_angle = (m_hour % 12) * 30 + (m_minute * 30) / 60 - 90;
    int hour_x, hour_y;
    getPointOnCircle(hour_angle, m_length_hand_h, hour_x, hour_y);
    u8g2.drawLine(m_x, m_y, hour_x, hour_y);

    /**< Minute hand */
    int minute_angle = m_minute * 6 - 90;
    int minute_x, minute_y;
    getPointOnCircle(minute_angle, m_length_hand_m, minute_x, minute_y);
    u8g2.drawLine(m_x, m_y, minute_x, minute_y);

    /**< Second hand */
    int second_angle = m_second * 6 - 90;
    int second_x, second_y;
    getPointOnCircle(second_angle, m_length_hand_s, second_x, second_y);
    u8g2.drawLine(m_x, m_y, second_x, second_y);

    /**< Draw center dot */
    u8g2.drawDisc(m_x, m_y, 2);
}

/**
 * @brief Convert angle in degrees to radians
 * @param angle Angle in degrees
 * @return float Angle in radians
 */
float Clock::angleToRadians(int angle) const {
    return (float)angle * M_PI / 180.0f;
}

/**
 * @brief Compute point coordinates on a circle
 * 
 * @param angle Angle in degrees from 0° horizontal
 * @param radius Radius of the circle
 * @param x Output x coordinate
 * @param y Output y coordinate
 */
void Clock::getPointOnCircle(int angle, uint16_t radius, int& x, int& y) const {
    float rad = angleToRadians(angle);
    x = m_x + (int)(cos(rad) * radius);
    y = m_y + (int)(sin(rad) * radius);
}

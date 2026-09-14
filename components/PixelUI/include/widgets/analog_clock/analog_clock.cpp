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

#include "widgets/analog_clock/analog_clock.h"
#include "PixelUI.h"

namespace {

constexpr int32_t TRIG_SCALE = 32767;
constexpr int32_t DEGREES_PER_STEP = 6;
constexpr int16_t SIN_Q15[] = {
    0, 3425, 6813, 10126, 13328, 16383, 19260, 21925,
    24351, 26509, 28377, 29934, 31163, 32051, 32587, 32767,
};

/** @brief firstQuadrantSinQ15. */
int32_t firstQuadrantSinQ15(int angle) {
    if (angle >= 90) return SIN_Q15[15];

    const int index = angle / DEGREES_PER_STEP;
    const int remainder = angle % DEGREES_PER_STEP;
    const int32_t lower = SIN_Q15[index];
    const int32_t upper = SIN_Q15[index + 1];
    return lower + ((upper - lower) * remainder) / DEGREES_PER_STEP;
}

/** @brief sinDegreesQ15. */
int32_t sinDegreesQ15(int angle) {
    angle %= 360;
    if (angle < 0) angle += 360;

    const int quadrant = angle / 90;
    const int offset = angle % 90;
    switch (quadrant) {
        case 0: return firstQuadrantSinQ15(offset);
        case 1: return firstQuadrantSinQ15(90 - offset);
        case 2: return -firstQuadrantSinQ15(offset);
        default: return -firstQuadrantSinQ15(90 - offset);
    }
}

/** @brief scaleTrig. */
int scaleTrig(int32_t trigValue, uint16_t radius) {
    const int64_t product = static_cast<int64_t>(trigValue) * radius;
    const int64_t rounded = product >= 0
        ? product + (TRIG_SCALE / 2)
        : product - (TRIG_SCALE / 2);
    return static_cast<int>(rounded / TRIG_SCALE);
}

} // namespace

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
    setWidgetBounds({m_x - m_radius, m_y - m_radius, 2 * m_radius + 1, 2 * m_radius + 1});
}

/**
 * @brief Initialize the clock widget and start animations
 */
void Clock::onLoad() {
    onLoad(LoadTransition::Animated);
}

/** @brief Clock::onLoad. */
void Clock::onLoad(LoadTransition transition) {
    const int32_t finalHourHandLength = m_radius - 10;
    const int32_t finalMinuteHandLength = m_radius - 4;
    const int32_t finalSecondHandLength = m_radius - 2;

    if (transition == LoadTransition::Immediate) {
        m_anim_state = AnimState::FINISHED;
        m_dial_progress = 360;
        m_marks_progress = 14;
        m_length_hand_h = finalHourHandLength;
        m_length_hand_m = finalMinuteHandLength;
        m_length_hand_s = finalSecondHandLength;
        m_ui.markDirty();
        return;
    }

    m_anim_state = AnimState::EXPANDING;

    m_dial_progress = 0; /**< Progress of dial animation (0-360 deg) */
    m_marks_progress = 0; /**< Number of hour marks drawn (0-12) */
    m_length_hand_h = 0;
    m_length_hand_m = 0;
    m_length_hand_s = 0;

    /**< Animate dial drawing */
    if (!m_ui.animate(m_dial_progress, 360, ANIM_DURATION, EasingType::EASE_OUT_CUBIC, PROTECTION::PROTECTED)) {
        m_dial_progress = 360;
    }

    /**< Animate hour marks appearance */
    if (!m_ui.animate(m_marks_progress, 14, 700, EasingType::EASE_IN_OUT_CUBIC, PROTECTION::PROTECTED)) {
        m_marks_progress = 14;
    }

    /**< Animate lengths of hour, minute, second hands */
    if (!m_ui.animate(m_length_hand_h, finalHourHandLength, 700, EasingType::EASE_IN_OUT_CUBIC, PROTECTION::PROTECTED)) {
        m_length_hand_h = finalHourHandLength;
    }
    if (!m_ui.animate(m_length_hand_m, finalMinuteHandLength, 400, EasingType::EASE_IN_OUT_CUBIC, PROTECTION::PROTECTED)) {
        m_length_hand_m = finalMinuteHandLength;
    }
    if (!m_ui.animate(m_length_hand_s, finalSecondHandLength, 300, EasingType::EASE_IN_OUT_CUBIC, PROTECTION::PROTECTED)) {
        m_length_hand_s = finalSecondHandLength;
    }
    m_ui.markDirty();
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
Canvas& Clock::display() { return m_ui.getCanvas(); }

/** @brief Clock::drawSelf. */
void Clock::drawSelf(const WidgetRenderContext& context) {
    draw_origin_x_ = context.originX;
    draw_origin_y_ = context.originY;
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
    Canvas& u8g2 = m_ui.getCanvas();

    if (m_dial_progress >= 360) {
        u8g2.drawCircle(draw_origin_x_ + m_x, draw_origin_y_ + m_y, m_radius); /**< Full dial */
    } else if (m_dial_progress > 0) {
        const uint8_t TOP = 64; /**< Starting angle */
        uint8_t len = (uint8_t)((m_dial_progress * 256UL) / 360UL);

        uint8_t arcStart = TOP;
        uint8_t arcEnd = (uint8_t)(TOP + len);

        if (arcEnd < arcStart) {
            /**< Split arc if wrapping around */
            u8g2.drawArc(draw_origin_x_ + m_x, draw_origin_y_ + m_y, m_radius, arcStart, 255);
            u8g2.drawArc(draw_origin_x_ + m_x, draw_origin_y_ + m_y, m_radius, 0, arcEnd);
        } else {
            u8g2.drawArc(draw_origin_x_ + m_x, draw_origin_y_ + m_y, m_radius, arcStart, arcEnd);
        }
    }
}

/**
 * @brief Draw hour marks (ticks) around the dial
 */
void Clock::drawHourMarks() {
    Canvas& u8g2 = m_ui.getCanvas();

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
    Canvas& u8g2 = m_ui.getCanvas();

    /**< Hour hand (with minute offset) */
    int hour_angle = (m_hour % 12) * 30 + (m_minute * 30) / 60 - 90;
    int hour_x, hour_y;
    getPointOnCircle(hour_angle, m_length_hand_h, hour_x, hour_y);
    u8g2.drawLine(draw_origin_x_ + m_x, draw_origin_y_ + m_y, hour_x, hour_y);

    /**< Minute hand */
    int minute_angle = m_minute * 6 - 90;
    int minute_x, minute_y;
    getPointOnCircle(minute_angle, m_length_hand_m, minute_x, minute_y);
    u8g2.drawLine(draw_origin_x_ + m_x, draw_origin_y_ + m_y, minute_x, minute_y);

    /**< Second hand */
    int second_angle = m_second * 6 - 90;
    int second_x, second_y;
    getPointOnCircle(second_angle, m_length_hand_s, second_x, second_y);
    u8g2.drawLine(draw_origin_x_ + m_x, draw_origin_y_ + m_y, second_x, second_y);

    /**< Draw center dot */
    u8g2.drawDisc(draw_origin_x_ + m_x, draw_origin_y_ + m_y, 2);
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
    x = draw_origin_x_ + m_x + scaleTrig(sinDegreesQ15(angle + 90), radius);
    y = draw_origin_y_ + m_y + scaleTrig(sinDegreesQ15(angle), radius);
}

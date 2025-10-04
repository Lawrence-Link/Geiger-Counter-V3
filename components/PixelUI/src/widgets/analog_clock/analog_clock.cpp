/*
 * Copyright (C) 2025 Lawrence Link
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "widgets/analog_clock/analog_clock.h"
#include <cmath>

Clock::Clock(PixelUI& ui) : m_ui(ui) {
    setFocusable(false); // not focasable
}

void Clock::onLoad() {
    m_anim_state = AnimState::EXPANDING;
    
    m_dial_progress = 0;
    m_marks_progress = 0;
    
    m_ui.animate(m_dial_progress,
                360, 
                ANIM_DURATION, EasingType::EASE_OUT_CUBIC, 
                PROTECTION::PROTECTED);
    m_ui.animate(m_marks_progress, 14, 
                700, EasingType::EASE_IN_OUT_CUBIC,
                PROTECTION::PROTECTED);
    m_ui.animate(m_length_hand_h, m_radius - 10, 
                700, EasingType::EASE_IN_OUT_CUBIC,
                PROTECTION::PROTECTED);
    m_ui.animate(m_length_hand_m, m_radius - 4, 
                400, EasingType::EASE_IN_OUT_CUBIC,
                PROTECTION::PROTECTED);
    m_ui.animate(m_length_hand_s, m_radius - 2, 
                300, EasingType::EASE_IN_OUT_CUBIC,
                PROTECTION::PROTECTED);
}

void Clock::onOffload() {
    m_ui.clearUnprotectedAnimations();
}

bool Clock::onSelect() {
    return false; // Clock is not interactive
}

bool Clock::handleEvent(InputEvent event) {
    return false; // Clock doesn't handle input events
}

void Clock::draw() {

    if (m_anim_state == AnimState::EXPANDING && 
        m_dial_progress >= 360 && m_marks_progress >= 12) {
        m_anim_state = AnimState::FINISHED;
    }
    
    U8G2& u8g2 = m_ui.getU8G2();
    
    drawDial();
    drawHourMarks();

    // if (m_anim_state == AnimState::FINISHED) {
        drawHands();
    // }
}

void Clock::drawDial() {
    U8G2& u8g2 = m_ui.getU8G2();
    
    if (m_dial_progress >= 360) {
        u8g2.drawCircle(m_x, m_y, m_radius);
    } else if (m_dial_progress > 0) {
        const uint8_t TOP = 64;
        
        uint8_t len = (uint8_t)((m_dial_progress * 256UL) / 360UL);
        
        uint8_t arcStart = TOP;
        uint8_t arcEnd = (uint8_t)(TOP + len);

        if (arcEnd < arcStart) {
            // devide into two halfs
            u8g2.drawArc(m_x, m_y, m_radius, arcStart, 255);
            u8g2.drawArc(m_x, m_y, m_radius, 0, arcEnd);
        } else {
            u8g2.drawArc(m_x, m_y, m_radius, arcStart, arcEnd);
        }
    }
}

void Clock::drawHourMarks() {
    U8G2& u8g2 = m_ui.getU8G2();
    
    // show mark according to animation progress
    int visible_marks = m_marks_progress;
    if (visible_marks > 12) visible_marks = 12;
    
    for (int i = 0; i < visible_marks; i++) {
        int angle = i * 30 - 90; // from -90 degress, 30 degress each
        
        int outer_x, outer_y, inner_x, inner_y;
        getPointOnCircle(angle, m_radius - 2, outer_x, outer_y);
        getPointOnCircle(angle, m_radius - 6, inner_x, inner_y);
        
        u8g2.drawLine(outer_x, outer_y, inner_x, inner_y);
    }
}

void Clock::drawHands() {
    U8G2& u8g2 = m_ui.getU8G2();
    
    // hours
    int hour_angle = (m_hour % 12) * 30 + (m_minute * 30) / 60 - 90; // including minute 
    int hour_x, hour_y;
    getPointOnCircle(hour_angle, m_length_hand_h, hour_x, hour_y);
    u8g2.drawLine(m_x, m_y, hour_x, hour_y);
    
    // minutes
    int minute_angle = m_minute * 6 - 90; // 6 degress per minutes
    int minute_x, minute_y;
    getPointOnCircle(minute_angle, m_length_hand_m, minute_x, minute_y);
    u8g2.drawLine(m_x, m_y, minute_x, minute_y);
    
    // seconds
    int second_angle = m_second * 6 - 90; // 6 degress per second
    int second_x, second_y;
    getPointOnCircle(second_angle, m_length_hand_s, second_x, second_y);
    u8g2.drawLine(m_x, m_y, second_x, second_y);
    
    // 中心点
    u8g2.drawDisc(m_x, m_y, 2);
}

float Clock::angleToRadians(int angle) const {
    return (float)angle * M_PI / 180.0f;
}

void Clock::getPointOnCircle(int angle, uint16_t radius, int& x, int& y) const {
    float rad = angleToRadians(angle);
    x = m_x + (int)(cos(rad) * radius);
    y = m_y + (int)(sin(rad) * radius);
}
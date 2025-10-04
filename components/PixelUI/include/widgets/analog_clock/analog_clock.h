/*
 * Copyright (C) 2025 Lawrence Link
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include "../IWidget.h"
#include "PixelUI.h"

/**
 * @class Clock
 * @brief An analog clock widget with animated dial and hour marks
 * 
 * This widget displays an analog clock with customizable radius.
 * Features animated loading sequence: dial and hour marks appear 
 * simultaneously using the PixelUI animate function.
 */
class Clock : public IWidget {
public:
    Clock(PixelUI& ui);
    ~Clock() = default;

    void onLoad() override;
    void onOffload() override;
    void draw() override;
    bool handleEvent(InputEvent event) override;
    bool onSelect() override;

    void setPosition(uint16_t x, uint16_t y) { m_x = x; m_y = y; }
    void setRadius(uint16_t radius) { m_radius = radius; }
    
    // API functions
    void setHour(uint8_t hour) { m_hour = hour % 12; }
    void setMinute(uint8_t minute) { m_minute = minute % 60; }
    void setSecond(uint8_t second) { m_second = second % 60; }

private:
    enum class AnimState {
        EXPANDING,    // 表盘和刻度同步扩张
        FINISHED
    };

    PixelUI& m_ui;
    
    uint16_t m_x = 64;        // Center X
    uint16_t m_y = 32;        // Center Y
    uint16_t m_radius = 25;   // Clock radius
    
    uint8_t m_hour = 12;      // Current hour (0-11)
    uint8_t m_minute = 0;     // Current minute (0-59)
    uint8_t m_second = 0;     // Current second (0-59)
    
    // 动画状态
    AnimState m_anim_state = AnimState::EXPANDING;  
    int32_t m_dial_progress = 0;     // (0-360)
    int32_t m_marks_progress = 0;    // (0-14 for better curve)
    
    int32_t m_length_hand_h = 0;
    int32_t m_length_hand_m = 0;
    int32_t m_length_hand_s = 0;

    static constexpr uint32_t ANIM_DURATION = 1000;
    
    void drawDial();
    void drawHourMarks();
    void drawHands();
    
    // Helper functions for angle calculations
    float angleToRadians(int angle) const;
    void getPointOnCircle(int angle, uint16_t radius, int& x, int& y) const;
};
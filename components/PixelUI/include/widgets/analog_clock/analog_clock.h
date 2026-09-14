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

#include "../IWidget.h"

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
/** @brief Clock. */
    Clock(PixelUI& ui, uint16_t pos_x, uint16_t pos_y, uint16_t radius);
/** @brief ~Clock. */
    ~Clock() = default;

/** @brief onLoad. */
    void onLoad() override;
/** @brief onLoad. */
    void onLoad(LoadTransition transition);
/** @brief onOffload. */
    void onOffload() override;
/** @brief handleEvent. */
    bool handleEvent(InputEvent event) override;
/** @brief onSelect. */
    bool onSelect() override;

/** @brief setPosition. */
    void setPosition(uint16_t x, uint16_t y) {
        m_x = x; m_y = y;
        setWidgetBounds({m_x - m_radius, m_y - m_radius, 2 * m_radius + 1, 2 * m_radius + 1});
    }
/** @brief setRadius. */
    void setRadius(uint16_t radius) {
        m_radius = radius;
        setWidgetBounds({m_x - m_radius, m_y - m_radius, 2 * m_radius + 1, 2 * m_radius + 1});
    }
    
    // API functions
/** @brief setHour. */
    void setHour(uint8_t hour) { m_hour = hour % 12; }
/** @brief setMinute. */
    void setMinute(uint8_t minute) { m_minute = minute % 60; }
/** @brief setSecond. */
    void setSecond(uint8_t second) { m_second = second % 60; }

private:
    enum class AnimState {
        EXPANDING,
        FINISHED
    };

    PixelUI& m_ui;
    
    uint16_t m_x = 64;        // Center X
    uint16_t m_y = 32;        // Center Y
    uint16_t m_radius = 25;   // Clock radius
    
    uint8_t m_hour = 12;      // Current hour (0-11)
    uint8_t m_minute = 0;     // Current minute (0-59)
    uint8_t m_second = 0;     // Current second (0-59)
    
    // Animation varibles
    AnimState m_anim_state = AnimState::EXPANDING;  
    int32_t m_dial_progress = 0;     // (0-360)
    int32_t m_marks_progress = 0;    // (0-14 for better curve)
    
    int32_t m_length_hand_h = 0;
    int32_t m_length_hand_m = 0;
    int32_t m_length_hand_s = 0;

    static constexpr uint32_t ANIM_DURATION = 1000;
    
/** @brief drawDial. */
    void drawDial();
/** @brief drawHourMarks. */
    void drawHourMarks();
/** @brief drawHands. */
    void drawHands();
/** @brief drawSelf. */
    void drawSelf(const WidgetRenderContext& context) override;
/** @brief display. */
    Canvas& display() override;

    int32_t draw_origin_x_ = 0;
    int32_t draw_origin_y_ = 0;
    
    // Integer-only helper for angle calculations.
/** @brief getPointOnCircle. */
    void getPointOnCircle(int angle, uint16_t radius, int& x, int& y) const;
};

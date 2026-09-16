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

#include "core/app/IApplication.h"
#include "PixelUI.h"
#include "core/app/app_system.h"
#include "app_registry.h"
#include <memory>
#include <ctime>
#include "time_module.h"
#include "i2c.h"
#include "system_nvs_varibles.h"
#include <cstdio>

// External variable from battery_task
extern int battery_percentage;

/** @brief Bitmap data for clock application icon (24x24). */
__attribute__((aligned(4)))
static const unsigned char image_clock_bits[] = {
    0xf0,0xff,0x0f,0xfc,0xff,0x3f,0xfe,0xff,0x7f,0xfe,0xff,0x7f,0xff,0x83,0xff,0xff,0x7c,0xfe,0x7f,0xff,0xfd,0xbf,0xff,0xfb,0xff,0xff,0xff,0xef,0x7e,0xc5,0x67,0x6e,0xf5,0xef,0x7e,0xe4,0xef,0xfe,0xdd,0x47,0xec,0xe5,0xff,0xff,0xff,0xbf,0xff,0xfb,0x7f,0xff,0xfd,0xff,0x7c,0xfe,0xff,0x83,0xff,0xff,0xff,0xff,0xfe,0xff,0x7f,0xfe,0xff,0x7f,0xfc,0xff,0x3f,0xf0,0xff,0x0f    
};

/** @brief Battery icon bitmaps (10x6). */
__attribute__((aligned(4)))
static const unsigned char image_BAT_FULL_bits[] = {0xff,0x01,0xff,0x03,0xff,0x03,0xff,0x03,0xff,0x03,0xff,0x01};
__attribute__((aligned(4)))
static const unsigned char image_BAT_75_bits[] = {0xff,0x01,0x3f,0x03,0x3f,0x03,0x3f,0x03,0x3f,0x03,0xff,0x01};
__attribute__((aligned(4)))
static const unsigned char image_BAT_50_bits[] = {0xff,0x01,0x1f,0x03,0x1f,0x03,0x1f,0x03,0x1f,0x03,0xff,0x01};
__attribute__((aligned(4)))
static const unsigned char image_BAT_25_bits[] = {0xff,0x01,0x07,0x03,0x07,0x03,0x07,0x03,0x07,0x03,0xff,0x01};
__attribute__((aligned(4)))
static const unsigned char image_BAT_empty_bits[] = {0xff,0x01,0x01,0x03,0x01,0x03,0x01,0x03,0x01,0x03,0xff,0x01};

/**
 * @brief Structure to hold scrolling animation state for a single digit.
 */
struct DigitState {
    int32_t currentValue;     ///< Current displayed value (0-9)
    int32_t offsetY;          ///< Vertical offset for animation (in pixels)
    bool isAnimating;         ///< Whether this digit is currently animating
};

/**
 * @brief Pixel Clock application with scrolling digit animations.
 */
class APP_PIXEL_CLOCK : public IApplication {
private:
    PixelUI& m_ui;
    
    // Digit states for HH:MM:SS
    DigitState hourTens;
    DigitState hourOnes;
    DigitState minuteTens;
    DigitState minuteOnes;
    DigitState secondTens;
    DigitState secondOnes;
    
    // Time synchronization
    uint32_t timestamp_prev = 0;
    uint32_t timestamp_now = 0;
    static constexpr uint32_t TICK_INTERVAL = 1000; // Tick every 1 second
    static constexpr uint32_t SYNC_INTERVAL = 60000; // Sync every 60 seconds
    uint32_t lastSyncTime = 0;
    bool isInitialized = false;
    
    // Time data from RTC
    struct tm timeinfo{};
    bool tm_valid = false;
    
    // Animation parameters
    static constexpr uint32_t DIGIT_ANIM_DURATION = 200; // milliseconds
    static constexpr int32_t DIGIT_HEIGHT_LARGE = 32; // Height for large font
    static constexpr int32_t DIGIT_HEIGHT_SMALL = 16; // Height for small font
    
    // Battery icon state
    uint32_t last_battery_update = 0;
    const unsigned char* current_battery_icon = image_BAT_FULL_bits;
    char print_buffer[32];

    static constexpr int16_t HEADER_BASELINE_Y = 8;
    static constexpr int16_t HEADER_DIVIDER_Y = 11;
    static constexpr int16_t TIME_BASELINE_Y = 52;
    static constexpr int16_t SECONDS_BASELINE_Y = 49;
    static constexpr int16_t PROGRESS_X = 2;
    static constexpr int16_t PROGRESS_Y = 59;
    static constexpr int16_t PROGRESS_WIDTH = 124;

    static const char* weekdayName(int weekday) {
        static constexpr const char* WEEKDAYS[] = {
            "SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"
        };
        return weekday >= 0 && weekday < 7 ? WEEKDAYS[weekday] : "---";
    }

    void updateBatteryIcon() {
        // Pick the icon whose nominal charge level is nearest to the reading.
        if (battery_percentage >= 88) {
            current_battery_icon = image_BAT_FULL_bits;
        } else if (battery_percentage >= 63) {
            current_battery_icon = image_BAT_75_bits;
        } else if (battery_percentage >= 38) {
            current_battery_icon = image_BAT_50_bits;
        } else if (battery_percentage >= 13) {
            current_battery_icon = image_BAT_25_bits;
        } else {
            current_battery_icon = image_BAT_empty_bits;
        }
    }

    void drawHeader(U8G2& u8g2) {
        u8g2.setFont(u8g2_font_5x7_mf);

        if (tm_valid) {
            std::snprintf(print_buffer, sizeof(print_buffer), "%s %02d/%02d",
                          weekdayName(timeinfo.tm_wday),
                          timeinfo.tm_mon + 1,
                          timeinfo.tm_mday);
        } else {
            std::snprintf(print_buffer, sizeof(print_buffer), "RTC --/--");
        }
        u8g2.drawStr(2, HEADER_BASELINE_Y, print_buffer);

        int battery = battery_percentage;
        if (battery < 0) battery = 0;
        if (battery > 100) battery = 100;
        std::snprintf(print_buffer, sizeof(print_buffer), "%d%%", battery);
        const int16_t textWidth = u8g2.getStrWidth(print_buffer);
        const int16_t textX = u8g2.getDisplayWidth() - 2 - textWidth;
        u8g2.drawXBM(textX - 12, 2, 10, 6, current_battery_icon);
        u8g2.drawStr(textX, HEADER_BASELINE_Y, print_buffer);

        u8g2.drawHLine(2, HEADER_DIVIDER_Y, u8g2.getDisplayWidth() - 4);
    }

    void drawSecondProgress(U8G2& u8g2) {
        const int second = tm_valid
            ? secondTens.currentValue * 10 + secondOnes.currentValue
            : 0;
        const int16_t innerWidth = PROGRESS_WIDTH - 4;
        const int16_t filledWidth = tm_valid
            ? static_cast<int16_t>((innerWidth * (second + 1)) / 60)
            : 0;

        u8g2.drawFrame(PROGRESS_X, PROGRESS_Y, PROGRESS_WIDTH, 4);
        if (filledWidth > 0) {
            u8g2.drawBox(PROGRESS_X + 2, PROGRESS_Y + 1, filledWidth, 2);
        }
    }
public:
    APP_PIXEL_CLOCK(PixelUI& ui, void* param) : m_ui(ui) {
        resetDigitState(hourTens);
        resetDigitState(hourOnes);
        resetDigitState(minuteTens);
        resetDigitState(minuteOnes);
        resetDigitState(secondTens);
        resetDigitState(secondOnes);
    }
    
    void onEnter(ExitCallback cb) override {
        IApplication::onEnter(cb);
        m_ui.setContinousDraw(true);
        
        // Synchronize with RTC time on startup
        isInitialized = false;
        timestamp_now = timestamp_prev = m_ui.getCurrentTime();
        lastSyncTime = timestamp_now;
        syncWithRTCTime();
    }
    
    void resetDigitState(DigitState& digit) {
        digit.currentValue = 0;
        digit.offsetY = 0;
        digit.isAnimating = false;
    }
    
    /**
     * @brief Start animation for a digit from current to target value.
     */
    void animateDigit(DigitState& digit, int newValue, bool isLarge = true) {
        // Don't animate if already animating or value hasn't changed
        if (digit.isAnimating || digit.currentValue == newValue) {
            return;
        }
        
        // Immediately update to new value
        digit.currentValue = newValue;
        
        // Set initial offset based on font size
        int32_t digitHeight = isLarge ? DIGIT_HEIGHT_LARGE : DIGIT_HEIGHT_SMALL;
        digit.offsetY = digitHeight;
        digit.isAnimating = true;
        
        // Animate offset back to 0
        m_ui.animate(digit.offsetY, 0, DIGIT_ANIM_DURATION, EasingType::EASE_OUT_QUAD);
        
        m_ui.markDirty();
    }
    
    /**
     * @brief Synchronize with RTC time (only called on startup and periodic sync).
     */
    void syncWithRTCTime() {
        // Read time from PCF8563 RTC
        tm_valid = false;
        const esp_err_t result = pcf8563_get_time(pcf8563_dev, &timeinfo, &tm_valid);
        if (result != ESP_OK) {
            tm_valid = false;
        }
        
        if (tm_valid) {
            int hour = timeinfo.tm_hour;
            int minute = timeinfo.tm_min;
            int second = timeinfo.tm_sec;
            
            // Set all digits without animation
            hourTens.currentValue = hour / 10;
            hourOnes.currentValue = hour % 10;
            minuteTens.currentValue = minute / 10;
            minuteOnes.currentValue = minute % 10;
            secondTens.currentValue = second / 10;
            secondOnes.currentValue = second % 10;
            
            isInitialized = true;
        } else {
            isInitialized = false;
        }
    }
    
    /**
     * @brief Increment the clock by one second (event-driven).
     */
    void incrementSecond() {
        // Calculate new values before animating
        int currentSecondOnes = secondOnes.currentValue;
        int currentSecondTens = secondTens.currentValue;
        int newSecondOnes = (currentSecondOnes + 1) % 10;
        int newSecondTens = currentSecondTens;
        bool minuteCarry = false;
        
        // Check if we need to increment seconds tens
        if (newSecondOnes == 0) {
            newSecondTens = (currentSecondTens + 1) % 6;
            // Check if we need to increment minutes (60 seconds passed)
            if (newSecondTens == 0) {
                minuteCarry = true;
            }
        }
        
        // Now animate the changes
        animateDigit(secondOnes, newSecondOnes, false); // Small font
        if (newSecondTens != currentSecondTens) {
            animateDigit(secondTens, newSecondTens, false); // Small font
        }
        if (minuteCarry) {
            incrementMinute();
        }
    }
    
    /**
     * @brief Increment the minutes.
     */
    void incrementMinute() {
        // Calculate new values before animating
        int currentMinuteOnes = minuteOnes.currentValue;
        int currentMinuteTens = minuteTens.currentValue;
        int newMinuteOnes = (currentMinuteOnes + 1) % 10;
        int newMinuteTens = currentMinuteTens;
        bool hourCarry = false;
        
        // Check if we need to increment minutes tens
        if (newMinuteOnes == 0) {
            newMinuteTens = (currentMinuteTens + 1) % 6;
            // Check if we need to increment hours (60 minutes passed)
            if (newMinuteTens == 0) {
                hourCarry = true;
            }
        }
        
        // Now animate the changes
        animateDigit(minuteOnes, newMinuteOnes, true); // Large font
        if (newMinuteTens != currentMinuteTens) {
            animateDigit(minuteTens, newMinuteTens, true); // Large font
        }
        if (hourCarry) {
            incrementHour();
        }
    }
    
    /**
     * @brief Increment the hours.
     */
    void incrementHour() {
        // Calculate current hour value before any animation
        int currentHour = hourTens.currentValue * 10 + hourOnes.currentValue;
        int newHour = (currentHour + 1) % 24;
        int newHourTens = newHour / 10;
        int newHourOnes = newHour % 10;
        
        // Now animate both digits
        animateDigit(hourTens, newHourTens, true); // Large font
        animateDigit(hourOnes, newHourOnes, true); // Large font
    }
    
    /**
     * @brief Draw a single digit with scrolling effect.
     */
    void drawScrollingDigit(int16_t x, int16_t y, DigitState& digit, bool useLargeFont, int16_t width, int16_t height) {
        U8G2& u8g2 = m_ui.getU8G2();
        
        int32_t digitHeight = useLargeFont ? DIGIT_HEIGHT_LARGE : DIGIT_HEIGHT_SMALL;
        
        if (useLargeFont) {
            u8g2.setFont(u8g2_font_maniac_tf);
        } else {
            u8g2.setFont(u8g2_font_pixzillav1_tr);
        }
        
        // Set clipping window - extend downward to ensure bottom digits are clipped
        u8g2.setClipWindow(x, y - height, x + width, y + 4);
        
        // Draw previous, current, and next digit (like NumScroll)
        char buf[2] = {0};
        
        // Draw previous digit (above, only visible when scrolling down)
        int prevValue = (digit.currentValue - 1 + 10) % 10;
        buf[0] = '0' + prevValue;
        int16_t prevY = y - digitHeight + digit.offsetY;
        u8g2.drawStr(x, prevY, buf);
        
        // Draw current digit (centered, with offset)
        buf[0] = '0' + digit.currentValue;
        int16_t currY = y + digit.offsetY;
        u8g2.drawStr(x, currY, buf);
        
        // Draw next digit (below, only visible when scrolling up)
        int nextValue = (digit.currentValue + 1) % 10;
        buf[0] = '0' + nextValue;
        int16_t nextY = y + digitHeight + digit.offsetY;
        u8g2.drawStr(x, nextY, buf);
        
        // Reset clipping window
        u8g2.setMaxClipWindow();
        
        // Check if animation is complete
        if (digit.isAnimating && digit.offsetY == 0) {
            digit.isAnimating = false;
        }
    }
    
    void draw() override {
        U8G2& u8g2 = m_ui.getU8G2();
        
        timestamp_now = m_ui.getCurrentTime();

        // Retry an invalid RTC as well as periodically correcting normal drift.
        if (timestamp_now - lastSyncTime >= SYNC_INTERVAL) {
            syncWithRTCTime();
            lastSyncTime = timestamp_now;
            timestamp_prev = timestamp_now;
        } else if (isInitialized && timestamp_now - timestamp_prev >= TICK_INTERVAL) {
            incrementSecond();
            timestamp_prev = timestamp_now;
        }

        if (timestamp_now - last_battery_update >= 1000) {
            last_battery_update = timestamp_now;
            updateBatteryIcon();
        }

        drawHeader(u8g2);

        constexpr int16_t hourX = 6;
        constexpr int16_t minuteX = 58;
        constexpr int16_t secondX = 107;
        
        // Font dimensions for clipping
        int16_t largeDigitWidth = 20;
        int16_t largeDigitHeight = 26;
        int16_t smallDigitWidth = 10;
        int16_t smallDigitHeight = 12;
        
        if (isInitialized) {
            // Draw hours and minutes as the dominant information layer.
            drawScrollingDigit(hourX, TIME_BASELINE_Y, hourTens, true, largeDigitWidth, largeDigitHeight);
            drawScrollingDigit(hourX + 20, TIME_BASELINE_Y, hourOnes, true, largeDigitWidth, largeDigitHeight);

            // The separator pulses once per second like a quiet clock heartbeat.
            if ((secondOnes.currentValue & 1) == 0) {
                u8g2.setFont(u8g2_font_freedoomr25_tn);
                u8g2.drawStr(43, TIME_BASELINE_Y, ":");
            }

            drawScrollingDigit(minuteX, TIME_BASELINE_Y, minuteTens, true, largeDigitWidth, largeDigitHeight);
            drawScrollingDigit(minuteX + 20, TIME_BASELINE_Y, minuteOnes, true, largeDigitWidth, largeDigitHeight);

            // Seconds remain readable, but no longer compete with HH:MM.
            drawScrollingDigit(secondX, SECONDS_BASELINE_Y, secondTens, false, smallDigitWidth, smallDigitHeight);
            drawScrollingDigit(secondX + 10, SECONDS_BASELINE_Y, secondOnes, false, smallDigitWidth, smallDigitHeight);
        } else {
            u8g2.setFont(u8g2_font_maniac_tf);
            u8g2.drawStr(17, TIME_BASELINE_Y, "--");
            u8g2.setFont(u8g2_font_freedoomr25_tn);
            u8g2.drawStr(48, TIME_BASELINE_Y, ":");
            u8g2.setFont(u8g2_font_maniac_tf);
            u8g2.drawStr(65, TIME_BASELINE_Y, "--");
        }

        drawSecondProgress(u8g2);
    }
    
    bool handleInput(InputEvent event) override {
        if (event == InputEvent::BACK) {
            requestExit();
            return true;
        }
        return false;
    }
    
    void onExit() override {
        m_ui.setContinousDraw(false);
    }
};

AppItem pixel_clock_app{
    .title = "像素钟",
    .bitmap = image_clock_bits,
    .factory = ApplicationFactory::make<APP_PIXEL_CLOCK>(),
};

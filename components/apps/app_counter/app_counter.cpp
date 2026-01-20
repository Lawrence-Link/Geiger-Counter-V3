/*
 * Copyright (C) 2025 Lawrence Link
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the
 * Free Software Foundation, either version 3 of the License, or
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

#include "app_counter.h"

 /**
 * @brief Formats a floating-point number into a string in a meter-style format (up to 4 significant digits).
 *
 * @param buffer Pointer to the destination string buffer.
 * @param buffer_size Maximum capacity of the buffer.
 * @param value The float value to format.
 * @param unit  Optional unit string ("uSv/h", "CPM", or NULL for no unit).
 * @return The number of characters written (excluding the null terminator), or a negative value on failure.
 */
int format_meter_style(char *buffer, size_t buffer_size, float value, const char *unit) {
    if (buffer_size == 0) return 0;
    
    bool withUnit = (unit != NULL && unit[0] != '\0');
    
    // 1) Zero/near-zero value displays a placeholder
    if (fabsf(value) < 1e-7f) {
        const char *placeholder;
        if (withUnit) {
            if (strcmp(unit, UNIT_CPM) == 0)
                placeholder = UNIT_PLACEHOLDER_CPM;
            else
                placeholder = UNIT_PLACEHOLDER_USV;
        } else {
            placeholder = EMPTY_PLACEHOLDER;
        }
        snprintf(buffer, buffer_size, "%s", placeholder);
        return (int)strlen(buffer);
    }
    
    double d = (double)value;
    double absd = fabs(d);
    char tmp[64];
    
    // 2) Normal range: fixed-point with up to 3 decimal places
    if (absd >= 0.001 && absd < 10000.0) {
        snprintf(tmp, sizeof(tmp), "%.3f", d);
        // Strip trailing zeros and possible trailing dot
        char *pdot = strchr(tmp, '.');
        if (pdot) {
            char *end = tmp + strlen(tmp) - 1;
            while (end > pdot && *end == '0') { *end = '\0'; --end; }
            if (end == pdot) *end = '\0'; // Remove trailing dot if all decimals were zero
        }
    }
    else {
        // 3) Scientific notation for very small or very large values
        snprintf(tmp, sizeof(tmp), "%.3g", d);
    }
    
    // Combine number and unit suffix
    if (withUnit)
        snprintf(buffer, buffer_size, "%s%s", tmp, unit);
    else
        snprintf(buffer, buffer_size, "%s", tmp);
    
    return (int)strlen(buffer);
}

// Called when the application is started
void APP_COUNTER::onEnter(ExitCallback cb) {
    IApplication::onEnter(cb);
    
    auto& syscfg = SystemConf::getInstance();
    cpm_warn_threshold = syscfg.read_conf_warn_threshold();
    cpm_dngr_threshold = syscfg.read_conf_dngr_threshold();
    cpm_hzdr_threshold = syscfg.read_conf_hzdr_threshold();
    use_cpm = syscfg.read_conf_use_cpm();
    en_dosage_alert = syscfg.read_conf_enable_alert();
    en_click = syscfg.read_conf_enable_geiger_click();

    m_ui.setContinousDraw(true);
    pcf8563_get_time(pcf8563_dev, &timeinfo, &tm_valid);
    
    // Initialize and configure widgets
    
    brace.setDrawContentFunction([this]() { braceContent(); });
    brace.setCallback([this](){braceCallback(); });
            currentBracePage = BracePage::MAX;
    targetBracePage = BracePage::MAX;
    anim_brace_y = 0;
    brace_animating = false;
    
    // ICON: Battery
    icon_battery.setSource(image_BAT_75_bits);
    
    // ICON: Sounding
    if (en_click) icon_sounding.setSource(image_SOUND_ON_bits);
    else          icon_sounding.setSource(image_SOUND_OFF_bits);
    
    // ICON: Alarm
    if (en_dosage_alert) icon_alarm.setSource(image_BELL_bits);
    else                 icon_alarm.setSource(image_BELL_OFF_bits);
    
    // Add widgets to focus manager for navigation
    m_ui.addWidgetToFocusManager(&brace);
    m_ui.addWidgetToFocusManager(&histogram);  
    
    timestamp_prev = timestamp_now = m_ui.getCurrentTime();
    
    coroutine_anim.reset();
    coroutine_alarm.reset();
    coroutine_anim.start();
    coroutine_alarm.start();

    m_ui.addCoroutine(&coroutine_anim);
    m_ui.addCoroutine(&coroutine_alarm); // 警报调度
    
    first_time = false;

    // Start hardware-related tasks
    voltage_controller.startTask();
    counter_task_config_t tube_conf = {
        .gpio_num = PIN_PULSE_IN
    };
    start_counter_task(&tube_conf);
}

void APP_COUNTER::braceCallback() 
{
    if (brace_animating) return;  // Ignore input during animation
    
    // Calculate the next page (MAX -> AVG -> MAX)
    targetBracePage = (currentBracePage == BracePage::MAX) ? BracePage::AVG : BracePage::MAX;
    
    // Start upward scrolling animation
    const int PAGE_HEIGHT = 18;
    anim_brace_y = 0;
    m_ui.animate(anim_brace_y, PAGE_HEIGHT, 300, EasingType::EASE_IN_OUT_CUBIC, PROTECTION::PROTECTED);
    brace_animating = true;
}

// Drawing function for the Brace widget content (Max/Avg value with scrolling)
void APP_COUNTER::braceContent() 
{
    U8G2& u8g2 = m_ui.getU8G2();
    u8g2.setFont(u8g2_font_5x7_tr);
    
    const int PAGE_HEIGHT = 18;
    
    auto drawPage = [&](BracePage page, int y_base) {
        const char* label = nullptr;
        float value = 0.0f;
        
        switch (page) {
            case BracePage::MAX:
                label = "Max";
                value = histogram.getMaxValueInHistory();
                break;
            case BracePage::AVG:
                label = "Avg";
                value = histogram.getAverageValueInHistory();
                break;
        }
        
        // 绘制数值
        if (!use_cpm) {
            format_meter_style(print_buffer, sizeof(print_buffer), value, NULL);
            u8g2.drawStr(30, y_base - 4, print_buffer);
            u8g2.drawStr(31, y_base + 3, "uSv/h");
        } else {
            snprintf(print_buffer, sizeof(print_buffer), "%d", (int)value);
            u8g2.drawStr(30, y_base - 4, print_buffer);
            u8g2.drawStr(31, y_base + 3, "CPM");
        }
        
        // 绘制标签框
        u8g2.drawRBox(8, y_base - 8, 20, 10, 2);
        u8g2.setDrawColor(0);
        u8g2.drawStr(11, y_base, label);
        u8g2.setDrawColor(1);
    };
    
    // 根据动画进度绘制页面
    int current_y = 58 + anim_brace_y;  // 当前页面的 Y 坐标
    int next_y = 58 + anim_brace_y - PAGE_HEIGHT;  // 下一个页面的 Y 坐标（从下方进入）
    
    // 绘制当前页面（向上移出）
    if (current_y >= 45 && current_y <= 70) {  // 在可视范围内
        drawPage(currentBracePage, current_y);
    }
    
    // 绘制目标页面（从下方进入）
    if (next_y >= 45 && next_y <= 70) {  // 在可视范围内
        drawPage(targetBracePage, next_y);
    }
}

void APP_COUNTER::onResume()
{
    m_ui.setContinousDraw(true);
}

// Draws the radiation level status text and color bar
void APP_COUNTER::drawLabel(int state) {
    U8G2& u8g2 = m_ui.getU8G2();

    if (!state) {
        u8g2.setDrawColor(1);
    } else {
        u8g2.setDrawColor(2);
        u8g2.drawBox(3, 35, anim_mark_m, 8);
        u8g2.setDrawColor(0);
    }

    if (current_cpm > 0) {
        // Determine status text based on radiation thresholds
        if (current_cpm < cpm_warn_threshold) {
            u8g2.drawStr(5, 42, "SAFE");
            blinker_description_bar.stopOnVisible();
        }
        else if (current_cpm < cpm_dngr_threshold) {
            u8g2.drawStr(5, 42, "WARN");
            blinker_description_bar.set_interval(500);
            blinker_description_bar.start();
        }
        else if (current_cpm < cpm_hzdr_threshold) {
            coroutine_alarm.getContext().localData[0] = 1;

            u8g2.drawStr(5, 42, "DNGR");
            blinker_description_bar.set_interval(300);
            blinker_description_bar.start();
        }
        else {
            coroutine_alarm.getContext().localData[0] = 1;
            u8g2.drawStr(5, 42, "HZDR");
            blinker_description_bar.set_interval(100);
            blinker_description_bar.start();
        }
    } else {
        u8g2.drawStr(5, 42, "WAIT");
        blinker_description_bar.stopOnVisible();
        u8g2.setDrawColor(1);    
        u8g2.drawStr(anim_status_x, 42, "Getting Ready"); 
    }
    
    u8g2.setDrawColor(1);    
}

void APP_COUNTER::draw() 
{
    timestamp_now = m_ui.getCurrentTime();
    
    // Initial setup and animation start
    if (!first_time) 
    {
        m_ui.animate(anim_mark_m, 23, 300, EasingType::EASE_OUT_QUAD, PROTECTION::PROTECTED);
        m_ui.animate(anim_bg, 128, 500, EasingType::EASE_IN_OUT_CUBIC, PROTECTION::PROTECTED);
        m_ui.animate(anim_clock_y, 8, 200, EasingType::EASE_OUT_CUBIC, PROTECTION::PROTECTED);
        
        blinker_description_bar.stopOnVisible();
        // Start the loading animation state machine
        state_timestamp = m_ui.getCurrentTime();
        first_time = true;
    }
    
    if (brace_animating && anim_brace_y >= 17) 
    {  // animation near complete
        // reset state
        currentBracePage = targetBracePage;
        anim_brace_y = 0;
        brace_animating = false;
    }
    
    // --- UI Drawing ---
    U8G2& u8g2 = m_ui.getU8G2();
    
    // Draw background with animation clipping
    u8g2.setClipWindow(0,7,anim_bg,18);
    u8g2.drawXBM(0, 7, 128, 10, image_Background_bits);
    u8g2.setMaxClipWindow();

    // Draw the main radiation reading
    u8g2.setFont(u8g2_font_profont17_tr);
    current_cpm = get_current_cpm();
    
    // Format and draw the value (CPM * conversion coefficient)
    
    if (!use_cpm) {
        if (current_cpm * SystemConf::getInstance().read_conf_tube_convertion_coefficient() < 1000.0f) {
            format_meter_style(print_buffer, sizeof(print_buffer), current_cpm * SystemConf::getInstance().read_conf_tube_convertion_coefficient(), UNIT_USV);
        } else {
            snprintf(print_buffer, sizeof(print_buffer), "%d CPM", (int)(current_cpm * SystemConf::getInstance().read_conf_tube_convertion_coefficient()));
        }
    } else {
        snprintf(print_buffer, sizeof(print_buffer), "%d CPM", (int)current_cpm);
    }
    u8g2.drawStr(3, 31, print_buffer);
    
    blinker_description_bar.update();
    
    // Update histogram data every second if not in startup mode
    if (timestamp_now - timestamp_prev >= 1000)
    {
            timestamp_prev = timestamp_now;
            if (!use_cpm) {
                histogram.addData(current_cpm * SystemConf::getInstance().read_conf_tube_convertion_coefficient());
            } else {
                histogram.addData(current_cpm);
            }
        
        const unsigned char* bat_source = nullptr;
        if (battery_percentage >= 75) {
            bat_source = image_BAT_FULL_bits; // 100% or 75%+
        } else if (battery_percentage >= 50) {
            bat_source = image_BAT_75_bits;   // 50% - 74%
        } else if (battery_percentage >= 25) {
            bat_source = image_BAT_50_bits;   // 25% - 49%
        } else if (battery_percentage > 0) {
            bat_source = image_BAT_25_bits;   // 1% - 24%
        } else {
            bat_source = image_BAT_empty_bits; // 0% or empty
        }
        icon_battery.setSource(bat_source);
        
        pcf8563_get_time(pcf8563_dev, &timeinfo, &tm_valid);
    }
    
    // Draw status label if the blinker is visible (for blinking effect)

    u8g2.setFont(u8g2_font_5x7_tr);
    if (current_cpm > 0) {
        // Determine status text based on radiation thresholds
        if (current_cpm < cpm_warn_threshold) {
            u8g2.setClipWindow(29,36,128,42);
            u8g2.drawStr(anim_status_x, 42, "Low Radiation");
            u8g2.setMaxClipWindow();
        }
        else if (current_cpm < cpm_dngr_threshold) {
            u8g2.setClipWindow(29,36,128,42);
            u8g2.drawStr(anim_status_x, 42, "RISING LEVEL");
            u8g2.setMaxClipWindow();
        }
        else if (current_cpm < cpm_hzdr_threshold) {
            u8g2.setClipWindow(29,36,128,42);
            u8g2.drawStr(anim_status_x, 42, "UNSAFE DOSE");
            u8g2.setMaxClipWindow();
        }
        else {
            u8g2.setClipWindow(29,36,128,42);
            u8g2.drawStr(anim_status_x, 42, "SEVERE THREAT");
            u8g2.setMaxClipWindow();
        }
    } 

    drawLabel(blinker_description_bar.is_visible());

    // Draw voltage display
    u8g2.setFont(u8g2_font_5x7_tr);
    uint16_t volt = voltage_controller.getVoltage();
    snprintf(print_buffer, sizeof(print_buffer), "%dV", volt);
    u8g2.drawStr(105, 42, print_buffer);
    
    // Highlight voltage box if target voltage is reached
    if (abs(volt - voltage_controller.getTargetVolt()) < 10) {
        u8g2.setDrawColor(2); // Inverse/toggle draw color
        u8g2.drawBox(104, 35, 21, 8);
    }
    u8g2.setDrawColor(1); // Restore draw color

    // Draw clock
    snprintf(print_buffer, sizeof(print_buffer), "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
    u8g2.drawStr(97, anim_clock_y, print_buffer);
    
    // Draw widgets (icons, brace, histogram)
    icon_sounding.draw();
    icon_alarm.draw();
    icon_battery.draw();
    brace.draw();

    if (pulse_marker) 
    {
        pulse_marker = false;
        u8g2.drawFilledEllipse(122, 30, 2, 2);
    } 
    else 
    {
        u8g2.drawEllipse(122, 30, 2, 2);
    }
    u8g2.drawStr(101, 26, "PULSE");

    // If histogram is expanded (e.g., in stats view), draw stats overlay
    if (histogram.isExpanded()) {
        u8g2.clearBuffer(); // Clear screen for expanded view

        u8g2.setDrawColor(1);
        u8g2.drawBox(0, 31, 48, 8);
        u8g2.drawBox(0, 0, 48, 8);

        u8g2.setFont(u8g2_font_profont11_tr);

        float history_val_max = histogram.getMaxValueInHistory();

        if (!use_cpm) {
            if (history_val_max < 1000.0f) 
            {
                snprintf(print_buffer, sizeof(print_buffer), "%.3g", history_val_max);
            } 
            else 
            {
                snprintf(print_buffer, sizeof(print_buffer), "%d", (int)history_val_max);
            }
            u8g2.drawStr(12, 28, "uSv/h");
            u8g2.drawStr(12, 60, "uSv/h");
        } else {
            snprintf(print_buffer, sizeof(print_buffer), "%d", (int)history_val_max);
            u8g2.drawStr(16, 28, "CPM");
            u8g2.drawStr(16, 60, "CPM");
        }

        int16_t x_max = TextAlignHelper::calcCenteredX(u8g2.getU8g2(), 0, 52, print_buffer);
        u8g2.drawStr(x_max, 18, print_buffer); // Maximum value

        float history_val_avg = histogram.getAverageValueInHistory();
        if (!use_cpm){
            if (history_val_avg < 1000.0f) {
                snprintf(print_buffer, sizeof(print_buffer), "%.3g", history_val_avg);
            } else {
                snprintf(print_buffer, sizeof(print_buffer), "%d", (int)history_val_avg);
            }
        } else {
            snprintf(print_buffer, sizeof(print_buffer), "%d", (int)history_val_avg);
        }

        int16_t x_avg = TextAlignHelper::calcCenteredX(u8g2.getU8g2(), 0, 52, print_buffer);
        u8g2.drawStr(x_avg, 49, print_buffer); // Average value

        u8g2.setDrawColor(0);
        u8g2.setFont(u8g2_font_5x7_tr);
        u8g2.drawStr(7, 7, "MAXIMUM");
        u8g2.drawStr(7, 38, "AVERAGE");

        u8g2.setDrawColor(1);
    }
    histogram.draw();
}

bool APP_COUNTER::handleInput(InputEvent event) 
{
    // Handle input for focus navigation
    if (event == InputEvent::BACK) {
        requestExit();
    }
    return true;
}

// Called when the application is exited
void APP_COUNTER::onExit() 
{
    voltage_controller.stop();
    stop_counter_task();
    
    m_ui.removeCoroutine(&coroutine_alarm);
    m_ui.removeCoroutine(&coroutine_anim);

    m_ui.clearAllAnimations();
    m_ui.setContinousDraw(false);
    m_ui.markFading();
}

// Application registry entry
AppItem counter_app
{
    .title = "盖革计数器",
    .bitmap = image_counter_bits,
    
    // Factory function to create an instance of APP_COUNTER
    .createApp = [](PixelUI& ui, void* param) -> std::unique_ptr<IApplication> 
    { 
        return std::make_unique<APP_COUNTER>(ui, param); 
    },
};
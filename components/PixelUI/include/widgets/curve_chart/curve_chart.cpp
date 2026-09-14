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

#include "widgets/curve_chart/curve_chart.h"
#include "PixelUI.h"
#include "calc/TextAlignHelper/TextAlignHelper.h"
#include <etl/algorithm.h>
#include <etl/limits.h>

/**
 * @brief Constructor for CurveChart widget.
 * @param ui Reference to the PixelUI instance.
 * @param pos_x X coordinate of the widget's top-left corner (new anchor).
 * @param pos_y Y coordinate of the widget's top-left corner (new anchor).
 */
CurveChart::CurveChart(
    PixelUI& ui,
    uint16_t pos_x,
    uint16_t pos_y,
    uint16_t size_w,
    uint16_t size_h,
    ChartSeries& series,
    uint16_t size_w_exp,
    uint16_t size_h_exp,
    EXPAND_BASE base,
    const char* label) :
    m_ui(ui),
    m_series(series),
    pos_x_(pos_x), 
    pos_y_(pos_y),
    size_w_(size_w),
    size_h_(size_h),
    exp_w(size_w_exp),
    exp_h(size_h_exp),
    base_(base),
    m_label(label)
{
    setFocusInsets({1, 1, 0, 0});
    setWidgetBounds({pos_x_, pos_y_, size_w_, size_h_});
    // pos_x_ and pos_y_ now represent the top-left anchor point.
    int32_t start_anim_x = (size_w_ / 2);
    int32_t start_anim_y = (size_h_ / 2);

    anim_w = 0; 
    anim_h = 0;

    anim_x = start_anim_x;
    anim_y = start_anim_y;
}

/** @brief CurveChart::onLoad. */
void CurveChart::onLoad() {
    onLoad(LoadTransition::Animated);
}

/** @brief CurveChart::onLoad. */
void CurveChart::onLoad(LoadTransition transition) {
    if (transition == LoadTransition::Immediate) {
        anim_w = size_w_;
        anim_h = size_h_;
        anim_x = 0;
        anim_y = 0;
        m_ui.markDirty();
        return;
    }

    int32_t start_anim_x = (size_w_ / 2);
    int32_t start_anim_y = (size_h_ / 2);

    anim_w = 0; 
    anim_h = 0;

    anim_x = start_anim_x;
    anim_y = start_anim_y;

    if (!m_ui.animate(anim_w, size_w_, 550, EasingType::EASE_OUT_QUAD, PROTECTION::PROTECTED)) {
        anim_w = size_w_;
    }
    if (!m_ui.animate(anim_h, size_h_, 600, EasingType::EASE_OUT_QUAD, PROTECTION::PROTECTED)) {
        anim_h = size_h_;
    }

    if (!m_ui.animate(anim_x, 0, 550, EasingType::EASE_OUT_QUAD, PROTECTION::PROTECTED)) {
        anim_x = 0;
    }
    if (!m_ui.animate(anim_y, 0, 600, EasingType::EASE_OUT_QUAD, PROTECTION::PROTECTED)) {
        anim_y = 0;
    }
    m_ui.markDirty();
}

/** @brief CurveChart::onOffload. */
void CurveChart::onOffload() {

}

/** @brief CurveChart::handleEvent. */
bool CurveChart::handleEvent(InputEvent event) {
    if (event == InputEvent::SELECT) {
        is_expanded = false;
        contractWidget();
        return true;
    }
    return false;
}

/** @brief CurveChart::onSelect. */
bool CurveChart::onSelect(){
    m_ui.clearUnprotectedAnimations();
    if (!is_expanded) {
        // Expand animation
        expandWidget();
        is_expanded = true;
    } else {
        // Shrink animation
        contractWidget();
        is_expanded = false;
    }
    return true;
}

/** @brief CurveChart::expandWidget. */
void CurveChart::expandWidget() {
    int32_t target_x, target_y;
    calculateExpandPosition(target_x, target_y);
    
    // Animate to expanded size and position
    m_ui.animate(anim_w, exp_w, 400, EasingType::EASE_OUT_QUAD);
    m_ui.animate(anim_h, exp_h, 350, EasingType::EASE_OUT_QUAD);
    // Animate the Top-Left position to the calculated target offsets
    m_ui.animate(anim_x, target_x, 400, EasingType::EASE_OUT_QUAD);
    m_ui.animate(anim_y, target_y, 350, EasingType::EASE_OUT_QUAD);
}

/** @brief CurveChart::contractWidget. */
void CurveChart::contractWidget() {
    // Animate back to original size and position
    m_ui.animate(anim_w, size_w_, 350, EasingType::EASE_OUT_QUAD, PROTECTION::PROTECTED);
    m_ui.animate(anim_h, size_h_, 400, EasingType::EASE_OUT_QUAD, PROTECTION::PROTECTED);
    // Animate the Top-Left position back to (0, 0) offset
    m_ui.animate(anim_x, 0, 350, EasingType::EASE_OUT_QUAD, PROTECTION::PROTECTED);
    m_ui.animate(anim_y, 0, 400, EasingType::EASE_OUT_QUAD, PROTECTION::PROTECTED);
}

/**
 * @brief Calculate target animation offsets for expansion based on the base corner.
 * The offsets (anim_x, anim_y) are applied to the top-left corner (pos_x_, pos_y_).
 * @param target_x Output target x offset.
 * @param target_y Output target y offset.
 */
void CurveChart::calculateExpandPosition(int32_t& target_x, int32_t& target_y) {
    int32_t width_diff = exp_w - size_w_;
    int32_t height_diff = exp_h - size_h_;
    
    switch (base_) {
        case EXPAND_BASE::TOP_LEFT:
            // Top-Left corner (pos_x_ + anim_x) should stay fixed at pos_x_
            target_x = 0;
            target_y = 0;
            break;
            
        case EXPAND_BASE::TOP_RIGHT:
            // Top-Right corner should stay fixed: tl_x + width = fixed
            target_x = -width_diff;
            target_y = 0;
            break;
            
        case EXPAND_BASE::BOTTOM_LEFT:
            // Bottom-Left corner should stay fixed: tl_y + height = fixed
            target_x = 0;
            target_y = -height_diff;
            break;
            
        case EXPAND_BASE::BOTTOM_RIGHT:
            // Bottom-Right corner should stay fixed
            target_x = -width_diff;
            target_y = -height_diff;
            break;
    }
}

/** @brief CurveChart::display. */
Canvas& CurveChart::display() { return m_ui.getCanvas(); }

/** @brief CurveChart::drawSelf. */
void CurveChart::drawSelf(const WidgetRenderContext& context) {
    Canvas& u8g2 = m_ui.getCanvas();
    // tl_x and tl_y are the animated top-left corner coordinates
    int tl_x = context.originX + pos_x_ + anim_x;
    int tl_y = context.originY + pos_y_ + anim_y;
    int current_w = anim_w;
    int current_h = anim_h;
    
    // Clear drawing area (background box)
    u8g2.setDrawColor(0);
    // Draw box relative to top-left corner
    u8g2.drawBox(tl_x + 2, tl_y, current_w - 4, current_h);
    u8g2.setDrawColor(1);
    
    // --- Draw border corners ---
    // Top-Left: (tl_x, tl_y)
    u8g2.drawLine(tl_x, tl_y, tl_x + 4, tl_y);
    u8g2.drawLine(tl_x, tl_y, tl_x, tl_y + 4);
    
    // Top-Right: (tl_x + current_w, tl_y)
    u8g2.drawLine(tl_x + current_w, tl_y, tl_x + current_w - 4, tl_y);
    u8g2.drawLine(tl_x + current_w, tl_y, tl_x + current_w, tl_y + 4); 

    // Bottom-Left: (tl_x, tl_y + current_h)
    u8g2.drawLine(tl_x, tl_y + current_h, tl_x + 4, tl_y + current_h);
    u8g2.drawLine(tl_x, tl_y + current_h, tl_x, tl_y + current_h - 4);
    
    // Bottom-Right: (tl_x + current_w, tl_y + current_h)
    u8g2.drawLine(tl_x + current_w, tl_y + current_h, tl_x + current_w - 4, tl_y + current_h);
    u8g2.drawLine(tl_x + current_w, tl_y + current_h, tl_x + current_w, tl_y + current_h - 4);

    // Draw border - using animated coordinates (thickness of 2)
    // Left vertical line
    u8g2.drawLine(tl_x, tl_y, tl_x, tl_y + current_h);
    u8g2.drawLine(tl_x + 1, tl_y, tl_x + 1, tl_y + current_h);
    // Right vertical line
    u8g2.drawLine(tl_x + current_w, tl_y, tl_x + current_w, tl_y + current_h);
    u8g2.drawLine(tl_x + current_w - 1, tl_y, tl_x + current_w - 1, tl_y + current_h);
    
    // Draw curve data from internal buffer, passing top-left coordinates, current width, and height
    drawCurveData(tl_x, tl_y, current_w, current_h, u8g2);
    
    // Draw label if provided (positioned in the top-right area)
    if (m_label != nullptr) {
        u8g2.setFont(PIXELUI_FONT_CHART);
        
        // Define label area in top-right corner (matching original offset logic)
        Rect label_area = {
            static_cast<int16_t>(tl_x + current_w - 24 - 2),  // Right area
            static_cast<int16_t>(tl_y + 2),               // Top margin
            24,                                            // Width for label area
            6                                              // Height matches font height
        };
        
        // Calculate text position using TextAlignHelper
        // Use Top alignment for Y to position near top, Right alignment for X
        TextPos text_pos = TextAlignHelper::calcTextPos(
            u8g2.rawDisplay().getU8g2(),
            label_area,
            m_label,
            TextAlignX::Right,
            TextAlignY::Top
        );
        
        u8g2.drawStr(text_pos.x, text_pos.y, m_label);
    }
}

/**
 * @brief Draws the curve data by connecting points with lines.
 * Data is plotted from right (newest) to left (oldest).
 * Uses visible window data for auto-scaling.
 * @param tl_x Widget top-left x coordinate.
 * @param tl_y Widget top-left y coordinate.
 * @param width Current width of the widget.
 * @param height Current height of the widget.
 * @param u8g2 Reference to U8G2 for drawing.
 */
void CurveChart::drawCurveData(int tl_x, int tl_y, int width, int height, Canvas& u8g2) {
    if (m_series.empty()) return;
    
    // Determine how many horizontal steps to draw
    // Subtract 3 to account for 2px borders (width - 3 = drawable columns from right border to left border inclusive)
    const int points_to_draw =
        etl::min(width - 3, static_cast<int>(m_series.size()));
    if (points_to_draw <= 1) { 
        // Handle single point case
        if (points_to_draw == 1) {
            const int chart_h = height - 4;
            const int y_bottom = tl_y + height - 2;
            int current_y = y_bottom - (chart_h / 2); // Center single point
            int current_x = tl_x + width - 2;
            
            u8g2.drawPixel(current_x, current_y);
        }
        return;
    }
    
    // Chart dimensions, excluding the 2px border margin on all sides
    const int chart_h = height - 4;
    
    // Calculate visible window min/max (with caching)
    if (m_cached_series_revision != m_series.revision() ||
        m_cached_visible_width != points_to_draw) {
        m_cached_visible_max = etl::numeric_limits<ChartSample>::min();
        m_cached_visible_min = etl::numeric_limits<ChartSample>::max();
        
        for (int i = 0; i < points_to_draw; ++i) {
            const ChartSample sample =
                m_series.sampleFromNewest(static_cast<size_t>(i));
            m_cached_visible_max = etl::max(m_cached_visible_max, sample);
            m_cached_visible_min = etl::min(m_cached_visible_min, sample);
        }
        
        m_cached_visible_width = points_to_draw;
        m_cached_series_revision = m_series.revision();
    }
    
    // Use cached visible window range for scaling
    const ChartSample visible_min = m_cached_visible_min;
    const ChartSample visible_max = m_cached_visible_max;
    const int64_t range = static_cast<int64_t>(visible_max) - visible_min;
    
    // Drawing area Y boundary (Bottom of the chart area)
    const int y_bottom = tl_y + height - 2;
    const int y_top = tl_y + 2;

    int prev_x = -1;
    int prev_y = -1;
    
    // Loop from newest data (i=0, rightmost pixel) backwards to the oldest data (i=points_to_draw-1, leftmost pixel)
    for (int i = 0; i < points_to_draw; ++i) {
        
        const ChartSample sample =
            m_series.sampleFromNewest(static_cast<size_t>(i));
        
        // 2. Calculate coordinates
        // X position: Right Edge of chart area (tl_x + width - 2), move left by 'i' pixels
        int current_x = tl_x + width - 2 - i; 
        
        // Y position: Normalize value relative to visible_min, then scale, and position relative to y_bottom
        int y_offset = chart_h / 2;
        if (range > 0) {
            const int64_t normalized_value =
                static_cast<int64_t>(sample) - visible_min;
            y_offset = static_cast<int>((normalized_value * chart_h) / range);
        }
        
        // Y coordinate (0 is top, y_bottom is bottom)
        int current_y = y_bottom - y_offset; 
        
        // Ensure Y stays within the vertical bounds of the chart area 
        current_y = etl::max(y_top, current_y);
        current_y = etl::min(y_bottom, current_y);

        // 3. Draw line segment
        if (prev_x != -1) {
            // Draw line from the previously plotted point (i-1, newer, on the right) 
            // to the current point (i, older, on the left)
            u8g2.drawLine(current_x, current_y, prev_x, prev_y);
        }
        
        // 4. Update previous coordinates for the next iteration (which is the next older point)
        prev_x = current_x;
        prev_y = current_y;
    }
}

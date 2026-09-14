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
#include "../ChartSeries.h"
#include <etl/limits.h>

class CurveChart : public IWidget {
public:
    template <size_t N, uint16_t ExpandedWidth, uint16_t ExpandedHeight>
/** @brief CurveChart. */
    CurveChart(
        PixelUI& ui,
        uint16_t pos_x,
        uint16_t pos_y,
        uint16_t size_w,
        uint16_t size_h,
        StaticChartSeries<N>& series,
        ChartExpandSize<ExpandedWidth, ExpandedHeight>,
        EXPAND_BASE base,
        const char* label = nullptr)
        : CurveChart(
              ui,
              pos_x,
              pos_y,
              size_w,
              size_h,
              series,
              ExpandedWidth,
              ExpandedHeight,
              base,
              label) {
        static_assert(N == ExpandedWidth,
                      "CurveChart series capacity must equal the expanded chart width");
        static_assert(N <= static_cast<size_t>(etl::numeric_limits<int>::max()),
                      "CurveChart buffer capacity exceeds supported index range");
    }
/** @brief ~CurveChart. */
    ~CurveChart() = default;

/** @brief onLoad. */
    void onLoad() override;
/** @brief onLoad. */
    void onLoad(LoadTransition transition);
/** @brief onOffload. */
    void onOffload() override;
/** @brief onSelect. */
    bool onSelect() override;
/** @brief handleEvent. */
    bool handleEvent(InputEvent event) override;

/** @brief setSize. */
    void setSize(uint16_t mar_w, uint16_t mar_h) { 
        size_w_ = mar_w; size_h_ = mar_h; 
        setWidgetBounds({pos_x_, pos_y_, size_w_, size_h_});
    }
/** @brief setPosition. */
    void setPosition(uint16_t pos_x, uint16_t pos_y) { 
        pos_x_ = pos_x; pos_y_ = pos_y; 
        setWidgetBounds({pos_x_, pos_y_, size_w_, size_h_});
    }

/** @brief isExpanded. */
    bool isExpanded() const { return is_expanded; }

private:
    PixelUI& m_ui;
    ChartSeries& m_series;

    uint16_t pos_x_ = 0, pos_y_ = 0;
    uint16_t size_w_ = 0, size_h_ = 0;
    uint16_t exp_w = 0, exp_h = 0;
    EXPAND_BASE base_;

    // Cache for visible window statistics
    ChartSample m_cached_visible_max = 0;
    ChartSample m_cached_visible_min = 0;
    int m_cached_visible_width = 0;
    uint32_t m_cached_series_revision = 0;

    // animation related variables:
    int32_t anim_w = 0;
    int32_t anim_h = 0;
    int32_t anim_x = 0;
    int32_t anim_y = 0;

    const char* m_label = nullptr;

/** @brief CurveChart. */
    CurveChart(
        PixelUI& ui,
        uint16_t pos_x,
        uint16_t pos_y,
        uint16_t size_w,
        uint16_t size_h,
        ChartSeries& series,
        uint16_t size_w_exp,
        uint16_t size_h_exp,
        EXPAND_BASE base,
        const char* label);

/** @brief expandWidget. */
    void expandWidget();
/** @brief contractWidget. */
    void contractWidget();
/** @brief calculateExpandPosition. */
    void calculateExpandPosition(int32_t& target_x, int32_t& target_y);
/** @brief drawCurveData. */
    void drawCurveData(int tl_x, int tl_y, int width, int height, Canvas& u8g2);
/** @brief drawSelf. */
    void drawSelf(const WidgetRenderContext& context) override;
/** @brief display. */
    Canvas& display() override;

    bool is_expanded = false;
};

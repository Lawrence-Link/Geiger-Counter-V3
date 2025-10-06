/*
 * Copyright (C) 2025 Lawrence Link
 */

#include "core/app/IApplication.h"
#include "core/app/app_system.h"
#include "core/coroutine/Coroutine.h"

extern bool showing_charging_anim;
extern int battery_percentage;

class Charge : public IApplication {
private:
    PixelUI& m_ui;
    // animation varibles
    int32_t lightIconSize = 0;
    int32_t batteryPercent_anim = 0;
    int32_t ringPercent = 0;
    int32_t lightningOffsetX = 0;
    int batteryPercent = 50;
    bool exit_flag = false;
    std::shared_ptr<Coroutine> animationCoroutine_;

    void animation_coroutine_body(CoroutineContext& ctx, PixelUI& ui) {
        CORO_BEGIN(ctx);
        
        ui.animate(lightIconSize, 7, 400, EasingType::EASE_IN_CUBIC, PROTECTION::PROTECTED);
        ui.animate(ringPercent, batteryPercent, 600, EasingType::EASE_OUT_CUBIC);
        CORO_DELAY(ctx, ui, 1200, 100);

        ui.animate(ringPercent, 0, 600, EasingType::EASE_OUT_CUBIC);
        CORO_DELAY(ctx, ui, 900, 200);   

        ui.animate(lightningOffsetX, -10, 600, EasingType::EASE_OUT_CUBIC);
        ui.animate(batteryPercent_anim, batteryPercent, 600, EasingType::EASE_OUT_CUBIC);
        CORO_DELAY(ctx, ui, 2200, 300); 
        exit_flag = true;
        CORO_END(ctx);
        
    }

public:
    Charge(PixelUI& ui) : m_ui(ui) {}

    void draw() override {
        m_ui.markDirty();
        U8G2& display = m_ui.getU8G2();
        int centerX = 64 + lightningOffsetX;
        int centerY = 32;

        drawChargingLightning(lightIconSize, centerX, centerY);
        drawBatteryRing(display, 64, 32, 15, 2, ringPercent);

        if (batteryPercent_anim > 0) {
            char buf[12];
            sprintf(buf, "%d%%", (int)batteryPercent_anim);
            display.setFont(u8g2_font_6x10_tf);
            display.drawStr(65, 36, buf);
        }

        if (exit_flag) requestExit();
    }

    bool handleInput(InputEvent event) override {
        if (animationCoroutine_) {
            m_ui.removeCoroutine(animationCoroutine_);
            animationCoroutine_.reset();
        }
        requestExit();
        return true;
    }

    void onEnter(ExitCallback cb) override {
        IApplication::onEnter(cb);

        lightIconSize = 0;
        batteryPercent_anim = 0;
        ringPercent = 0;
        lightningOffsetX = 0;
        batteryPercent = battery_percentage;
        
        animationCoroutine_ = std::make_shared<Coroutine>( // add coroutine for the *Amazing* setup animation
            std::bind(&Charge::animation_coroutine_body, this, std::placeholders::_1, std::placeholders::_2),
            m_ui
        );
        // register coroutine
        m_ui.addCoroutine(animationCoroutine_);

        m_ui.setContinousDraw(true);
        m_ui.markDirty();
    }

    void onExit() override {
        m_ui.setContinousDraw(false);
        showing_charging_anim = false;
        m_ui.markFading();
        
        // remove the coroutine instance when exit
        if (animationCoroutine_) {
            m_ui.removeCoroutine(animationCoroutine_);
            animationCoroutine_.reset();
        }
    }

private:
    // ---------------- Lightning drawing ----------------
    void drawChargingLightning(int size, int centerX, int centerY) {
        U8G2 &g = m_ui.getU8G2();
        int p1x = centerX + size * 0.4; int p1y = centerY - size * 0.6;
        int p2x = centerX - size * 0.1; int p2y = centerY - size * 0.1;
        int p3x = centerX + size * 0.35; int p3y = centerY - size * 0.1;
        int p4x = centerX - size * 0.35; int p4y = centerY + size * 0.1;
        int p5x = centerX + size * 0.1; int p5y = centerY + size * 0.1;
        int p6x = centerX - size * 0.4; int p6y = centerY + size * 0.6;

        g.drawLine(p1x, p1y, p4x, p4y);
        g.drawLine(p4x, p4y, p5x, p5y);
        g.drawLine(p5x, p5y, p6x, p6y);
        g.drawLine(p1x, p1y, p2x, p2y);
        g.drawLine(p2x, p2y, p3x, p3y);
        g.drawLine(p3x, p3y, p6x, p6y);
        g.drawLine(p1x, p1y, p6x, p6y);
    }

    // ---------------- Ring drawing ----------------
    void drawBatteryRing(U8G2 &u8g2, int x0, int y0, int radius, int thickness, int percent) {
        if (thickness < 1) thickness = 1;
        if (radius <= 0) return;
        if (percent <= 0) return;

        const uint8_t TOP = 64; // 12 o'clock
        uint8_t len = (uint8_t)((percent * 256UL) / 100UL);
        uint8_t arcStart = (uint8_t)(TOP - len);
        uint8_t arcEnd   = TOP;

        for (int w = 0; w < thickness; ++w) {
            int r = radius - w;
            if (r > 0) u8g2.drawArc(x0, y0, r, arcStart, arcEnd);
        }
    }
};

// ---------------- Application registration ----------------
AppItem charge_app{
    .title = nullptr,
    .bitmap = nullptr,
    
    .createApp = [](PixelUI& ui) -> std::unique_ptr<IApplication> { 
        return std::make_unique<Charge>(ui); 
    },
};

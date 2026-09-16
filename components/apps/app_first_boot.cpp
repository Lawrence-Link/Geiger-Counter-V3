/*
 * Copyright (C) 2025 Lawrence Link
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "PixelUI.h"
#include "app_ui_utils/hello_animation.h"
#include "core/app/IApplication.h"
#include "core/app/app_system.h"
#include "system_nvs_varibles.h"

namespace {
constexpr uint32_t kPostHelloDelayMs = 600U;
constexpr uint32_t kDeviceEnterMs = 700U;
constexpr uint32_t kPromptEnterMs = 450U;
constexpr uint32_t kPromptNudgeMs = 320U;
constexpr uint32_t kPromptPauseMs = 260U;
constexpr int32_t kTransitionOne = 1024;
constexpr int32_t kScreenWidth = 120;
constexpr int32_t kScreenHeight = 64;
constexpr HelloAnimation::Bounds kHelloStartBounds{22, 14, 84, 37};
constexpr HelloAnimation::Bounds kHelloDeviceBounds{20, 23, 41, 18};
constexpr int32_t kDeviceStartWidth = 150;
constexpr int32_t kDeviceWidth = 115;
constexpr int32_t kPromptStartX = 128;
constexpr int32_t kPromptX = 80;
constexpr int32_t kPromptY = 19;
constexpr int32_t kPromptWidth = 40;
constexpr int32_t kPromptHeight = 26;

int32_t interpolate(int32_t from, int32_t to, int32_t progress) {
    return from + (to - from) * progress / kTransitionOne;
}
}  // namespace

class AppFirstBoot : public IApplication {
private:
    PixelUI& ui_;
    Coroutine sequence_;
    uint32_t helloStart_ = 0U;
    int32_t deviceProgress_ = 0;
    int32_t promptX_ = kPromptStartX;
    int32_t promptNudge_ = 0;
    bool deviceFrameVisible_ = false;
    bool promptVisible_ = false;

    void startSequence() {
        deviceProgress_ = 0;
        promptX_ = kPromptStartX;
        promptNudge_ = 0;
        deviceFrameVisible_ = false;
        promptVisible_ = false;
        helloStart_ = ui_.getCurrentTime();
        sequence_.reset();
        sequence_.start();
        ui_.addCoroutine(&sequence_);
    }

public:
    AppFirstBoot(PixelUI& ui, void*)
        : ui_(ui),
          sequence_([this](CoroutineContext& ctx) {
              CORO_BEGIN(ctx);
              CORO_DELAY(ctx, ui_, HelloAnimation::durationMs(), 100);
              CORO_DELAY(ctx, ui_, kPostHelloDelayMs, 200);

              deviceFrameVisible_ = true;
              if (!ui_.animate(deviceProgress_, kTransitionOne,
                               kDeviceEnterMs,
                               EasingType::EASE_IN_OUT_CUBIC,
                               PROTECTION::PROTECTED)) {
                  deviceProgress_ = kTransitionOne;
              }
              CORO_WAIT_ANIMATION(ctx, ui_, 250);

              promptVisible_ = true;
              if (!ui_.animate(promptX_, kPromptX, kPromptEnterMs,
                               EasingType::EASE_OUT_CUBIC,
                               PROTECTION::PROTECTED)) {
                  promptX_ = kPromptX;
              }
              CORO_WAIT_ANIMATION(ctx, ui_, 300);

              while (true) {
                  if (!ui_.animate(promptNudge_, -3, kPromptNudgeMs,
                                   EasingType::EASE_IN_OUT_QUAD,
                                   PROTECTION::PROTECTED)) {
                      promptNudge_ = -3;
                  }
                  CORO_WAIT_ANIMATION(ctx, ui_, 400);

                  if (!ui_.animate(promptNudge_, 0, kPromptNudgeMs,
                                   EasingType::EASE_IN_OUT_QUAD,
                                   PROTECTION::PROTECTED)) {
                      promptNudge_ = 0;
                  }
                  CORO_WAIT_ANIMATION(ctx, ui_, 500);
                  CORO_DELAY(ctx, ui_, kPromptPauseMs, 600);
              }
              CORO_END(ctx);
          }) {}

    void draw() override {
        const uint32_t elapsed = ui_.getCurrentTime() - helloStart_;
        const HelloAnimation::Bounds helloBounds{
            interpolate(kHelloStartBounds.x, kHelloDeviceBounds.x,
                        deviceProgress_),
            interpolate(kHelloStartBounds.y, kHelloDeviceBounds.y,
                        deviceProgress_),
            interpolate(kHelloStartBounds.width, kHelloDeviceBounds.width,
                        deviceProgress_),
            interpolate(kHelloStartBounds.height, kHelloDeviceBounds.height,
                        deviceProgress_),
        };

        Canvas& canvas = ui_.getCanvas();
        HelloAnimation::draw(canvas, elapsed, helloBounds);

        if (deviceFrameVisible_) {
            const int32_t width = interpolate(
                kDeviceStartWidth, kDeviceWidth, deviceProgress_);
            const int32_t height = width / 2;
            const int32_t x = (kScreenWidth - width) / 2;
            const int32_t y = (kScreenHeight - height) / 2;

            canvas.setDrawColor(1);
            canvas.drawRFrame(x, y, width, height, 3);
        }

        if (!promptVisible_) return;

        const int32_t x = promptX_ + promptNudge_;
        canvas.setDrawColor(1);

        // canvas.drawFilledEllipse(104, 32, 7, 12)
        canvas.setClipWindow(116, 15, 128, 64);
        canvas.drawFilledEllipse(x+36, 32, 7, 12);
        canvas.setMaxClipWindow();

        canvas.setFont(PIXELUI_FONT_SMALL);
        canvas.drawStr(x-5, kPromptY + 16, "<- PUSH");
    }

    bool handleInput(InputEvent event) override {
        if (event == InputEvent::SELECT && promptVisible_) {
            SystemConf& config = SystemConf::getInstance();
            config.set_conf_first_boot(false);
            config.save_conf_to_nvs();
            requestExit();
        }
        return true;
    }

    void onEnter(ExitCallback callback) override {
        IApplication::onEnter(callback);
        ui_.setContinousDraw(true);
        startSequence();
        ui_.markDirty();
    }

    void onExit() override {
        ui_.removeCoroutine(&sequence_);
        ui_.clearAllAnimations();
        ui_.setContinousDraw(false);
        ui_.markFading();
    }
};

AppItem first_boot_app{
    .title = nullptr,
    .bitmap = nullptr,
    .factory = ApplicationFactory::make<AppFirstBoot>(),
};

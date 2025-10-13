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
#include "core/app/app_system.h"
#include <memory>
#include <cstring>
#include <cstdlib>
#include <inttypes.h>
#include "system_nvs_varibles.h"
#include "esp_log.h"

// Game constants
#define UPT_MOVE_NONE 0
#define UPT_MOVE_UP 1
#define UPT_MOVE_DOWN 2
#define CAR_COUNT 3
#define CAR_WIDTH 12
#define CAR_LENGTH 15
#define ROAD_SPEED 6
#define FRAME_WIDTH 128
#define FRAME_HEIGHT 64

static int32_t highscore;

// Car sprites (converted from PROGMEM)
__attribute__((aligned(4)))
static const unsigned char carImg[] = {
0x1c,0x38,0x3e,0x3c,0xe2,0x63,0x7e,0x46,0x47,0x5d,0x46,0x65,0x46,0x65,0x47,0x5d,0x7e,0x46,0xe2,0x63,0x3e,0x3c,0x1c,0x38
};

// Road markings
__attribute__((aligned(4)))
static const unsigned char roadMarking[] = {
    0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
};

// Lives icon (simplified heart shape)
__attribute__((aligned(4)))
static const unsigned char livesImg[] = {
    0x36,0x7f,0x7f,0x3e,0x1c,0x08,0x00,0x00
};

// App icon for menu
__attribute__((aligned(4)))
static const unsigned char image_racing_bits[] = {
    0xf0,0xff,0x0f,0xfc,0xff,0x3f,0xfe,0xff,0x7f,0xfe,0xff,0x7f,0xff,0xff,0xff,0xff,0xff,0xff,0x3f,0x7e,0xfc,0x1f,0x3c,0xfc,0xdf,0xc1,0xd9,0x1f,0x98,0xdb,0x8a,0x2b,0xda,0x9f,0xab,0xd9,0x9f,0xab,0xd9,0x8a,0x2b,0xfa,0x1f,0x98,0xdb,0xdf,0xc1,0xf9,0x1f,0x3c,0xfc,0x3f,0x7e,0xfc,0xff,0xff,0xff,0xff,0xff,0xff,0xfe,0xff,0x7f,0xfe,0xff,0x7f,0xfc,0xff,0x3f,0xf0,0xff,0x0f
};

// Game structures
typedef struct {
    int16_t x;  
    uint8_t y;
    uint8_t speed;
    bool active;  
} s_otherCars;

typedef struct {
    bool hit;
    uint8_t lane;
    int16_t y;  
} s_myCar;

class RacingGame : public IApplication {
private:
    PixelUI& m_ui;
    
    // Game state
    uint32_t score;
    uint8_t uptMove;
    int8_t lives;
    s_otherCars cars[CAR_COUNT];
    s_myCar myCar;
    uint32_t hitTime;
    bool newHighscore;
    int8_t quakeY;
    
    // Road markings animation
    uint8_t dotX[3];
    
    // Buffer for string formatting
    char print_buffer[32];
    
    uint32_t timestamp_prev;
    uint32_t timestamp_now;
    uint32_t last_update_time;

public:
    RacingGame(PixelUI& ui) : m_ui(ui) {
        // Set random seed
        srand(m_ui.getCurrentTime());
    }

    void resetGame() {
        // Reset game state
        if (newHighscore) {
            SystemConf::getInstance().set_rec_highscore_car_dodge(highscore);
            SystemConf::getInstance().save_conf_to_nvs();
        }
        // reload from nvs
        highscore = SystemConf::getInstance().read_rec_highscore_car_dodge();
        score = 0;
        uptMove = UPT_MOVE_NONE;
        lives = 3;
        newHighscore = false;
        quakeY = 0;
        
        // Initialize player car (start at lane 1, middle position)
        myCar.hit = false;
        myCar.lane = 1;  // 0-indexed: lanes 0,1,2,3
        myCar.y = myCar.lane * 16;
        
        // Initialize other cars with proper spacing
        for (int i = 0; i < CAR_COUNT; i++) {
            cars[i].x = FRAME_WIDTH + (i * 30);  // distribute the initial pos
            cars[i].y = (rand() % 4) * 16;       // random lanes
            cars[i].speed = 1;    // velocity 2~4
            cars[i].active = true;               // activation status
        }
        
        // Reset road markings
        dotX[0] = 0;
        dotX[1] = 45;
        dotX[2] = 90;
        
        // Load highscore (simplified - using default value)
        
        last_update_time = m_ui.getCurrentTime();
    }

    void onEnter(ExitCallback cb) override {
        IApplication::onEnter(cb);
        m_ui.setContinousDraw(true);
        m_ui.markDirty();
        // load highscore when entering the game
        highscore = SystemConf::getInstance().read_rec_highscore_car_dodge();
        timestamp_now = timestamp_prev = m_ui.getCurrentTime();
        resetGame();
    }

    bool handleInput(InputEvent event) override {
        switch (event) {
            case InputEvent::SELECT:
                if (lives == -1) {  // Game over state
                    resetGame();
                } else {
                    requestExit();
                }
                return true;
                
            case InputEvent::LEFT:
                if (lives != -1) {  // respond only in the game
                    uptMove = UPT_MOVE_UP;
                }
                return true;
                
            case InputEvent::RIGHT:
                if (lives != -1) { 
                    uptMove = UPT_MOVE_DOWN;
                }
                return true;

            case InputEvent::BACK:
                requestExit(); 
                return true;
            default:
                return false;
        }
    }

    void updateGameLogic() {
        uint32_t current_time = m_ui.getCurrentTime();
        
        // Change lane based on input
        if (uptMove == UPT_MOVE_UP && myCar.lane > 0) {
            myCar.lane--;
        } else if (uptMove == UPT_MOVE_DOWN && myCar.lane < 3) {
            myCar.lane++;
        }
        uptMove = UPT_MOVE_NONE;
        
        // Smooth lane transition
        int16_t targetY = myCar.lane * 16;
        if (myCar.y > targetY) {
            myCar.y -= 2;
            if (myCar.y < targetY) myCar.y = targetY;
        } else if (myCar.y < targetY) {
            myCar.y += 2;
            if (myCar.y > targetY) myCar.y = targetY;
        }
        
        if (lives != -1) {
            // Move cars
            for (int i = 0; i < CAR_COUNT; i++) {
                if (cars[i].active) {
                    cars[i].x -= cars[i].speed;
                    
                    // Car went off left edge - respawn on right
                    if (cars[i].x < -CAR_LENGTH) {
                        cars[i].x = FRAME_WIDTH + (rand() % 40);
                        cars[i].y = (rand() % 4) * 16;
                        cars[i].speed = 1 + (rand() % 2);
                        score++;  // 成功躲避一辆车，加分
                    }
                }
            }
            
            // Prevent cars from overlapping on same lane
            for (int i = 0; i < CAR_COUNT; i++) {
                if (!cars[i].active) continue;
                
                for (int c = 0; c < CAR_COUNT; c++) {
                    if (i != c && cars[c].active && cars[i].y == cars[c].y) {
                        // 如果两车在同一车道且过于接近
                        if (cars[i].x > cars[c].x && 
                            cars[i].x < cars[c].x + CAR_LENGTH + 10) {
                            // 调整后车位置
                            cars[i].x = cars[c].x + CAR_LENGTH + 10;
                        }
                    }
                }
            }
            
            // Collision detection
            if (!myCar.hit) {
                for (int i = 0; i < CAR_COUNT; i++) {
                    if (!cars[i].active) continue;
                    
                    // 检查x轴重叠
                    if (cars[i].x < CAR_LENGTH && cars[i].x > -CAR_LENGTH) {
                        // 检查y轴重叠（考虑车辆高度）
                        int16_t carY = cars[i].y;
                        int16_t myCarY = myCar.y;
                        
                        // 碰撞检测：检查两个矩形是否重叠
                        if (!(carY + CAR_WIDTH < myCarY || 
                              carY > myCarY + CAR_WIDTH ||
                              cars[i].x + CAR_LENGTH < 0 ||
                              cars[i].x > CAR_LENGTH)) {
                            
                            myCar.hit = true;
                            hitTime = current_time;
                            lives--;
                            
                            // 重置碰撞的车辆
                            cars[i].x = FRAME_WIDTH + (rand() % 40);
                            cars[i].y = (rand() % 4) * 16;
                            
                            if (lives == -1) {  // Game over
                                if (score > highscore) {
                                    newHighscore = true;
                                    highscore = score;
                                } else {
                                    newHighscore = false;
                                }
                            }
                            break;
                        }
                    }
                }
            }
        }
        
        // Reset hit state after 1 second
        if (myCar.hit && (current_time - hitTime >= 1000)) {
            myCar.hit = false;
        }
        
        // Quake effect during hit
        if (myCar.hit && (current_time - hitTime < 350)) {
            if (quakeY == 2) {
                quakeY = -2;
            } else {
                quakeY = 2;
            }
        } else {
            quakeY = 0;
        }
        
        // Update road markings animation
        for (int i = 0; i < 3; i++) {
            dotX[i] = (dotX[i] + FRAME_WIDTH - ROAD_SPEED) % FRAME_WIDTH;
        }

        // Previous road marking logic (simplified)
        // for (int i = 0; i < 3; i++) {
        //     if (dotX[i] >= ROAD_SPEED) {
        //         dotX[i] -= ROAD_SPEED;
        //     } else {
        //         dotX[i] = 0;
        //     }
            
        //     if (dotX[i] >= FRAME_WIDTH && dotX[i] < 255 - 8) {
        //         dotX[i] = FRAME_WIDTH;
        //     }
        // }
    }

    void draw() override {
        U8G2& u8g2 = m_ui.getU8G2();
        timestamp_now = m_ui.getCurrentTime();
        
        // Update game logic
        updateGameLogic();
        
        // Clear screen
        u8g2.clearBuffer();
        
        if (lives != -1) {
            // === Draw lane dividers (3条分隔线) ===
            for (int lane = 0; lane < 3; lane++) {
                int16_t laneY = (lane + 1) * 16 - 1;  // 在每个16像素车道之间
                // 绘制虚线
                for (int x = 0; x < FRAME_WIDTH; x += 8) {
                    // 使用 ROAD_SPEED 来计算移动效果，模拟道路向左移动
                    if ((x + (timestamp_now * ROAD_SPEED / 100)) % 16 < 8) { // 移动的虚线
                        u8g2.drawPixel(x, laneY + quakeY);
                        u8g2.drawPixel(x + 1, laneY + quakeY);
                    }
                }
            }
            
            // === Draw road markings (moving dots) ===
            
            // === Draw other cars ===
            for (int i = 0; i < CAR_COUNT; i++) {
                if (cars[i].active && cars[i].x >= -CAR_LENGTH && cars[i].x < FRAME_WIDTH) {
                    u8g2.drawXBMP(cars[i].x, cars[i].y + quakeY + (16-12)/2, 15, 12, carImg);
                }
            }
            
            // === Draw player car (with blinking effect when hit) ===
            if (!myCar.hit || (myCar.hit && (timestamp_now % 128 < 64))) {
                u8g2.drawXBMP(0, myCar.y + quakeY + (16-12)/2, 15, 12, carImg);
            }
            
            // === Draw score ===
            u8g2.setFont(u8g2_font_5x7_tr);
            snprintf(print_buffer, sizeof(print_buffer), "Score:%lu", score);
            u8g2.drawStr(70, 8, print_buffer);
            
            // === Draw lives ===
            u8g2.drawStr(2, 8, "Lives:");
            for (int i = 0; i < lives && i < 3; i++) {
                u8g2.drawXBMP(30 + (8 * i), 1, 7, 8, livesImg);
            }
            
        } else {
            // === Game over screen ===
            u8g2.setFont(u8g2_font_7x13B_tr);
            u8g2.drawStr(20, 15, "GAME OVER");
            
            u8g2.setFont(u8g2_font_5x7_tr);
            u8g2.drawStr(20, 30, "SCORE:");
            snprintf(print_buffer, sizeof(print_buffer), "%lu", score);
            u8g2.drawStr(70, 30, print_buffer);
            
            u8g2.drawStr(20, 42, "HIGHSCORE:");
            snprintf(print_buffer, sizeof(print_buffer), "%lu", highscore);
            u8g2.drawStr(70, 42, print_buffer);
            
            if (newHighscore) {
                u8g2.drawStr(28, 52, "NEW HIGHSCORE!");
            }
            
            u8g2.setFont(u8g2_font_4x6_tr);
            u8g2.drawStr(25, 60, "Push Encoder to restart");
        }
        
        u8g2.sendBuffer();
    }
    
    void onExit() override {
        m_ui.setContinousDraw(false);
        m_ui.markFading();
    }
};

// App registration
AppItem racing_game_app{
    .title = "除我都逆行",
    .bitmap = image_racing_bits,
    
    .createApp = [](PixelUI& ui) -> std::unique_ptr<IApplication> { 
        return std::make_unique<RacingGame>(ui); 
    },
};

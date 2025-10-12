#include "core/app/IApplication.h"
#include "core/app/app_system.h"
#include "focus/focus.h"
#include "widgets/num_scroll/num_scroll.h"
#include "widgets/text_button/text_button.h"
#include <stdlib.h>
#include <math.h>

__attribute__((aligned(4)))
static const unsigned char image_games_bits[] = {
    0xf0,0xff,0x0f,0x1c,0x00,0x38,0xde,0xff,0x77,0xde,0xff,0x77,0x5f,0x00,0xf4,0x5f,0x53,0xf5,0x5f,0x77,0xf5,0x5f,0xff,0xf5,0x5f,0xfd,0xf5,0x5f,0xec,0xf5,0x5f,0x84,0xf4,0x5f,0x00,0xf6,0xdf,0xff,0xf7,0xdf,0xd7,0xf7,0xdf,0xfe,0xf4,0x5f,0x9c,0xf4,0xdf,0x9e,0xf7,0xdf,0xff,0xf7,0xdf,0xd7,0xf7,0xdf,0xeb,0xf7,0xde,0xff,0x7b,0x1e,0x00,0x7c,0xfc,0xff,0x3f,0xf0,0xff,0x0f
};

// 游戏常量定义 - 严格按照原版
#define FRAME_WIDTH 128
#define FRAME_HEIGHT 64
#define PLATFORM_WIDTH 12
#define PLATFORM_HEIGHT 4
#define BLOCK_COLS 32  // 原版数值
#define BLOCK_ROWS 5   // 原版数值
#define BLOCK_COUNT (BLOCK_COLS * BLOCK_ROWS)
#define BLOCK_DRAW_WIDTH 3   // 实际绘制宽度
#define BLOCK_DRAW_HEIGHT 3  // 实际绘制高度，改小形成独立方块
#define BLOCK_SPACING 4      // 砖块间距（x*4, y*4）
#define BLOCK_OFFSET_Y 8     // 砖块Y轴偏移
#define BALL_SIZE 2
#define PLATFORM_MOVE_STEP 5

// 球结构体
typedef struct {
    float x;
    float y;
    float velX;
    float velY;
} Ball;

// 游戏状态枚举
enum GameState {
    GAME_PLAYING,
    GAME_WON,
    GAME_OVER
};

class GamePong : public IApplication {
private:
    PixelUI& m_ui;
    FocusManager focusMan;
    
    // 游戏状态变量
    Ball ball;
    bool* blocks;
    uint8_t lives;
    uint16_t score;
    uint8_t platformX;
    GameState gameState;
    uint32_t lastUpdateTime;
    bool gameInitialized;

public:
    GamePong(PixelUI& ui) : m_ui(ui), focusMan(ui), blocks(nullptr), gameInitialized(false) {}
    
    ~GamePong() {
        if (blocks) {
            free(blocks);
            blocks = nullptr;
        }
    }
    
    void onEnter(ExitCallback cb) override {
        IApplication::onEnter(cb);
        m_ui.setContinousDraw(true);
        initGame();
    }
    
    void initGame() {
        // 初始化游戏状态
        ball.x = FRAME_WIDTH / 2.0f;
        ball.y = FRAME_HEIGHT - 15.0f;
        ball.velX = -1.0f;
        ball.velY = -1.1f;
        
        // 分配砖块内存
        if (blocks) {
            free(blocks);
        }
        blocks = (bool*)calloc(BLOCK_COUNT, sizeof(bool));
        
        lives = 3;
        score = 0;
        platformX = (FRAME_WIDTH / 2) - (PLATFORM_WIDTH / 2);
        gameState = GAME_PLAYING;
        lastUpdateTime = m_ui.getCurrentTime();
        gameInitialized = true;
        
        // 使用当前时间作为随机种子
        srand(m_ui.getCurrentTime());
    }
    
    void updateGame() {
        if (gameState != GAME_PLAYING) return;
        
        uint32_t currentTime = m_ui.getCurrentTime();
        float deltaTime = (currentTime - lastUpdateTime) / 1000.0f;
        lastUpdateTime = currentTime;
        
        // 限制deltaTime避免跳跃过大
        if (deltaTime > 0.05f) deltaTime = 0.05f;
        
        // 更新球位置
        ball.x += ball.velX * 30.0f * deltaTime;
        ball.y += ball.velY * 30.0f * deltaTime;
        
        // 球与砖块碰撞检测
        checkBlockCollisions();
        
        // 球与边界碰撞检测
        checkWallCollisions();
        
        // 球与平台碰撞检测
        checkPlatformCollision();
        
        // 检查游戏结束条件
        checkGameEndConditions();
    }
    
    void checkBlockCollisions() {
        bool blockCollided = false;
        int ballX = (int)ball.x;
        int ballY = (int)ball.y;
        
        // 完全按照原版的碰撞检测逻辑
        int idx = 0;
        for (int x = 0; x < BLOCK_COLS; x++) {
            for (int y = 0; y < BLOCK_ROWS; y++) {
                if (!blocks[idx]) {
                    // 原版碰撞检测：ballX >= x*4 && ballX < (x*4) + 4 && ballY >= (y*4) + 8 && ballY < (y*4) + 8 + 4
                    int blockX = x * BLOCK_SPACING;
                    int blockY = (y * BLOCK_SPACING) + BLOCK_OFFSET_Y;
                    
                    if (ballX >= blockX && ballX < blockX + BLOCK_SPACING &&
                        ballY >= blockY && ballY < blockY + BLOCK_SPACING) {
                        blocks[idx] = true;
                        blockCollided = true;
                        score++;
                        break;
                    }
                }
                idx++;
            }
            if (blockCollided) break;
        }
        
        if (blockCollided) {
            ball.velY *= -1;
        }
    }
    
    void checkWallCollisions() {
        // 左右边界
        if (ball.x <= 0) {
            ball.x = 0;
            ball.velX *= -1;
        } else if (ball.x >= FRAME_WIDTH - BALL_SIZE) {
            ball.x = FRAME_WIDTH - BALL_SIZE;
            ball.velX *= -1;
        }
        
        // 上边界
        if (ball.y <= 0) {
            ball.y = 0;
            ball.velY *= -1;
        }
        
        // 下边界（失去生命）
        if (ball.y >= FRAME_HEIGHT) {
            lives--;
            if (lives > 0) {
                // 重置球位置
                ball.x = FRAME_WIDTH / 2.0f;
                ball.y = FRAME_HEIGHT - 15.0f;
                ball.velX = ((float)rand() / RAND_MAX - 0.5f) * 2.0f;
                ball.velY = -1.1f;
            }
        }
    }
    
    void checkPlatformCollision() {
        if (ball.y >= FRAME_HEIGHT - PLATFORM_HEIGHT - BALL_SIZE &&
            ball.y < FRAME_HEIGHT - PLATFORM_HEIGHT &&
            ball.x >= platformX && ball.x <= platformX + PLATFORM_WIDTH) {
            
            ball.y = FRAME_HEIGHT - PLATFORM_HEIGHT - BALL_SIZE;
            if (ball.velY > 0) {
                ball.velY *= -1;
                // 根据球击中平台的位置调整反弹角度
                float hitPos = (ball.x - platformX) / PLATFORM_WIDTH; // 0-1
                ball.velX = (hitPos - 0.5f) * 2.0f; // -1 to 1
            }
        }
    }
    
    void checkGameEndConditions() {
        // 检查是否获胜（所有砖块被摧毁）
        if (score >= BLOCK_COUNT) {
            gameState = GAME_WON;
        }
        // 检查是否失败（生命值用完）
        else if (lives == 0) {
            gameState = GAME_OVER;
        }
    }
    
    void draw() override {
        if (!gameInitialized) return;
        
        U8G2& display = m_ui.getU8G2();
        
        // 更新游戏逻辑
        updateGame();
        
        // 清屏
        display.clearBuffer();
        
        // 绘制平台
        display.drawBox(platformX, FRAME_HEIGHT - PLATFORM_HEIGHT, PLATFORM_WIDTH, PLATFORM_HEIGHT);
        
        // 绘制球
        display.drawBox((int)ball.x, (int)ball.y, BALL_SIZE, BALL_SIZE);
        
        // 绘制砖块 - 严格按照原版逻辑
        int idx = 0;
        for (int x = 0; x < BLOCK_COLS; x++) {
            for (int y = 0; y < BLOCK_ROWS; y++) {
                if (!blocks[idx]) {
                    // 原版绘制：draw_bitmap(x*4, (y*4) + 8, block, 3, 8, NOINVERT, 0)
                    int blockX = x * BLOCK_SPACING;
                    int blockY = (y * BLOCK_SPACING) + BLOCK_OFFSET_Y;
                    
                    // 只有在屏幕范围内才绘制
                    if (blockX < FRAME_WIDTH && blockY < FRAME_HEIGHT - 10) {
                        display.drawBox(blockX, blockY, BLOCK_DRAW_WIDTH, BLOCK_DRAW_HEIGHT);
                    }
                }
                idx++;
            }
        }
        
        // 绘制分数
        display.setFont(u8g2_font_5x7_tf);
        char scoreStr[16];
        sprintf(scoreStr, "Score: %d", score);
        display.drawStr(0, 7, scoreStr);
        
        // 绘制生命值
        char livesStr[16];
        sprintf(livesStr, "Lives: %d", lives);
        display.drawStr(FRAME_WIDTH - 40, 7, livesStr);
        
        // 绘制游戏结束信息
        if (gameState == GAME_WON) {
            display.setFont(u8g2_font_7x13_tf);
            display.drawStr(35, 35, "YOU WIN!");
            display.setFont(u8g2_font_5x7_tf);
            display.drawStr(25, 50, "BACK to restart");
        } else if (gameState == GAME_OVER) {
            display.setFont(u8g2_font_7x13_tf);
            display.drawStr(25, 35, "GAME OVER");
            display.setFont(u8g2_font_5x7_tf);
            display.drawStr(25, 50, "BACK to restart");
        }
        
        display.sendBuffer();
    }
    
    bool handleInput(InputEvent event) override {
        // 检查活动组件
        IWidget* activeWidget = focusMan.getActiveWidget();
        if (activeWidget) {
            if (activeWidget->handleEvent(event)) {
                focusMan.clearActiveWidget();
            }
            return true;
        }
        
        // 处理游戏输入
        if (event == InputEvent::BACK) {
            if (gameState != GAME_PLAYING) {
                // 重新开始游戏
                initGame();
            } else {
                requestExit();
            }
        } 
        else if (event == InputEvent::RIGHT) {
            if (gameState == GAME_PLAYING) {
                // 移动平台右移2像素
                if (platformX + PLATFORM_MOVE_STEP <= FRAME_WIDTH - PLATFORM_WIDTH) {
                    platformX += PLATFORM_MOVE_STEP;
                }
            }
        } 
        else if (event == InputEvent::LEFT) {
            if (gameState == GAME_PLAYING) {
                // 移动平台左移2像素
                if (platformX >= PLATFORM_MOVE_STEP) {
                    platformX -= PLATFORM_MOVE_STEP;
                } else {
                    platformX = 0;
                }
            }
        } 
        else if (event == InputEvent::SELECT) {
            if (gameState != GAME_PLAYING) {
                // 重新开始游戏
                initGame();
            }
        }
        
        return true;
    }
    
    void onExit() override {
        m_ui.setContinousDraw(false);
        if (blocks) {
            free(blocks);
            blocks = nullptr;
        }
    }
};

// 自注册机制 - 更新名称
AppItem game_pong_app{
    .title = "Pong",
    .bitmap = image_games_bits,
    
    .createApp = [](PixelUI& ui) -> std::unique_ptr<IApplication> { 
        return std::make_unique<GamePong>(ui); 
    },
};
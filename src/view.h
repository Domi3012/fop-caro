#pragma once

#include "model.h"
#include "controller.h"

// Quản lý tài nguyên của View
void initView(); // goi sau khi init window
void unloadView(); // goi truoc khi close window

// controller chi can goi cai ham nay de ve thoi
void renderGame(const MatchState& match, const UIState& ui);

// API Quản lý tài nguyên đồ họa (Dành cho Dev 3 & 4)
void initVisualAssets();   // Load ảnh background, spritesheet, font
void unloadVisualAssets();

// Các hàm render màn hình mới (Dành cho Dev 1 & 2)
void drawSettingsScreen(const UIState& ui);
void drawLoadGameScreen(const UIState& ui, const std::vector<std::string>& saveFiles);

// API Hiệu ứng và Polish (Dành cho Dev 3)
// 1. Vẽ nhân vật có animation (isAttacking = true thì vẽ sprite chém/phép)
void drawAnimatedCharacter(CharacterType type, float x, float y, bool isAttacking);

// 2. Vẽ thanh máu tụt từ từ (cần lưu lại máu cũ và lerp tới máu mới)
void drawHealthBarSmooth(int currentHealth, int maxHealth, float x, float y, float width, float height);

// 3. Highlight 5 quân cờ (glow effect)
void drawWinningHighlight(const std::vector<std::pair<int, int>>& winningCells, float startX, float startY, float cellSize);

// Vài màu dùng chung

const Color buttonYellow = GetColor(0xb8dbc0FF);
const Color buttonDarkPurple = GetColor(0x1A2421FF);

// Vài texture dùng chung
static Font font8bit;

// --- PARALLAX BACKGROUND ---
// Tên file các layer rừng, xếp từ xa nhất (index 0) đến gần nhất (index 11)
// Quy ước: số suffix càng nhỏ = càng gần = càng nhanh
static const char* FOREST_LAYER_PATHS[] = {
    "./assets/images/layered_forest/Layer_0011_0.png",  // xa nhất - chậm nhất
    "./assets/images/layered_forest/Layer_0010_1.png",
    "./assets/images/layered_forest/Layer_0009_2.png",
    "./assets/images/layered_forest/Layer_0008_3.png",
    "./assets/images/layered_forest/Layer_0007_Lights.png",
    "./assets/images/layered_forest/Layer_0006_4.png",
    "./assets/images/layered_forest/Layer_0005_5.png",
    "./assets/images/layered_forest/Layer_0004_Lights.png",
    "./assets/images/layered_forest/Layer_0003_6.png",
    "./assets/images/layered_forest/Layer_0002_7.png",
    "./assets/images/layered_forest/Layer_0001_8.png",
    "./assets/images/layered_forest/Layer_0000_9.png",  // gần nhất - nhanh nhất
};
static const int FOREST_LAYER_COUNT = 12;

struct ParallaxLayer {
    Texture2D texture;
    float scrollX;   // offset cuộn hiện tại (pixel, âm = đã cuộn sang trái)
    float speed;     // pixel/giây
};

static ParallaxLayer forestLayers[FOREST_LAYER_COUNT];

// Vẽ parallax background cho main menu, gọi mỗi frame
void drawParallaxBackground(float speedMultiplier = 1.0f);

// Declaration cua vai ham noi bo
// main menu:
void drawMenuButton(const UIState& ui);
void drawMenu(const UIState& ui);
void drawModeSelectionScreen(const UIState& ui);
void drawGameIntro(const MatchState& match, const UIState& ui);
// game caro:
void drawCaroGame(const MatchState& match, const UIState& ui);
void drawBoard(const MatchState& match, const UIState& ui);
void drawStatusPanel(const MatchState& match); // Vẽ máu, tên nhân vật
void drawTurnBanner(const MatchState& match);
// char select
void drawCharSelection(const UIState& ui);
// game over
void drawGameOver(const MatchState& match, const UIState& ui);
// load menu
std::string formatSaveDisplayName(const std::string& fileName);

// Resolution options for settings menu
struct ResolutionOption {
    int width;
    int height;
    const char* label;
};

static const ResolutionOption RESOLUTIONS[] =
{
    { 1280,  720,  "1280x720 (16:9)"   },
    { 1366,  768,  "1366x768 (16:9)"   },
    { 1600,  900,  "1600x900 (16:9)"   },
    { 1920, 1080,  "1920x1080 (16:9)"  },
    { 2560, 1440,  "2560x1440 (16:9)"  }
};

static const int RESOLUTION_COUNT = sizeof(RESOLUTIONS) / sizeof(RESOLUTIONS[0]);

static bool isResolutionAllowed(int width, int height) {
    int monitor = GetCurrentMonitor();
    return width <= GetMonitorWidth(monitor) && height <= GetMonitorHeight(monitor);
}
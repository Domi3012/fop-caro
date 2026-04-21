#pragma once

#include "model.h"
#include "controller.h"

// Quản lý tài nguyên của View
void initView();   // goi sau khi init window
void unloadView(); // goi truoc khi close window

// controller chi can goi cai ham nay de ve thoi
void renderGame(const MatchState &match, const UIState &ui);

// API Quản lý tài nguyên đồ họa (Dành cho Dev 3 & 4)
void initVisualAssets(); // Load ảnh background, spritesheet, font
void unloadVisualAssets();

// Các hàm render màn hình mới (Dành cho Dev 1 & 2)
void drawSettingsScreen(const UIState &ui);
void drawLoadGameScreen(const UIState &ui, const std::vector<std::string> &saveFiles);

// API Hiệu ứng và Polish (Dành cho Dev 3)
// 1. Vẽ nhân vật có animation (isAttacking = true thì vẽ sprite chém/phép)
void drawAnimatedCharacter(CharacterType type, float x, float y, bool isAttacking);

// 2. Vẽ thanh máu tụt từ từ (cần lưu lại máu cũ và lerp tới máu mới)
void drawHealthBarSmooth(int currentHealth, int maxHealth, float x, float y, float width, float height);

// 3. Highlight 5 quân cờ (glow effect)
void drawWinningHighlight(const std::vector<std::pair<int, int>> &winningCells, float startX, float startY, float cellSize);

// Declaration cua vai ham noi bo
// main menu:
void drawMenuButton(const UIState &ui);
void drawMenu(const UIState &ui);
void drawModeSelectionScreen(const UIState &ui);
void drawGameIntro(const MatchState &match, const UIState &ui);
// game caro:
void drawCaroGame(const MatchState &match, const UIState &ui);
void drawBoard(const MatchState &match, const UIState &ui);
void drawStatusPanel(const MatchState &match); // Vẽ máu, tên nhân vật
void drawTurnBanner(const MatchState &match);
void drawBotDifficultyScreen(const UIState &ui);

// char select
void drawCharSelection(const UIState &ui);
// game over
void drawGameOver(const MatchState &match, const UIState &ui);
// load menu
std::string formatSaveDisplayName(const std::string &fileName);

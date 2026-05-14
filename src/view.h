#pragma once

#include "model.h"
#include "controller.h"

// Resource lifecycle (call after InitWindow / before CloseWindow)
void initView();
void unloadView();

// Master render — dispatches to screen-specific draw functions
void renderGame(const MatchState &match, UIState &ui);

// Screen renderers called by controller or renderGame
void drawSettingsScreen(const UIState &ui);
void drawLoadGameScreen(const UIState &ui, const std::vector<std::string> &saveFiles);
void drawSaveGameScreen(const UIState &ui);
void drawModeSelectionScreen(const UIState &ui);
void drawGameIntro(const MatchState &match, const UIState &ui);
void drawCaroGame(const MatchState &match, UIState &ui);
void drawBotDifficultyScreen(const UIState &ui);
void drawCharSelection(const UIState &ui);
void drawGameOver(const MatchState &match, const UIState &ui);

// Utility: tách tên đặt và ngày giờ từ tên file save
// VD: "MyGame_20260415_120000.txt" -> displayName="MyGame", dateTime="15/04/2026 12:00:00"
void parseSaveFileName(const std::string &fileName, std::string &displayName, std::string &dateTimeInfo);

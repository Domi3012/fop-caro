#pragma once

#include "model.h"
#include "controller.h"

// Resource lifecycle (call after InitWindow / before CloseWindow)
void initView();
void unloadView();

// Master render — dispatches to screen-specific draw functions
void renderGame(const MatchState &match, const UIState &ui);

// Screen renderers called by controller or renderGame
void drawSettingsScreen(const UIState &ui);
void drawLoadGameScreen(const UIState &ui, const std::vector<std::string> &saveFiles);
void drawModeSelectionScreen(const UIState &ui);
void drawGameIntro(const MatchState &match, const UIState &ui);
void drawCaroGame(const MatchState &match, const UIState &ui);
void drawBotDifficultyScreen(const UIState &ui);
void drawCharSelection(const UIState &ui);
void drawGameOver(const MatchState &match, const UIState &ui);

// Utility
std::string formatSaveDisplayName(const std::string &fileName);

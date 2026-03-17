#pragma once

#include "model.h"
#include "controller.h"

// Quản lý tài nguyên của View
void initView();
void unloadView();

void renderGame(const MatchState& match, const UIState& ui);

void drawMenu(const UIState& ui);
//void drawCharSelection(const UIState& ui);
//void drawBoard(const MatchState& match, const UIState& ui);
//void drawStatusPanel(const MatchState& match); // Vẽ máu, tên nhân vật
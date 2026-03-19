#pragma once

#include "model.h"
#include "raylib.h"

enum GameScreen { // Bản mvp nên đơn giản thôi
	MAIN_MENU,
	CHARACTER_SELECTION,
	GAME_BOARD,
	ROUND_OVER,
	GAME_OVER
};

struct UIState {
	GameScreen currentScreen;

	// Main menu:
	int mainMenuIndex; // Chỉ có 1 = "Start Game", nhưng để sau này dễ mở rộng thêm option khác

	// Character selection:
	bool isSelectingX; // true nếu đang chọn nhân vật cho X, false nếu đang chọn cho O
	int characterMenuIndex; // 1 = ASSASSIN, 2 = BRUISER, 3 = VAMPIRE

	// Game board:
	int cursorX;
	int cursorY;
};

// CÁC HÀM XỬ LÝ (CHỨC NĂNG CỦA CONTROLLER)

void handleMainMenuInput(UIState& ui); // Lên xuống để chọn, enter để xác nhận (chỉ có 1 option thôi nhưng vẫn làm cho nó đúng quy trình)
void handleCharSelectionInput(MatchState& match, UIState& ui); // Lên xuống để chọn nhân vật, enter để xác nhận, sau khi chọn xong cho X thì chuyển sang chọn O, sau khi chọn xong cho O thì chuyển state sang GAME_BOARD
void handleGameplayInput(MatchState& match, UIState& ui); // lên xuống trái phải để di chuyển, enter để chơi
void handleRoundOverInput(MatchState& match, UIState& ui); // Làm đơn giản: Nhấn bất kì để tiếp tục, sau đó chuyển state
void handleGameOverInput(MatchState& match, UIState& ui); // Nhất enter để quay lại main menu, hoặc esc để thoát game


void handleInput(MatchState& match, UIState& ui);



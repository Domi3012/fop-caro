#pragma once

#include "model.h"
#include "raylib.h"

enum GameScreen { // Bản mvp nên đơn giản thôi
	MAIN_MENU,
	CHARACTER_SELECTION,
	GAME_BOARD,
	ROUND_OVER,
	GAME_OVER,
	LOAD_GAME, 
	SETTINGS
};

struct UIState {
	bool shouldExit = false;

	GameScreen currentScreen;

	// Main menu:
	int mainMenuIndex;
	// menu index:
	// 0: New Game
	// 1: Load Game
	// 2: Settings
	// 3: Exit

	// Character selection:
	bool isSelectingX; // true nếu đang chọn nhân vật cho X, false nếu đang chọn cho O
	int characterMenuIndex; // 1 = ASSASSIN, 2 = BRUISER, 3 = VAMPIRE

	// Game board:
	int cursorX;
	int cursorY;

	// load menu
	int loadMenuIndex;       // Vị trí con trỏ đang chọn file save nào

	// settings menu
	int settingsMenuIndex;   // Vị trí con trỏ đang chọn Audio hay SFX

	// round over
	float roundOverTimer = 0.0f; // đếm thời gian hiển thị màn ROUND_OVER (giây)
};

// CÁC HÀM XỬ LÝ (CHỨC NĂNG CỦA CONTROLLER)

void handleMainMenuInput(UIState& ui); // Lên xuống để chọn, enter để xác nhận (chỉ có 1 option thôi nhưng vẫn làm cho nó đúng quy trình)
void handleCharSelectionInput(MatchState& match, UIState& ui); // Lên xuống để chọn nhân vật, enter để xác nhận, sau khi chọn xong cho X thì chuyển sang chọn O, sau khi chọn xong cho O thì chuyển state sang GAME_BOARD
void handleGameplayInput(MatchState& match, UIState& ui); // lên xuống trái phải để di chuyển, enter để chơi
void handleRoundOverInput(MatchState& match, UIState& ui); // Làm đơn giản: Nhấn bất kì để tiếp tục, sau đó chuyển state
void handleGameOverInput(MatchState& match, UIState& ui); // Nhất enter để quay lại main menu, hoặc esc để thoát game


void handleInput(MatchState& match, UIState& ui);


// Các hàm xử lý input mới (Dành cho Dev 1 & 2)
void handleLoadGameInput(MatchState& match, UIState& ui, const std::vector<std::string>& saveFiles);
void handleSettingsInput(UIState& ui);

// Hàm thống nhất chuyển sang trạng thái GAME_BOARD (dùng chung cho cả New Game lẫn Load Game)
// Gọi hàm này sau khi match đã được khởi tạo đầy đủ (qua initMatch hoặc loadGame)
void startMatch(UIState& ui);

// Hàm setup độ phân giải/scale (Dành cho Dev 1)
void applyCameraAndScaling(); // Gọi ở đầu game hoặc khi thay đổi cửa sổ
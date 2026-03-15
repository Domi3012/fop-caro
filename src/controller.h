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

// --- CÁC HÀM XỬ LÝ (CHỨC NĂNG CỦA CONTROLLER) ---

// 1. Hàm điều phối chính (Gọi trong main loop)
void handleInput(MatchState& match, UIState& ui); // Lắng nghe các nút bấm và xử lí gọi dữ liệu và gọi các hàm bên dưới

// 2. Các hàm xử lý riêng cho từng màn hình
void updateMenu(UIState& ui);
void updateCharSelection(MatchState& match, UIState& ui);
void updateGameplay(MatchState& match, UIState& ui);


// ví dụ: cho GAME_BOARD, hàm sẽ lắng nghe các nút di chuyển con trỏ (WASD) và nút xác nhận (Enter),
// sau đó gọi checkValidMove và makeMove nếu hợp lệ, rồi gọi checkRoundResult để cập nhật kết
// quả vòng đấu. 

void handleInput(MatchState& match, UIState& ui) {
	switch (ui.currentScreen) {
	case GAME_BOARD:
		if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) ui.cursorX++ % BOARD_SIZE;
		if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) ui.cursorX++ % BOARD_SIZE;



		updateGameplay(match, ui);
		break;
	default:
		break;
	}
}
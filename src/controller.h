#pragma once

#include "model.h"
#include "raylib.h"
#include <string>
#include "bot_ai.h"

enum GameScreen
{
    MAIN_MENU,                // Màn hình menu chính
    MODE_SELECTION,           // Chọn chế độ: PVP hoặc PVE
    BOT_DIFFICULTY_SELECTION, // Chọn độ khó bot (chỉ xuất hiện ở PVE)
    NAME_INPUT,               // Màn hình nhập tên người chơi (trước khi chọn nhân vật)
    CHARACTER_SELECTION,      // Mỗi người chọn nhân vật lần lượt (X trước, O sau)
    GAME_INTRO,               // Animation camera bay vào trước khi bắt đầu ván đấu
    GAME_BOARD,               // Màn hình chơi cờ chính
    ATTACK_ANIMATION,         // Phát animation tấn công (1800ms) trước khi chuyển ROUND_OVER
    ROUND_OVER,               // Màn hình kết thúc một round (delay 2s rồi tự chuyển)
    GAME_OVER,                // Màn hình kết thúc toàn bộ trận đấu
    LOAD_GAME,                // Danh sách file save để tải
    SAVE_GAME,                // Màn hình đặt tên file save
    SETTINGS                  // Màn hình cài đặt âm thanh và hiển thị
};

// Toàn bộ trạng thái giao diện — Controller đọc/ghi, View chỉ được đọc
struct UIState
{
    bool shouldExit = false;  // true → vòng lặp game sẽ thoát
    GameScreen currentScreen; // Màn hình đang hiển thị

    // --- Menu chính ---
    int mainMenuIndex; // Index mục đang được highlight (0‥3)
    int modeMenuIndex; // 0 = PVP, 1 = PVE
    bool isPVE;        // Chế độ chơi hiện tại có phải PVE không
    int loadMenuIndex; // Index file save đang chọn

    // --- Chọn nhân vật ---
    bool isSelectingX;      // true = X đang chọn, false = O/Bot đang chọn
    int characterMenuIndex; // 1 = ASSASSIN, 2 = BRUISER, 3 = VAMPIRE

    // --- Bot ---
    int botDifficultyIndex = 0;         // Index trong danh sách độ khó (0‥2)
    BotDifficulty botDifficulty = EASY; // Độ khó được áp dụng khi gọi getBestMove()

    // --- Bàn cờ ---
    int cursorX;                 // Cột con trỏ đang trỏ (0 ≤ cursorX < BOARD_SIZE)
    int cursorY;                 // Hàng con trỏ đang trỏ (0 ≤ cursorY < BOARD_SIZE)
    bool isPaused = false;       // true → game tạm dừng, hiện pause menu
    int pauseMenuIndex = 0;      // 0 = Save, 1 = Quit to Menu
    float roundOverTimer = 0.0f; // Đồng hồ tính thời gian delay ở ROUND_OVER / GAME_INTRO
    float introCamX = 0.0f;      // Vị trí camera theo trục X cho animation intro

    // --- Undo/Redo ---
    std::vector<MoveRecord> moveHistory; // Complete history of all moves with undo status
    std::vector<MoveRecord> undoStack;   // Moves that can be undone
    std::vector<MoveRecord> redoStack;   // Moves that can be redone

    // --- Name input ---
    std::string nameInputBuffer; // Current text being entered on name input screen
    bool isEnteringPlayerXName;  // true = entering X's name, false = entering O's name
    std::string playerXName;     // Stored name for Player X
    std::string playerOName;     // Stored name for Player O

    // --- Save naming ---
    std::string saveNameInput;  // Tên người dùng nhập khi lưu game
    bool saveNameError = false; // true nếu tên có kí tự đặc biệt

    // --- Xoá save ---
    bool showDeleteConfirm = false; // true → hiện popup xác nhận xoá

    // --- Cài đặt ---
    int settingsMenuIndex = 0; // Index mục cài đặt đang chọn (0‥6)
    float uiScale = 1.0f;      // Tỉ lệ co giãn giao diện (hiện chưa dùng)
    bool isFullscreen = true;  // true = toàn màn hình, false = cửa sổ
    int resolutionIndex = 4;   // Index trong mảng RESOLUTIONS[] (mặc định 1920×1080)
    int renderWidth = 1920;    // Chiều rộng render hiện tại (px)
    int renderHeight = 1080;   // Chiều cao render hiện tại (px)

    // --- Hiệu ứng HP mượt ---
    float displayHealthX = 0.0f; // HP hiển thị (nội suy mượt) của X
    float displayHealthO = 0.0f; // HP hiển thị (nội suy mượt) của O

    // --- Floating damage/heal text ---
    struct FloatingText
    {
        std::string text;
        Color color;
        float x, y;     // vị trí hiển thị (screen coords)
        float timer;    // thời gian còn lại
        float maxTimer; // thời gian tổng
    };
    std::vector<FloatingText> floatingTexts;

    // --- Attack Animation ---
    bool attackAnimPlaying = false;                // true khi đang phát animation attack
    PlayerType attackingPlayer = NONE;             // Ai đang tấn công (X hoặc O)
    int attackStep = 0;                            // Bước hiện tại (0..8)
    float attackTimer = 0.0f;                      // Timer cho bước hiện tại
    GameScreen postAttackScreen = ROUND_OVER;      // Màn hình chuyển đến sau animation
};

// Luồng điều phối chính

// Khởi động bàn cờ: đặt con trỏ về giữa, reset timer, chuyển sang GAME_BOARD.
// Dùng chung cho New Game, Load Game và bắt đầu round mới.
void startMatch(UIState &ui);

// Bắt đầu animation intro: đặt camera lệch sang phải rồi chuyển sang GAME_INTRO.
// Phải gọi sau khi initMatch() hoặc loadGame() đã xong.
void startGameIntro(UIState &ui);

// Đặt quân tại (x, y), kiểm tra kết quả round và match, cập nhật ui.currentScreen.
// x = hàng (row), y = cột (col) — tương ứng với board[x][y] trong model.
void processMoveAndResult(MatchState &match, UIState &ui, int x, int y);

// Undo/Redo functions
void undoMove(MatchState &match, UIState &ui);
void redoMove(MatchState &match, UIState &ui);

// Gọi mỗi frame
void handleInput(MatchState &match, UIState &ui);
// Menu chính
void handleMainMenuInput(UIState &ui);
// Chọn chế độ
void handleModeSelectionInput(UIState &ui);
// Chọn độ khó
void handleBotDifficultyInput(UIState &ui);
// Chọn nhân vật
void handleCharSelectionInput(MatchState &match, UIState &ui);
// Nhập tên người chơi
void handleNameInputScreen(MatchState &match, UIState &ui);
// Intro animation
void handleGameIntroInput(MatchState &match, UIState &ui);
// Gameplay
void handleGameplayInput(MatchState &match, UIState &ui);
// Round over
void handleRoundOverInput(MatchState &match, UIState &ui);
// Game over
void handleGameOverInput(MatchState &match, UIState &ui);
// Load game
void handleLoadGameInput(MatchState &match, UIState &ui, std::vector<std::string> &saveFiles);
// Save game (đặt tên)
void handleSaveGameInput(MatchState &match, UIState &ui);
// Settings
void handleSettingsInput(UIState &ui);
// Attack animation
void handleAttackAnimInput(MatchState &match, UIState &ui);

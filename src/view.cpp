#include "view.h"
#include "raylib.h"

// Hàm render mock - Chỉ in text để test logic Controller
void renderGame(const MatchState& match, const UIState& ui) {
    DrawText("--- DEBUG VIEW: UI STATE ---", 20, 20, 20, DARKGRAY);

    switch (ui.currentScreen) {
    case MAIN_MENU:
        DrawText("State: MAIN_MENU", 20, 60, 20, BLUE);
        DrawText(TextFormat("mainMenuIndex: %d", ui.mainMenuIndex), 20, 100, 20, BLACK);
        DrawText("-> Test: Up/Down to change index, Enter to select", 20, 140, 20, GRAY);
        break;

    case CHARACTER_SELECTION:
        DrawText("State: CHARACTER_SELECTION", 20, 60, 20, BLUE);
        DrawText(TextFormat("isSelectingX: %s", ui.isSelectingX ? "TRUE (Player X)" : "FALSE (Player O)"), 20, 100, 20, BLACK);
        DrawText(TextFormat("characterMenuIndex: %d", ui.characterMenuIndex), 20, 140, 20, BLACK);
        DrawText("(0: ASSASSIN, 1: BRUISER, 2: VAMPIRE)", 20, 180, 20, DARKGRAY);
        DrawText("-> Test: Up/Down to change char, Enter to confirm", 20, 220, 20, GRAY);
        break;

    case GAME_BOARD:
        DrawText("State: GAME_BOARD", 20, 60, 20, BLUE);
        DrawText(TextFormat("cursorX: %d", ui.cursorX), 20, 100, 20, BLACK);
        DrawText(TextFormat("cursorY: %d", ui.cursorY), 20, 140, 20, BLACK);
        DrawText(TextFormat("turnCount: %d", match.currentRound.turnCount), 20, 180, 20, BLACK); // Trích xuất thử data từ MatchState
        DrawText("-> Test: Arrow keys to move cursor, Enter to place", 20, 220, 20, GRAY);
        break;

    case ROUND_OVER:
        DrawText("State: ROUND_OVER", 20, 60, 20, ORANGE);
        DrawText("-> Test: Press any key to continue", 20, 100, 20, GRAY);
        break;

    case GAME_OVER:
        DrawText("State: GAME_OVER", 20, 60, 20, RED);
        DrawText("-> Test: Press Enter for Menu, Esc to quit", 20, 100, 20, GRAY);
        break;
    }
}
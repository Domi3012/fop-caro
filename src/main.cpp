#include "raylib.h"
#include "model.h"
#include "controller.h"
#include "view.h"

int main()
{
    // 1. Báo cho Raylib biết mình muốn chạy Fullscreen
    SetConfigFlags(FLAG_FULLSCREEN_MODE);

    // 2. Truyền 0, 0 để tự động fit với độ phân giải màn hình hiện tại
    InitWindow(0, 0, "RGBCaro - The RPG Caro Game");
    initView();
    SetTargetFPS(60);

    MatchState match;
    UIState ui;
    ui.currentScreen = MAIN_MENU;
    ui.mainMenuIndex = 0;

    initRound(match.currentRound, 0);

    while (!WindowShouldClose())
    {
        handleInput(match, ui);
        if (IsKeyDown(KEY_ESCAPE))
            break;

        BeginDrawing();

        renderGame(match, ui);

        EndDrawing();
    }

    unloadView();
    CloseWindow();
    return 0;
}
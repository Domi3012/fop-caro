#include "raylib.h"
#include "model.h"
#include "controller.h"
#include "view.h"
#include "audio_manager.h"

int main()
{
    // 1. Báo cho Raylib biết mình muốn chạy Fullscreen
    SetConfigFlags(FLAG_FULLSCREEN_MODE);
    
    // 2. Truyền 0, 0 để tự động fit với độ phân giải màn hình hiện tại
    InitWindow(0, 0, "RGBCaro - The RPG Caro Game");
    initAudio(); // thêm khởi tạo hệ thống âm thanh
    initView();
    SetTargetFPS(60);
    
    MatchState match;
    UIState ui;
    ui.currentScreen = MAIN_MENU;
    ui.mainMenuIndex = 0;
    playMusic(BGM_MENU); // thêm nhạc nền menu khi vào game

    initRound(match.currentRound, 0);
    while (!ui.shouldExit)
    {
        handleInput(match, ui);
        updateAudioStream(); // thêm cập nhật music stream mỗi frame

        BeginDrawing();

        renderGame(match, ui);

        EndDrawing();
    }

    unloadView();
    unloadAudio(); // thêm giải phóng tài nguyên âm thanh
    CloseWindow();
    return 0;
}
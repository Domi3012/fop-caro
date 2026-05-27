#include "raylib.h"
#include "model.h"
#include "controller.h"
#include "view.h"
#include "audio_manager.h"
#include "sprite_manager.h"

int main()
{
    // --- Init ---
    SetConfigFlags(FLAG_FULLSCREEN_MODE);
    InitWindow(0, 0, "RGBCaro - The RPG Caro Game");  // 0,0 = auto-fit monitor
    initAudio();
    initView();
    initSpriteManager();
    SetTargetFPS(60);

    MatchState match;
    UIState ui;
    ui.currentScreen = MAIN_MENU;
    ui.mainMenuIndex = 0;
    playMusic(BGM_MENU);
    initRound(match.currentRound, 0);

    SetExitKey(0);  // Disable ESC auto-quit; only allow exit via menu/window close

    // --- Game Loop ---
    while (!ui.shouldExit && !WindowShouldClose())
    {
        handleInput(match, ui);
        updateAudioStream();
        BeginDrawing();
        renderGame(match, ui);
        EndDrawing();
    }

    // --- Cleanup ---
    unloadSpriteManager();
    unloadView();
    unloadAudio();
    CloseWindow();
    return 0;
}
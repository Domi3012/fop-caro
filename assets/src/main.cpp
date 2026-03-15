#include "raylib.h"

int main() {
    InitWindow(800, 600, "Game Caro RPG");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(RAYWHITE);
        DrawText("Test Raylib", 190, 280, 20, DARKGRAY);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
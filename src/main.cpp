#include <raylib.h>

int main() {
    InitWindow(800, 600, "Test Raylib Build");
    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(RAYWHITE);
        DrawText("Raylib da tich hop thanh cong!", 190, 200, 20, LIGHTGRAY);
        EndDrawing();
    }
    CloseWindow();
    return 0;
}
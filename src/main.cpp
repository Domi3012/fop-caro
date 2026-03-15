#include "model.h"
#include "controller.h"
#include "view.h"

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



// Ví dụ luồng chương trình main có thể có
/*
int main() {
    // --- 1. KHỞI TẠO HỆ THỐNG ĐỒ HỌA (RAYLIB) ---
    const int screenWidth = 1200; // Để rộng ra chút để vẽ board + status panel
    const int screenHeight = 800;
    InitWindow(screenWidth, screenHeight, "RGBCaro - The RPG Caro Game");
    SetTargetFPS(60);

    // --- 2. KHỞI TẠO TRẠNG THÁI TRÒ CHƠI (MODEL & UI) ---
    MatchState match;
    UIState ui;

    // Gán các giá trị mặc định ban đầu cho UI
    ui.currentScreen = MAIN_MENU;
    ui.mainMenuIndex = 0;
    ui.isSelectingX = true;
    ui.cursorX = BOARD_SIZE / 2;
    ui.cursorY = BOARD_SIZE / 2;

    // --- 3. VÒNG LẶP GAME CHÍNH (GAME LOOP) ---
    while (!WindowShouldClose()) {

        // --- GIAI ĐOẠN 1: XỬ LÝ INPUT (CONTROLLER) ---
        // handleInput sẽ dựa vào ui.currentScreen để gọi các hàm con 
        // như handleMainMenuInput, handleGameplayInput...
        handleInput(match, ui);

        // --- GIAI ĐOẠN 2: CẬP NHẬT LOGIC (MODEL) ---
        // Thường các logic thay đổi dữ liệu đã được Controller gọi 
        // thông qua makeMove(), executeAttack() bên trong handleInput.
        // Ở đây ta có thể cập nhật thêm các hiệu ứng thời gian (nếu có).

        // --- GIAI ĐOẠN 3: VẼ LÊN MÀN HÌNH (VIEW) ---
        BeginDrawing();
        ClearBackground(RAYWHITE);

        // Hàm renderGame sẽ Switch-Case để vẽ đúng màn hình hiện tại
        renderGame(match, ui);

        EndDrawing();

        // --- XỬ LÝ THOÁT GAME ĐẶC BIỆT ---
        // Nếu người chơi chọn Exit ở Menu hoặc bấm ESC ở màn hình GameOver
        if (ui.currentScreen == MAIN_MENU && IsKeyPressed(KEY_ESCAPE)) break;
    }

    // --- 4. GIẢI PHÓNG TÀI NGUYÊN ---
    CloseWindow();

    return 0;
}
*/
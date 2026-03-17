#include "view.h"
#include "raylib.h"
#include <string>

using std::string;


static Texture2D menuBg;
static Font fontTtf = LoadFontEx("assets/fonts/Ithaca.ttf", 32, 0, 250);

// vai ham noi bo
// main menu:
void drawMenuButton(const UIState& ui);

void renderGame(const MatchState& match, const UIState& ui) {
    switch (ui.currentScreen) {
    case MAIN_MENU:
        drawMenu(ui);
        break;

    case CHARACTER_SELECTION:
        break;

    case GAME_BOARD:
        break;

    case ROUND_OVER:
        break;

    case GAME_OVER:
        break;
    }
}

void drawMenu(const UIState& ui) {
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();

    float scale = std::max((float)screenHeight / menuBg.height,
                     (float)screenHeight / menuBg.height);

    DrawTextureEx(menuBg, { 0.0f, 0.0f }, 0.0f, scale, WHITE);

    drawMenuButton(ui);
}

void initView() {
    menuBg = LoadTexture("./assets/images/menuBg.png");
}

void unloadView() {
    UnloadTexture(menuBg);
}

void drawMenuButton(const UIState& ui) {
    string options[] = { "Start Game", "Exit"}; // Khớp với controller của bạn
    const int menuFontSize = 30;
    int totalOptions = 2;

    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();

    for (int i = 0; i < totalOptions; i++) {
        int buttonWidth = 0.2*screenW;
        int buttonHeight = 0.15*screenH;
        int posX = screenW / 2 - buttonWidth / 2;
        int posY = screenH * 0.45 + i * (buttonHeight + 0.05*screenH); // Cách nhau 70px nếu có nhiều nút

        // Kiểm tra xem nút này có đang được chọn không (dựa vào UIState)
        bool isSelected = (ui.mainMenuIndex == (i + 1));

        // Đổi màu nếu được chọn
        Color buttonYellow = GetColor(0xD9E69AFF);
        Color buttonDarkPurple = GetColor(0x240620FF);

        Color bgColor = isSelected ? buttonYellow : buttonDarkPurple;
        Color borderColor = buttonDarkPurple;
        Color textColor = isSelected ? buttonDarkPurple: buttonYellow;

        // Vẽ thân nút và viền (kiểu 8-bit vuông vức)
        DrawRectangle(posX, posY, buttonWidth, buttonHeight, bgColor);
        DrawRectangleLinesEx({ (float)posX, (float)posY, (float)buttonWidth, (float)buttonHeight }, 4, borderColor); // Viền dày 4px

        int textWidth = MeasureText(options[i].c_str(), menuFontSize);

        // 3. Áp dụng công thức căn giữa
        int textX = posX + (buttonWidth - textWidth) / 2;
        int textY = posY + (buttonHeight - menuFontSize) / 2;

        // Vẽ chữ (Bạn nên LoadFont thay vì dùng font mặc định để ra chất 8-bit)
        //LoadFont("./assets/fonts/Ithaca.ttf");
        
        DrawText(options[i].c_str(), textX, textY, menuFontSize, textColor);
    }
}
#include "view.h"
#include "raylib.h"
#include "model.h"
#include <string>
#include <vector>

using std::string;
using std::vector;

// Vài màu dùng chung

const Color buttonYellow = GetColor(0xD9E69AFF);
const Color buttonDarkPurple = GetColor(0x240620FF);

// Vài texture dùng chung
static Texture2D menuBg;
static Texture2D plainBg;
static Font font8bit;

// Declaration cua vai ham noi bo
void drawBackground(const UIState& ui);
// main menu:
void drawMenuButton(const UIState& ui);
void drawMenu(const UIState& ui);
// game caro:
void drawCaroGame(const MatchState& match, const UIState& ui);
void drawBoard(const MatchState& match, const UIState& ui);
void drawStatusPanel(const MatchState& match); // Vẽ máu, tên nhân vật
// char select
void drawCharSelection(const UIState& ui);
// game over
void drawGameOver(const MatchState& match, const UIState& ui);


// --- HAM RENDER TONG ---
void renderGame(const MatchState& match, const UIState& ui) {
    switch (ui.currentScreen) {
    case MAIN_MENU:
        drawMenu(ui);
        break;

    case CHARACTER_SELECTION:
        drawCharSelection(ui);
        break;

    case GAME_BOARD:
        drawCaroGame(match, ui);
        break;

    case ROUND_OVER:
        break;

    case GAME_OVER:
        drawGameOver(match, ui);
        break;
    }
}

// --- HAI HAM INIT VA UNDLOAD RESUOUCE ---
void initView() {
    menuBg = LoadTexture("./assets/images/menuBg.png");
    plainBg = LoadTexture("./assets/images/plainBg.jpg");
	font8bit = LoadFont("./assets/fonts/Ithaca.ttf");

    SetTextureFilter(font8bit.texture, TEXTURE_FILTER_POINT);
}

void unloadView() {
    UnloadTexture(menuBg);
    UnloadTexture(plainBg);
    UnloadFont(font8bit);

}


// --- CAC HAM LIEN QUAN DEN MAIN MENU ---
void drawMenu(const UIState& ui) {
    drawBackground(ui);

    drawMenuButton(ui);
}

void drawMenuButton(const UIState& ui) {
    vector<string> options = { "New Game", "Exit" }; // them option thi them vao day, nho chinh lai size voi padding cho vua du
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();
    int buttonWidth = 0.2 * screenW;
    int buttonHeight = 0.15 * screenH;
    int totalOptions = options.size();

    // font
    float menuFontSize = buttonHeight * 0.45f;
    float spacing = 2.0f;

    for (int i = 0; i < totalOptions; i++) {
        
        int posX = screenW / 2 - buttonWidth / 2;
        int posY = screenH * 0.45 + i * (buttonHeight + 0.05*screenH); // Cách nhau 70px nếu có nhiều nút

        // Kiểm tra xem nút này có đang được chọn không (dựa vào UIState)
        bool isSelected = ((ui.mainMenuIndex % totalOptions) == i);

        // Đổi màu nếu được chọn
        

        Color bgColor = isSelected ? buttonYellow : buttonDarkPurple;
        Color borderColor = isSelected ? BLACK : buttonDarkPurple;
        Color textColor = isSelected ? buttonDarkPurple: buttonYellow;

        DrawRectangle(posX, posY, buttonWidth, buttonHeight, bgColor);
        DrawRectangleLinesEx({ (float)posX, (float)posY, (float)buttonWidth, (float)buttonHeight }, 4, borderColor); // Viền dày 4px

        int textWidth = MeasureTextEx(font8bit, options[i].c_str(), menuFontSize, spacing).x;

        int textX = posX + (buttonWidth - textWidth) / 2;
        int textY = posY + (buttonHeight - menuFontSize) / 2;

        // Vẽ chữ (Bạn nên LoadFont thay vì dùng font mặc định để ra chất 8-bit)
        Vector2 textSize = MeasureTextEx(font8bit, options[i].c_str(), menuFontSize, spacing);

        DrawTextEx(font8bit, options[i].c_str(), {(float)textX, (float)textY}, menuFontSize, spacing, textColor);
    }
}


// --- CAC HAM LIEN QUAN DEN CHARACTER SELECTION ---
void drawCharSelection(const UIState& ui) {
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();
    drawBackground(ui);

    // Ve ai dang chon
    float headerFontSize = 0.15 * screenH; // 15% chieu cao man hinh
    if (ui.isSelectingX) {
        DrawTextEx(font8bit, "X is choosing...", { 0.02f*screenW , 0 }, headerFontSize, 0.0, buttonYellow);
    }
    else {
        DrawTextEx(font8bit, "O is choosing...", { 0.02f * screenW , 0 }, headerFontSize, 0.0, buttonYellow);
    }
    

    // Ve chon nhan vat gi
    float charNameFontSize = 0.1 * screenH; // 10% chieu cao man hinh
    float charDescFontSize = 0.05 * screenH; // 5% chieu cao man hinh

    Vector2 measureName, measureDesc;
    switch (abs(ui.characterMenuIndex % 3)) {
    case 0:
        measureName = MeasureTextEx(font8bit, "ASSASSIN", charNameFontSize, 0.0);
        measureDesc = MeasureTextEx(font8bit, "Skill: Output damage increase significantly as the round goes on", charDescFontSize, 0.0f);
        DrawTextEx(font8bit, "ASSASSIN", { screenW / 2 - measureName.x / 2, screenH / 2 - measureName.y / 2 }, charNameFontSize, 0.0, buttonYellow);
        DrawTextEx(font8bit, "Skill: Output damage increase significantly as the round goes on", 
            { screenW / 2 - measureDesc.x / 2, screenH / 2 + measureName.y / 2 }, charDescFontSize, 0.0, BLACK);
        break;
    case 1:
        measureName = MeasureTextEx(font8bit, "BRUISER", charNameFontSize, 0.0);
        measureDesc = MeasureTextEx(font8bit, "Skill: Output damage is a moderately high constant value", charDescFontSize, 0.0f);
        DrawTextEx(font8bit, "BRUISER", { screenW / 2 - measureName.x / 2, screenH / 2 - measureName.y / 2 }, charNameFontSize, 0.0, buttonYellow);
        DrawTextEx(font8bit, "Skill: Output damage is always a moderately high constant",
            { screenW / 2 - measureDesc.x / 2, screenH / 2 + measureName.y / 2 }, charDescFontSize, 0.0, BLACK);
        break;
    case 2:
        measureName = MeasureTextEx(font8bit, "VAMPIRE", charNameFontSize, 0.0);
        measureDesc = MeasureTextEx(font8bit, "Skill: Heals a small, random amount of HP when attack", charDescFontSize, 0.0f);
        DrawTextEx(font8bit, "VAMPIRE", { screenW / 2 - measureName.x / 2, screenH / 2 - measureName.y / 2 }, charNameFontSize, 0.0, buttonYellow);
        DrawTextEx(font8bit, "Skill: Heals a small, random amount of HP when attack",
            { screenW / 2 - measureDesc.x / 2, screenH / 2 + measureName.y / 2 }, charDescFontSize, 0.0, BLACK);
        break;

    }

}


// --- CAC HAM LIEN QUAN DEN GAME CARO
//void drawCaroGame(const MatchState& match, const UIState& ui) {
//    drawBackground(ui);
//}
//
//void drawBoard(const MatchState& match, const UIState& ui) {
//
//}
//
//void drawStatusPanel(const MatchState& match) {
//
//}


void drawBackground(const UIState& ui) {
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();

    Texture2D bg;
    switch (ui.currentScreen) {
    case MAIN_MENU:
        bg = menuBg;
        break;
    default:
        bg = plainBg;
    }

    float scale = std::max((float)screenHeight / bg.height,
        (float)screenWidth / bg.width);

    DrawTextureEx(bg, { 0.0f, 0.0f }, 0.0f, scale, WHITE);
}


// Nhom ban co
void drawCaroGame(const MatchState& match, const UIState& ui) {
    drawBackground(ui);

    // Vẽ hai bên trước, vẽ bàn cờ ở giữa sau
    drawStatusPanel(match);
    drawBoard(match, ui);
}

void drawBoard(const MatchState& match, const UIState& ui) {
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();

    // Responsive: Bàn cờ chiếm 80% chiều cao màn hình
    float boardPixelSize = screenH * 0.8f;
    float cellSize = boardPixelSize / BOARD_SIZE;

    // Căn giữa bàn cờ
    float startX = (screenW - boardPixelSize) / 2.0f;
    float startY = (screenH - boardPixelSize) / 2.0f;

    // Vẽ nền bàn cờ (màu tối để nổi bật viền)
    DrawRectangle(startX, startY, boardPixelSize, boardPixelSize, Fade(BLACK, 0.6f));

    // Vẽ các ô cờ
    for (int row = 0; row < BOARD_SIZE; row++) {
        for (int col = 0; col < BOARD_SIZE; col++) {
            float cellX = startX + col * cellSize;
            float cellY = startY + row * cellSize;

            // 1. Vẽ khung từng ô
            DrawRectangleLinesEx({ cellX, cellY, cellSize, cellSize }, 2, buttonDarkPurple);

            // 2. Vẽ Highlight nếu con trỏ đang ở ô này
            if (col == ui.cursorX && row == ui.cursorY) {
                DrawRectangle(cellX, cellY, cellSize, cellSize, Fade(buttonYellow, 0.4f));
                DrawRectangleLinesEx({ cellX, cellY, cellSize, cellSize }, 4, buttonYellow);
            }

            // 3. Vẽ X hoặc O
            PlayerType piece = match.currentRound.board[row][col];
            if (piece != NONE) {
                string text = (piece == X) ? "X" : "O";
                Color pieceColor = (piece == X) ? RED : BLUE;
                float pieceFontSize = cellSize * 0.7f;

                // Căn giữa chữ X/O vào giữa ô cờ
                Vector2 textSize = MeasureTextEx(font8bit, text.c_str(), pieceFontSize, 0);
                float textX = cellX + (cellSize - textSize.x) / 2.0f;
                float textY = cellY + (cellSize - textSize.y) / 2.0f;

                DrawTextEx(font8bit, text.c_str(), { textX, textY }, pieceFontSize, 0, pieceColor);
            }
        }
    }
}

void drawStatusPanel(const MatchState& match) {
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();

    // Font size responsive
    float nameFontSize = screenH * 0.08f;
    float hpFontSize = screenH * 0.04f;

    // Kích thước thanh máu (HP Bar)
    float hpBarWidth = screenW * 0.2f;  // Chiếm 20% chiều ngang
    float hpBarHeight = screenH * 0.05f; // Chiếm 5% chiều cao

    // VỊ TRÍ PANEL PLAYER X (BÊN TRÁI)
    float leftX = screenW * 0.05f;
    float leftY = screenH * 0.2f;

    DrawTextEx(font8bit, "Player X", { leftX, leftY }, nameFontSize, 0, RED);
    // Tính toán máu X
    float hpFillX = hpBarWidth * ((float)match.playerX.health / MAX_HEALTH);
    DrawRectangleLinesEx({ leftX, leftY + nameFontSize + 10, hpBarWidth, hpBarHeight }, 3, BLACK);
    DrawRectangle(leftX, leftY + nameFontSize + 10, hpFillX, hpBarHeight, RED);
    DrawTextEx(font8bit, TextFormat("HP: %d/%d", match.playerX.health, MAX_HEALTH),
        { leftX, leftY + nameFontSize + hpBarHeight + 20 }, hpFontSize, 0, BLACK);


    // VỊ TRÍ PANEL PLAYER O (BÊN PHẢI)
    float rightX = screenW * 0.95f - hpBarWidth; // Neo về bên phải
    float rightY = screenH * 0.2f;

    DrawTextEx(font8bit, "Player O", { rightX, rightY }, nameFontSize, 0, BLUE);
    // Tính toán máu O
    float hpFillO = hpBarWidth * ((float)match.playerO.health / MAX_HEALTH);
    DrawRectangleLinesEx({ rightX, rightY + nameFontSize + 10, hpBarWidth, hpBarHeight }, 3, BLACK);
    DrawRectangle(rightX, rightY + nameFontSize + 10, hpFillO, hpBarHeight, BLUE);
    DrawTextEx(font8bit, TextFormat("HP: %d/%d", match.playerO.health, MAX_HEALTH),
        { rightX, rightY + nameFontSize + hpBarHeight + 20 }, hpFontSize, 0, BLACK);
}

// nhom game over: Lam tam
void drawGameOver(const MatchState& match, const UIState& ui) {
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();

    drawBackground(ui);

    Vector2 measure = MeasureTextEx(font8bit, "GAME OVER", 0.25f*screenH, 0.0);
    Vector2 measureStat= MeasureTextEx(font8bit, "X WINS", 0.15f*screenH, 0.0f);
    DrawTextEx(font8bit, "GAME OVER", { screenW / 2 - measure.x / 2, screenH * 0.4f - measure.y / 2 }, 0.25f*screenH, 0.0, buttonYellow);
    DrawTextEx(font8bit, (match.matchResult == X_WINS) ? "X WINS" : "O WINS",
        { screenW / 2 - measureStat.x / 2, screenH * 0.4f + measure.y / 2 }, 0.15f*screenH, 0.0, BLACK);

}
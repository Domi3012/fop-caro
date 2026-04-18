#include "view.h"
#include "raylib.h"
#include "model.h"
#include "save_manager.h"
#include <string>
#include <vector>

using std::string;
using std::vector;

// Vài màu dùng chung

const Color buttonYellow = GetColor(0xD9E69AFF);
const Color buttonDarkPurple = GetColor(0x240620FF);

// Vài texture dùng chung
static Font font8bit;

// --- PARALLAX BACKGROUND ---
// Tên file các layer rừng, xếp từ xa nhất (index 0) đến gần nhất (index 11)
// Quy ước: số suffix càng nhỏ = càng gần = càng nhanh
static const char* FOREST_LAYER_PATHS[] = {
    "./assets/images/layered_forest/Layer_0011_0.png",  // xa nhất - chậm nhất
    "./assets/images/layered_forest/Layer_0010_1.png",
    "./assets/images/layered_forest/Layer_0009_2.png",
    "./assets/images/layered_forest/Layer_0008_3.png",
    "./assets/images/layered_forest/Layer_0007_Lights.png",
    "./assets/images/layered_forest/Layer_0006_4.png",
    "./assets/images/layered_forest/Layer_0005_5.png",
    "./assets/images/layered_forest/Layer_0004_Lights.png",
    "./assets/images/layered_forest/Layer_0003_6.png",
    "./assets/images/layered_forest/Layer_0002_7.png",
    "./assets/images/layered_forest/Layer_0001_8.png",
    "./assets/images/layered_forest/Layer_0000_9.png",  // gần nhất - nhanh nhất
};
static const int FOREST_LAYER_COUNT = 12;

struct ParallaxLayer {
    Texture2D texture;
    float scrollX;   // offset cuộn hiện tại (pixel, âm = đã cuộn sang trái)
    float speed;     // pixel/giây
};

static ParallaxLayer forestLayers[FOREST_LAYER_COUNT];

// Vẽ parallax background cho main menu, gọi mỗi frame
void drawParallaxBackground();

// Declaration cua vai ham noi bo
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
    ClearBackground(BLACK);
    switch (ui.currentScreen) {
    case MAIN_MENU:
        drawMenu(ui);
        break;

    case CHARACTER_SELECTION:
        drawCharSelection(ui);
        break;
    
    case LOAD_GAME:
        drawLoadGameScreen(ui, getSaveFilesList());
        break;
    
    case SETTINGS:
        drawSettingsScreen(ui);
        break;

    case GAME_BOARD:
        drawCaroGame(match, ui);
        break;

    case ROUND_OVER:
    {
        drawCaroGame(match, ui);

        int screenW = GetScreenWidth();
        int screenH = GetScreenHeight();
        float fontSize = screenH * 0.1f;
        // match.currentRound.result luon duoc set truoc khi chuyen sang ROUND_OVER
        const char* msg = (match.currentRound.result == X_WINS) ? "X WINS THIS ROUND!" :
            (match.currentRound.result == O_WINS) ? "O WINS THIS ROUND!" :
            "DRAW!";
        Vector2 measure = MeasureTextEx(font8bit, msg, fontSize, 0);
        DrawRectangle(0, screenH * 0.4f - 20, screenW, fontSize + 40, Fade(BLACK, 0.75f));
        DrawTextEx(font8bit, msg,
            { screenW / 2.0f - measure.x / 2, screenH * 0.4f }, fontSize, 0, buttonYellow);
        break;
    }
    case GAME_OVER:
        drawGameOver(match, ui);
        break;
    }
}

// --- HAI HAM INIT VA UNDLOAD RESUOUCE ---
void initView() {
    font8bit = LoadFont("./assets/fonts/Ithaca.ttf");

    SetTextureFilter(font8bit.texture, TEXTURE_FILTER_POINT);

    // Load các layer rừng parallax
    // Tốc độ: layer 0 (xa nhất) = 10 px/s, tăng dần, layer 11 (gần nhất) = 120 px/s
    for (int i = 0; i < FOREST_LAYER_COUNT; i++) {
        forestLayers[i].texture = LoadTexture(FOREST_LAYER_PATHS[i]);
        SetTextureFilter(forestLayers[i].texture, TEXTURE_FILTER_BILINEAR);
        forestLayers[i].scrollX = 0.0f;
        // Nội suy tuyến tính: layer 0 chậm, layer 11 nhanh, x 0.25 để chậm lại
        float t = (float)i / (FOREST_LAYER_COUNT - 1); // 0.0 -> 1.0
        forestLayers[i].speed = (10.0f + t * 110.0f) * 0.25f; // 2.5 -> 30 px/s
    }
}

void unloadView() {
    // Unload các layer rừng
    for (int i = 0; i < FOREST_LAYER_COUNT; i++) {
        UnloadTexture(forestLayers[i].texture);
    }

    UnloadFont(font8bit);

}


// --- PARALLAX BACKGROUND ---
void drawParallaxBackground() {
    float dt = GetFrameTime();
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();

    for (int i = 0; i < FOREST_LAYER_COUNT; i++) {
        ParallaxLayer& layer = forestLayers[i];
        Texture2D& tex = layer.texture;

        // Cap nhat offset cuon
        layer.scrollX -= layer.speed * dt;

        // Source rect: chi lay 2/3 phia duoi cua anh goc (bo 1/3 tren)
        float srcY = tex.height / 3.0f;
        float srcH = tex.height * (2.0f / 3.0f);
        float srcW = (float)tex.width;

        // Dest width: giu aspect ratio cua vung crop, fill full chieu cao man hinh
        float scaledW = srcW / srcH * screenH;

        // Wrap
        if (layer.scrollX <= -scaledW)
            layer.scrollX += scaledW;

        // Ve du so ban de phu kin toan man hinh
        int copies = (int)((float)screenW / scaledW) + 2;
        for (int c = 0; c < copies; c++) {
            Rectangle src  = { 0, srcY, srcW, srcH };
            Rectangle dest = { layer.scrollX + c * scaledW, 0, scaledW, (float)screenH };
            DrawTexturePro(tex, src, dest, { 0, 0 }, 0.0f, WHITE);
        }
    }
}

// --- CAC HAM LIEN QUAN DEN MAIN MENU ---
void drawMenu(const UIState& ui) {
    drawParallaxBackground();
    drawMenuButton(ui);
}

void drawMenuButton(const UIState& ui) {
    
    vector<string> options = { "New Game", "Load Game", "Settings", "Exit" }; 
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();
    int totalOptions = options.size();

    // 1. Tính toán không gian hiển thị (Y-axis)
    float startY = screenH * 0.45f;               // Vị trí bắt đầu vẽ (45% màn hình từ trên xuống)
    float marginBottom = screenH * 0.05f;         // Chừa lề dưới cùng 5% màn hình
    float availableHeight = screenH - startY - marginBottom; // Tổng không gian dọc còn lại cho các nút

    // 2. Tính toán khoảng cách (gap) và chiều cao nút (buttonHeight)
    float gap = screenH * 0.03f;                  // Khoảng cách giữa các nút (3% màn hình)
    float totalGapSpace = (totalOptions > 1) ? (totalOptions - 1) * gap : 0;
    
    // Chia đều không gian còn lại cho tổng số nút
    float buttonHeight = (availableHeight - totalGapSpace) / totalOptions;
    
    // Giới hạn chiều cao tối đa để nút không bị quá to (nếu sau này bớt nút đi)
    float maxButtonHeight = screenH * 0.15f; 
    if (buttonHeight > maxButtonHeight) {
        buttonHeight = maxButtonHeight;
    }

    // Chiều rộng nút (nới lên 0.3 để vừa chữ "Load Game" / "Settings")
    float buttonWidth = screenW * 0.2f; 
    
    // 3. Tính toán font
    float menuFontSize = buttonHeight * 0.45f;
    float spacing = 2.0f;

    // 4. Vòng lặp vẽ các nút
    for (int i = 0; i < totalOptions; i++) {
        
        float posX = screenW / 2.0f - buttonWidth / 2.0f;
        float posY = startY + i * (buttonHeight + gap);

        // Kiểm tra xem nút này có đang được chọn không (dựa vào UIState)
        bool isSelected = ((ui.mainMenuIndex % totalOptions) == i);

        // Đổi màu nếu được chọn
        Color bgColor = isSelected ? buttonYellow : buttonDarkPurple;
        Color borderColor = isSelected ? BLACK : buttonDarkPurple;
        Color textColor = isSelected ? buttonDarkPurple: buttonYellow;

        // Vẽ background và viền
        DrawRectangle((int)posX, (int)posY, (int)buttonWidth, (int)buttonHeight, bgColor);
        DrawRectangleLinesEx({ posX, posY, buttonWidth, buttonHeight }, 4, borderColor); // Viền dày 4px

        // Đo kích thước chữ để căn giữa nút
        Vector2 textSize = MeasureTextEx(font8bit, options[i].c_str(), menuFontSize, spacing);
        float textX = posX + (buttonWidth - textSize.x) / 2.0f;
        float textY = posY + (buttonHeight - textSize.y) / 2.0f;

        // Vẽ chữ
        DrawTextEx(font8bit, options[i].c_str(), { textX, textY }, menuFontSize, spacing, textColor);
    }
}


// --- CAC HAM LIEN QUAN DEN CHARACTER SELECTION ---
void drawCharSelection(const UIState& ui) {
    drawParallaxBackground();

    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();

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



// Nhom ban co
void drawCaroGame(const MatchState& match, const UIState& ui) {
    
    drawParallaxBackground();

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

    drawParallaxBackground();

    Vector2 measure = MeasureTextEx(font8bit, "GAME OVER", 0.25f*screenH, 0.0);
    Vector2 measureStat= MeasureTextEx(font8bit, "X WINS", 0.15f*screenH, 0.0f);
    DrawTextEx(font8bit, "GAME OVER", { screenW / 2 - measure.x / 2, screenH * 0.4f - measure.y / 2 }, 0.25f*screenH, 0.0, buttonYellow);
    DrawTextEx(font8bit, (match.matchResult == X_WINS) ? "X WINS" : "O WINS",
        { screenW / 2 - measureStat.x / 2, screenH * 0.4f + measure.y / 2 }, 0.15f*screenH, 0.0, BLACK);

}

void drawLoadGameScreen(const UIState& ui, const std::vector<std::string>& saveFiles) {
    drawParallaxBackground();
    
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();

    DrawTextEx(font8bit, "SELECT SAVE FILE", { screenW / 2.0f - 180, screenH * 0.1f }, 40, 2, buttonYellow);

    if (saveFiles.empty()) {
        DrawTextEx(font8bit, "No saves found in /saves folder.", { screenW / 2.0f - 250, screenH / 2.0f }, 30, 2, GRAY);
        DrawTextEx(font8bit, "Press ESC to return.", { screenW / 2.0f - 150, screenH / 2.0f + 50 }, 20, 2, DARKGRAY);
        return;
    }

    float startY = screenH * 0.3f;
    for (size_t i = 0; i < saveFiles.size(); i++) {
        bool isSelected = ((int)i == ui.loadMenuIndex);
        Color textColor = isSelected ? buttonDarkPurple : buttonYellow;
        Color bgColor = isSelected ? buttonYellow : BLANK;

        float yPos = startY + i * 50;
        DrawRectangle(screenW / 2.0f - 200, yPos - 5, 400, 40, bgColor);
        DrawTextEx(font8bit, saveFiles[i].c_str(), { screenW / 2.0f - 180, yPos }, 30, 2, textColor);
    }
}

void drawSettingsScreen(const UIState& ui) {
    // mock
    drawParallaxBackground();
}
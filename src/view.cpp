#include "view.h"
#include "raylib.h"
#include "model.h"
#include "save_manager.h"
#include <string>
#include <vector>
#include <cmath>
#include "audio_manager.h"

using std::string;
using std::vector;

// Vài màu dùng chung

const Color buttonYellow = GetColor(0xb8dbc0FF);
const Color buttonDarkPurple = GetColor(0x1A2421FF);

// Vài texture dùng chung
static Font font8bit;

// --- PARALLAX BACKGROUND ---
// Tên file các layer rừng, xếp từ xa nhất (index 0) đến gần nhất (index 11)
// Quy ước: số suffix càng nhỏ = càng gần = càng nhanh
static const char *FOREST_LAYER_PATHS[] = {
    "./assets/images/layered_forest/Layer_0011_0.png", // xa nhất - chậm nhất
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
    "./assets/images/layered_forest/Layer_0000_9.png", // gần nhất - nhanh nhất
};
static const int FOREST_LAYER_COUNT = 12;

struct ParallaxLayer
{
    Texture2D texture;
    float scrollX; // offset cuộn hiện tại (pixel, âm = đã cuộn sang trái)
    float speed;   // pixel/giây
};

static ParallaxLayer forestLayers[FOREST_LAYER_COUNT];

// Vẽ parallax background cho main menu, gọi mỗi frame
void drawParallaxBackground(float speedMultiplier = 1.0f);

// Resolution options for settings menu
struct ResolutionOption
{
    int width;
    int height;
    const char *label;
};

static const ResolutionOption RESOLUTIONS[] =
    {
        {1280, 720, "1280x720 (16:9)"},
        {1366, 768, "1366x768 (16:9)"},
        {1600, 900, "1600x900 (16:9)"},
        {1920, 1080, "1920x1080 (16:9)"},
        {2560, 1440, "2560x1440 (16:9)"}};

static const int RESOLUTION_COUNT = sizeof(RESOLUTIONS) / sizeof(RESOLUTIONS[0]);

static bool isResolutionAllowed(int width, int height)
{
    int monitor = GetCurrentMonitor();
    return width <= GetMonitorWidth(monitor) && height <= GetMonitorHeight(monitor);
}

// --- HAM RENDER TONG ---
void renderGame(const MatchState &match, const UIState &ui)
{
    ClearBackground(BLACK);
    switch (ui.currentScreen)
    {
    case MAIN_MENU:
        drawMenu(ui);
        break;

    case MODE_SELECTION:
        drawModeSelectionScreen(ui);
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

    case GAME_INTRO:
        drawGameIntro(match, ui);
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
        const char *msg = (match.currentRound.result == X_WINS) ? "X WINS THIS ROUND!" : (match.currentRound.result == O_WINS) ? "O WINS THIS ROUND!"
                                                                                                                               : "DRAW!";
        Vector2 measure = MeasureTextEx(font8bit, msg, fontSize, 0);
        DrawRectangle(0, screenH * 0.4f - 20, screenW, fontSize + 40, Fade(BLACK, 0.75f));
        DrawTextEx(font8bit, msg,
                   {screenW / 2.0f - measure.x / 2, screenH * 0.4f}, fontSize, 0, buttonYellow);
        break;
    }
    case GAME_OVER:
        drawGameOver(match, ui);
        break;

    case BOT_DIFFICULTY_SELECTION:
        drawBotDifficultyScreen(ui);
        break;
    }
}
// --- HAI HAM INIT VA UNDLOAD RESUOUCE ---
void initView()
{
    font8bit = LoadFont("./assets/fonts/Ithaca.ttf");

    SetTextureFilter(font8bit.texture, TEXTURE_FILTER_POINT);

    // Load các layer rừng parallax
    // Tốc độ: layer 0 (xa nhất) = 10 px/s, tăng dần, layer 11 (gần nhất) = 120 px/s
    for (int i = 0; i < FOREST_LAYER_COUNT; i++)
    {
        forestLayers[i].texture = LoadTexture(FOREST_LAYER_PATHS[i]);
        SetTextureFilter(forestLayers[i].texture, TEXTURE_FILTER_BILINEAR);
        forestLayers[i].scrollX = 0.0f;
        // Nội suy tuyến tính: layer 0 chậm, layer 11 nhanh, x 0.25 để chậm lại
        float t = (float)i / (FOREST_LAYER_COUNT - 1);        // 0.0 -> 1.0
        forestLayers[i].speed = (10.0f + t * 110.0f) * 0.25f; // 2.5 -> 30 px/s
    }
}

void unloadView()
{
    // Unload các layer rừng
    for (int i = 0; i < FOREST_LAYER_COUNT; i++)
    {
        UnloadTexture(forestLayers[i].texture);
    }

    UnloadFont(font8bit);
}

struct BoardLayout
{
    float boardPixelSize;
    float cellSize;
    float startX;
    float startY;
};

static BoardLayout getBoardLayout(int screenW, int screenH)
{
    // Chừa chỗ cho panel trái/phải + turn banner phía trên
    float reservedSide = screenW * 0.28f;
    float reservedTop = screenH * 0.16f;
    float reservedBottom = screenH * 0.08f;

    float maxBoardW = screenW - reservedSide * 2.0f;
    float maxBoardH = screenH - reservedTop - reservedBottom;

    float boardPixelSize = std::min(maxBoardW, maxBoardH);
    float cellSize = boardPixelSize / BOARD_SIZE;

    float startX = (screenW - boardPixelSize) / 2.0f;
    float startY = reservedTop + (maxBoardH - boardPixelSize) / 2.0f;

    return {boardPixelSize, cellSize, startX, startY};
}

void drawCharacters(float shiftX)
{
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();

    // Ve placeholder hinh chu nhat
    float charW = screenW * 0.08f;
    float charH = screenH * 0.20f;

    float baseGroundY = screenH * 0.85f; // Platform khoang 85% manh hinh

    float posX_X = screenW * 0.15f + shiftX - charW / 2.0f;
    float posX_O = screenW * 0.85f + shiftX - charW / 2.0f;

    DrawRectangle(posX_X, baseGroundY - charH, charW, charH, Fade(RED, 0.8f));
    DrawRectangleLinesEx({posX_X, baseGroundY - charH, charW, charH}, 3, WHITE);
    DrawTextEx(font8bit, "X", {posX_X + charW / 2.0f - 15, baseGroundY - charH / 2.0f - 20}, 40, 2, WHITE);

    DrawRectangle(posX_O, baseGroundY - charH, charW, charH, Fade(BLUE, 0.8f));
    DrawRectangleLinesEx({posX_O, baseGroundY - charH, charW, charH}, 3, WHITE);
    DrawTextEx(font8bit, "O", {posX_O + charW / 2.0f - 15, baseGroundY - charH / 2.0f - 20}, 40, 2, WHITE);
}

// --- PARALLAX BACKGROUND ---
void drawParallaxBackground(float speedMultiplier)
{
    float dt = GetFrameTime();
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();

    for (int i = 0; i < FOREST_LAYER_COUNT; i++)
    {
        ParallaxLayer &layer = forestLayers[i];
        Texture2D &tex = layer.texture;

        // Cap nhat offset cuon
        layer.scrollX -= layer.speed * speedMultiplier * dt;

        // Source rect: chi lay 2/3 phia duoi cua anh goc (bo 1/3 tren)
        float srcY = tex.height / 3.0f;
        float srcH = tex.height * (2.0f / 3.0f);
        float srcW = (float)tex.width;

        // Dest width: giu aspect ratio cua vung crop, fill full chieu cao man hinh
        float scaledW = srcW / srcH * screenH;

        // Wrap
        while (layer.scrollX <= -scaledW)
            layer.scrollX += scaledW;

        // Ve du so ban de phu kin toan man hinh
        int copies = (int)((float)screenW / scaledW) + 2;
        for (int c = 0; c < copies; c++)
        {
            Rectangle src = {0, srcY, srcW, srcH};
            Rectangle dest = {layer.scrollX + c * scaledW, 0, scaledW, (float)screenH};
            DrawTexturePro(tex, src, dest, {0, 0}, 0.0f, WHITE);
        }
    }
}

// --- CAC HAM LIEN QUAN DEN MAIN MENU ---
void drawMenu(const UIState &ui)
{
    drawParallaxBackground(1.0f);

    // Ve title RGBCaro idle loop dao dong
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();
    float time = GetTime();

    float titleY = screenH * 0.12f + std::sin(time * 2.5f) * 15.0f; // len xuong
    const char *titleText = "RGB Caro";
    float titleSize = screenH * 0.15f;
    Vector2 titleMeasure = MeasureTextEx(font8bit, titleText, titleSize, 10);

    // Bong mo shadow
    DrawTextEx(font8bit, titleText, {screenW / 2.0f - titleMeasure.x / 2.0f + 6, titleY + 6}, titleSize, 10, Fade(BLACK, 0.6f));
    DrawTextEx(font8bit, titleText, {screenW / 2.0f - titleMeasure.x / 2.0f, titleY}, titleSize, 10, WHITE); // White or Yellow

    drawMenuButton(ui);
}

void drawMenuButton(const UIState &ui)
{

    vector<string> options = {"New Game", "Load Game", "Settings", "Exit"};
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();
    int totalOptions = options.size();

    // 1. Tính toán không gian hiển thị (Y-axis)
    float startY = screenH * 0.35f;                          // Vị trí bắt đầu vẽ (35% màn hình từ trên xuống)
    float marginBottom = screenH * 0.05f;                    // Chừa lề dưới cùng 5% màn hình
    float availableHeight = screenH - startY - marginBottom; // Tổng không gian dọc còn lại cho các nút

    // 2. Tính toán khoảng cách (gap) và chiều cao nút (buttonHeight)
    float gap = screenH * 0.03f; // Khoảng cách giữa các nút (3% màn hình)
    float totalGapSpace = (totalOptions > 1) ? (totalOptions - 1) * gap : 0;

    // Chia đều không gian còn lại cho tổng số nút
    float buttonHeight = (availableHeight - totalGapSpace) / totalOptions;

    // Giới hạn chiều cao tối đa để nút không bị quá to (nếu sau này bớt nút đi)
    float maxButtonHeight = screenH * 0.15f;
    if (buttonHeight > maxButtonHeight)
    {
        buttonHeight = maxButtonHeight;
    }

    // Chiều rộng nút (nới lên 0.3 để vừa chữ "Load Game" / "Settings")
    float buttonWidth = screenW * 0.2f;

    // 3. Tính toán font
    float menuFontSize = buttonHeight * 0.45f;
    float spacing = 2.0f;

    // 4. Vòng lặp vẽ các nút
    for (int i = 0; i < totalOptions; i++)
    {

        float posX = screenW / 2.0f - buttonWidth / 2.0f;
        float posY = startY + i * (buttonHeight + gap);

        // Kiểm tra xem nút này có đang được chọn không (dựa vào UIState)
        bool isSelected = ((ui.mainMenuIndex % totalOptions) == i);

        // Đổi màu nếu được chọn
        Color bgColor = isSelected ? buttonYellow : buttonDarkPurple;
        Color borderColor = isSelected ? BLACK : buttonDarkPurple;
        Color textColor = isSelected ? buttonDarkPurple : buttonYellow;

        // Vẽ background và viền
        DrawRectangle((int)posX, (int)posY, (int)buttonWidth, (int)buttonHeight, bgColor);
        DrawRectangleLinesEx({posX, posY, buttonWidth, buttonHeight}, 4, borderColor); // Viền dày 4px

        // Đo kích thước chữ để căn giữa nút
        Vector2 textSize = MeasureTextEx(font8bit, options[i].c_str(), menuFontSize, spacing);
        float textX = posX + (buttonWidth - textSize.x) / 2.0f;
        float textY = posY + (buttonHeight - textSize.y) / 2.0f;

        // Vẽ chữ
        DrawTextEx(font8bit, options[i].c_str(), {textX, textY}, menuFontSize, spacing, textColor);
    }
}

void drawModeSelectionScreen(const UIState &ui)
{
    drawParallaxBackground(1.0f);
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();

    float startY = screenH * 0.4f;
    float gap = 80.0f;
    const char *modes[] = {"Player vs Player", "Player vs Environment (Bot)"};

    DrawTextEx(font8bit, "SELECT GAME MODE", {screenW / 2.0f - 200, screenH * 0.2f}, 50, 2, buttonYellow);

    for (int i = 0; i < 2; i++)
    {
        bool isSelected = (i == ui.modeMenuIndex);
        Color textColor = isSelected ? buttonDarkPurple : buttonYellow;
        Color bgColor = isSelected ? buttonYellow : BLANK;

        float posY = startY + i * gap;
        DrawRectangle(screenW / 2.0f - 300, posY - 5, 600, 50, bgColor);
        DrawTextEx(font8bit, modes[i], {screenW / 2.0f - 280, posY + 5}, 35, 2, textColor);
    }
}

// --- CAC HAM LIEN QUAN DEN CHARACTER SELECTION ---
void drawCharSelection(const UIState &ui)
{
    drawParallaxBackground(1.0f);

    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();

    // Glassmorphism panel
    float panelW = screenW * 0.7f;
    float panelH = screenH * 0.7f;
    float panelX = (screenW - panelW) / 2.0f;
    float panelY = (screenH - panelH) / 2.0f;

    DrawRectangle(panelX, panelY, panelW, panelH, Fade(BLACK, 0.7f));
    DrawRectangleLinesEx({panelX, panelY, panelW, panelH}, 4, buttonYellow);

    // Header
    const char *headerText = ui.isSelectingX ? "PLAYER X IS CHOOSING" : "PLAYER O IS CHOOSING";
    Color headerColor = ui.isSelectingX ? RED : BLUE;
    float headerFontSize = screenH * 0.08f;
    Vector2 headerSize = MeasureTextEx(font8bit, headerText, headerFontSize, 2);
    DrawTextEx(font8bit, headerText, {screenW / 2.0f - headerSize.x / 2.0f, panelY + screenH * 0.05f}, headerFontSize, 2, headerColor);

    // Character Box Placeholder
    float boxW = panelW * 0.3f;
    float boxH = panelH * 0.4f;
    float boxX = screenW / 2.0f - boxW / 2.0f;
    float boxY = panelY + screenH * 0.18f;

    Color charColor;
    const char *charName;
    const char *charDesc;

    switch (ui.characterMenuIndex)
    {
    case 1:
        charName = "ASSASSIN";
        charColor = Fade(PURPLE, 0.8f);
        charDesc = "Skill: DMG increases significantly as the round goes on";
        break;
    case 2:
        charName = "BRUISER";
        charColor = Fade(ORANGE, 0.8f);
        charDesc = "Skill: DMG is always a moderately high constant";
        break;
    case 3:
    default:
        charName = "VAMPIRE";
        charColor = Fade(DARKGREEN, 0.8f);
        charDesc = "Skill: Heals a small, random amount of HP when attack";
        break;
    }

    // Draw box
    DrawRectangle(boxX, boxY, boxW, boxH, charColor);
    DrawRectangleLinesEx({boxX, boxY, boxW, boxH}, 5, WHITE); // Khung net dut
    DrawTextEx(font8bit, "?", {boxX + boxW / 2.0f - 20, boxY + boxH / 2.0f - 40}, 80, 2, Fade(WHITE, 0.5f));

    // Char Name
    float nameFontSize = screenH * 0.05f;
    Vector2 nameSize = MeasureTextEx(font8bit, charName, nameFontSize, 2);
    DrawTextEx(font8bit, charName, {screenW / 2.0f - nameSize.x / 2.0f, boxY + boxH + screenH * 0.02f}, nameFontSize, 2, buttonYellow);

    // Description
    float descFontSize = screenH * 0.035f;
    Vector2 descSize = MeasureTextEx(font8bit, charDesc, descFontSize, 1);
    DrawTextEx(font8bit, charDesc, {screenW / 2.0f - descSize.x / 2.0f, boxY + boxH + screenH * 0.09f}, descFontSize, 1, Fade(WHITE, 0.9f));

    // Indicators (● ● ○)
    float dotGap = 40.0f;
    float dotsStartX = screenW / 2.0f - dotGap;
    float dotsY = panelY + panelH - screenH * 0.08f;
    for (int i = 1; i <= 3; i++)
    {
        if (i == ui.characterMenuIndex)
            DrawCircle(dotsStartX + (i - 1) * dotGap, dotsY, 10.0f, buttonYellow);
        else
            DrawCircleLines(dotsStartX + (i - 1) * dotGap, dotsY, 10.0f, Fade(WHITE, 0.5f));
    }

    // Footer Navigation
    const char *footerText = "LEFT/RIGHT: Navigate  |  ENTER: Confirm  |  ESC: Back";
    float footerFontSize = screenH * 0.025f;
    Vector2 footerSize = MeasureTextEx(font8bit, footerText, footerFontSize, 1);
    DrawTextEx(font8bit, footerText, {screenW / 2.0f - footerSize.x / 2.0f, panelY + panelH + screenH * 0.02f}, footerFontSize, 1, Fade(WHITE, 0.6f));
}

void drawGameIntro(const MatchState &match, const UIState &ui)
{
    float totalTime = 3.5f;
    float totalDistance = (float)GetScreenWidth() * 5.0f;
    float p = ui.roundOverTimer / totalTime;
    if (p > 1.0f)
        p = 1.0f;

    float ep_prime = 30.0f * p * p * (1.0f - p) * (1.0f - p);
    float velocity = ep_prime * totalDistance / totalTime;
    float speedMultiplier = velocity / 30.0f;

    drawParallaxBackground(speedMultiplier);
    drawCharacters(ui.introCamX);
}

// Nhom ban co
void drawCaroGame(const MatchState &match, const UIState &ui)
{

    drawParallaxBackground(0.0f); // Lock background
    drawCharacters(0.0f);         // Ve character o trang thai Dung im

    // Vẽ hai bên trước, vẽ bàn cờ ở giữa sau
    drawStatusPanel(match);
    drawBoard(match, ui);

    // Hien thi Pause Menu de len khung game
    if (ui.isPaused)
    {
        int screenW = GetScreenWidth();
        int screenH = GetScreenHeight();

        // Lam toi toan man hinh
        DrawRectangle(0, 0, screenW, screenH, Fade(BLACK, 0.7f));

        float panelW = 400; // Hoac screenW * 0.35f
        float panelH = 300; // Hoac screenH * 0.40f
        float panelX = screenW / 2.0f - panelW / 2.0f;
        float panelY = screenH / 2.0f - panelH / 2.0f;

        // Vẽ panel
        DrawRectangle(panelX, panelY, panelW, panelH, buttonDarkPurple);
        DrawRectangleLinesEx({panelX, panelY, panelW, panelH}, 4, buttonYellow);

        // Header
        const char *title = "PAUSED";
        float titleSize = 50.0f;
        Vector2 titleMeasure = MeasureTextEx(font8bit, title, titleSize, 2);
        DrawTextEx(font8bit, title, {screenW / 2.0f - titleMeasure.x / 2.0f, panelY + 30}, titleSize, 2, WHITE);

        // Menu options
        const char *options[] = {"Save Game", "Exit to Menu"};
        float gap = 70.0f;
        float startY = panelY + 120.0f;

        for (int i = 0; i < 2; ++i)
        {
            Color color = (i == ui.pauseMenuIndex) ? buttonDarkPurple : buttonYellow;
            Color bgColor = (i == ui.pauseMenuIndex) ? buttonYellow : BLANK;

            float optY = startY + i * gap;
            DrawRectangle(screenW / 2.0f - 150, optY - 10, 300, 50, bgColor);

            Vector2 optSize = MeasureTextEx(font8bit, options[i], 30.0f, 2);
            DrawTextEx(font8bit, options[i], {screenW / 2.0f - optSize.x / 2.0f, optY}, 30.0f, 2, color);
        }
    }
}

void drawBoard(const MatchState &match, const UIState &ui)
{
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();

    BoardLayout layout = getBoardLayout(screenW, screenH);

    float boardPixelSize = layout.boardPixelSize;
    float cellSize = layout.cellSize;
    float startX = layout.startX;
    float startY = layout.startY;

    DrawRectangle(startX, startY, boardPixelSize, boardPixelSize, Fade(BLACK, 0.6f));

    for (int row = 0; row < BOARD_SIZE; row++)
    {
        for (int col = 0; col < BOARD_SIZE; col++)
        {
            float cellX = startX + col * cellSize;
            float cellY = startY + row * cellSize;

            DrawRectangleLinesEx({cellX, cellY, cellSize, cellSize}, 2, buttonDarkPurple);

            if (col == ui.cursorX && row == ui.cursorY)
            {
                DrawRectangle(cellX, cellY, cellSize, cellSize, Fade(buttonYellow, 0.4f));
                DrawRectangleLinesEx({cellX, cellY, cellSize, cellSize}, 4, buttonYellow);
            }

            PlayerType piece = match.currentRound.board[row][col];
            if (piece != NONE)
            {
                std::string text = (piece == X) ? "X" : "O";
                Color pieceColor = (piece == X) ? RED : BLUE;
                float pieceFontSize = cellSize * 0.7f;

                Vector2 textSize = MeasureTextEx(font8bit, text.c_str(), pieceFontSize, 0);
                float textX = cellX + (cellSize - textSize.x) / 2.0f;
                float textY = cellY + (cellSize - textSize.y) / 2.0f;

                DrawTextEx(font8bit, text.c_str(), {textX, textY}, pieceFontSize, 0, pieceColor);
            }
        }
    }
}
void drawTurnBanner(const MatchState &match)
{
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();

    BoardLayout layout = getBoardLayout(screenW, screenH);

    const char *turnText = (match.currentRound.toMove == X) ? "TURN: PLAYER X" : "TURN: PLAYER O";
    Color turnColor = (match.currentRound.toMove == X) ? RED : BLUE;

    float fontSize = screenH * 0.04f;
    Vector2 turnSize = MeasureTextEx(font8bit, turnText, fontSize, 0);

    float paddingX = 28.0f;
    float paddingY = 12.0f;

    float turnBoxW = turnSize.x + paddingX * 2.0f;
    float turnBoxH = turnSize.y + paddingY * 2.0f;

    float turnBoxX = screenW / 2.0f - turnBoxW / 2.0f;
    float turnBoxY = layout.startY - turnBoxH - 16.0f; // cách hẳn bàn cờ ra

    DrawRectangle((int)turnBoxX, (int)turnBoxY, (int)turnBoxW, (int)turnBoxH, Fade(BLACK, 0.75f));
    DrawRectangleLinesEx({turnBoxX, turnBoxY, turnBoxW, turnBoxH}, 3, turnColor);

    DrawTextEx(font8bit,
               turnText,
               {turnBoxX + paddingX, turnBoxY + paddingY},
               fontSize, 0, RAYWHITE);
}
void drawStatusPanel(const MatchState &match)
{
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();

    float nameFontSize = screenH * 0.08f;
    float hpFontSize = screenH * 0.035f;
    float turnFontSize = screenH * 0.04f;

    float hpBarWidth = screenW * 0.20f;
    float hpBarHeight = screenH * 0.045f;

    Color panelBg = Fade(BLACK, 0.50f);
    Color hpBg = Fade(RAYWHITE, 0.16f);
    Color hpBorder = Fade(WHITE, 0.90f);
    Color infoColor = RAYWHITE;

    float leftX = screenW * 0.05f;
    float leftY = screenH * 0.16f;

    float rightX = screenW * 0.95f - hpBarWidth;
    float rightY = screenH * 0.16f;

    DrawRectangle((int)(leftX - 20), (int)(leftY - 20), (int)(hpBarWidth + 40), 150, panelBg);
    DrawRectangle((int)(rightX - 20), (int)(rightY - 20), (int)(hpBarWidth + 40), 150, panelBg);

    DrawTextEx(font8bit, "Player X", {leftX, leftY}, nameFontSize, 0, RED);
    float hpFillX = hpBarWidth * ((float)match.playerX.health / MAX_HEALTH);
    DrawRectangle((int)leftX, (int)(leftY + nameFontSize + 12), (int)hpBarWidth, (int)hpBarHeight, hpBg);
    DrawRectangle((int)leftX, (int)(leftY + nameFontSize + 12), (int)hpFillX, (int)hpBarHeight, RED);
    DrawRectangleLinesEx({leftX, leftY + nameFontSize + 12, hpBarWidth, hpBarHeight}, 3, hpBorder);
    DrawTextEx(font8bit,
               TextFormat("HP: %d/%d", match.playerX.health, MAX_HEALTH),
               {leftX, leftY + nameFontSize + hpBarHeight + 24},
               hpFontSize, 0, infoColor);

    DrawTextEx(font8bit, "Player O", {rightX, rightY}, nameFontSize, 0, BLUE);
    float hpFillO = hpBarWidth * ((float)match.playerO.health / MAX_HEALTH);
    DrawRectangle((int)rightX, (int)(rightY + nameFontSize + 12), (int)hpBarWidth, (int)hpBarHeight, hpBg);
    DrawRectangle((int)rightX, (int)(rightY + nameFontSize + 12), (int)hpFillO, (int)hpBarHeight, BLUE);
    DrawRectangleLinesEx({rightX, rightY + nameFontSize + 12, hpBarWidth, hpBarHeight}, 3, hpBorder);
    DrawTextEx(font8bit,
               TextFormat("HP: %d/%d", match.playerO.health, MAX_HEALTH),
               {rightX, rightY + nameFontSize + hpBarHeight + 24},
               hpFontSize, 0, infoColor);

    const char *turnText;
    Color turnColor;

    if (match.currentRound.toMove == X)
    {
        turnText = "TURN: PLAYER X";
        turnColor = RED;
    }
    else
    {
        turnText = "TURN: PLAYER O";
        turnColor = BLUE;
    }

    Vector2 turnSize = MeasureTextEx(font8bit, turnText, turnFontSize, 0);
    float turnBoxW = turnSize.x + 60.0f;
    float turnBoxH = turnSize.y + 24.0f;
    float turnBoxX = screenW / 2.0f - turnBoxW / 2.0f;
    float turnBoxY = screenH * 0.06f;

    DrawRectangle((int)turnBoxX, (int)turnBoxY, (int)turnBoxW, (int)turnBoxH, Fade(BLACK, 0.60f));
    DrawRectangleLinesEx({turnBoxX, turnBoxY, turnBoxW, turnBoxH}, 3, turnColor);
    DrawTextEx(font8bit,
               turnText,
               {turnBoxX + 30.0f, turnBoxY + 12.0f},
               turnFontSize, 0, RAYWHITE);
}
// nhom game over: Lam tam
void drawGameOver(const MatchState &match, const UIState &ui)
{

    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();

    drawParallaxBackground(0.0f); // Lock background
    drawCharacters(0.0f);

    Vector2 measure = MeasureTextEx(font8bit, "GAME OVER", 0.25f * screenH, 0.0);
    Vector2 measureStat = MeasureTextEx(font8bit, "X WINS", 0.15f * screenH, 0.0f);
    DrawTextEx(font8bit, "GAME OVER", {screenW / 2 - measure.x / 2, screenH * 0.4f - measure.y / 2}, 0.25f * screenH, 0.0, buttonYellow);
    DrawTextEx(font8bit, (match.matchResult == X_WINS) ? "X WINS" : "O WINS",
               {screenW / 2 - measureStat.x / 2, screenH * 0.4f + measure.y / 2}, 0.15f * screenH, 0.0, BLACK);
}

void drawBotDifficultyScreen(const UIState &ui)
{
    drawParallaxBackground(1.0f);

    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();

    const char *title = "SELECT BOT DIFFICULTY";
    const char *options[] = {"EASY", "MEDIUM", "HARD"};

    float titleFontSize = screenH * 0.07f;
    float optionFontSize = screenH * 0.045f;
    float spacing = 2.0f;

    Vector2 titleSize = MeasureTextEx(font8bit, title, titleFontSize, spacing);
    DrawTextEx(font8bit,
               title,
               {screenW / 2.0f - titleSize.x / 2.0f, screenH * 0.18f},
               titleFontSize,
               spacing,
               buttonYellow);

    float boxWidth = screenW * 0.32f;
    float boxHeight = screenH * 0.075f;
    float startY = screenH * 0.38f;
    float gap = screenH * 0.035f;

    for (int i = 0; i < 3; i++)
    {
        bool isSelected = (i == ui.botDifficultyIndex);

        float posX = screenW / 2.0f - boxWidth / 2.0f;
        float posY = startY + i * (boxHeight + gap);

        Color bgColor = isSelected ? buttonYellow : Fade(BLACK, 0.45f);
        Color borderColor = isSelected ? BLACK : buttonYellow;
        Color textColor = isSelected ? buttonDarkPurple : buttonYellow;

        DrawRectangle((int)posX, (int)posY, (int)boxWidth, (int)boxHeight, bgColor);
        DrawRectangleLinesEx({posX, posY, boxWidth, boxHeight}, 3, borderColor);

        Vector2 textSize = MeasureTextEx(font8bit, options[i], optionFontSize, spacing);
        float textX = posX + (boxWidth - textSize.x) / 2.0f;
        float textY = posY + (boxHeight - textSize.y) / 2.0f;

        DrawTextEx(font8bit,
                   options[i],
                   {textX, textY},
                   optionFontSize,
                   spacing,
                   textColor);
    }

    const char *hint = "W/S OR UP/DOWN: CHOOSE   |   ENTER: CONFIRM   |   ESC: BACK";
    float hintFontSize = screenH * 0.022f;
    Vector2 hintSize = MeasureTextEx(font8bit, hint, hintFontSize, 1);

    DrawTextEx(font8bit,
               hint,
               {screenW / 2.0f - hintSize.x / 2.0f, screenH * 0.82f},
               hintFontSize,
               1,
               Fade(WHITE, 0.7f));
}
void drawLoadGameScreen(const UIState &ui, const std::vector<std::string> &saveFiles)
{
    drawParallaxBackground(1.0f);

    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();

    DrawTextEx(font8bit, "SELECT SAVE FILE",
               {screenW / 2.0f - 220, screenH * 0.10f}, 42, 2, buttonYellow);

    if (saveFiles.empty())
    {
        DrawTextEx(font8bit, "No saves found in /saves folder.",
                   {screenW / 2.0f - 250, screenH / 2.0f}, 30, 2, GRAY);
        DrawTextEx(font8bit, "Press ESC to return.",
                   {screenW / 2.0f - 150, screenH / 2.0f + 50}, 20, 2, DARKGRAY);
        return;
    }

    float rowWidth = 620.0f;
    float rowHeight = 52.0f;
    float startY = screenH * 0.22f;
    float gap = 16.0f;

    for (size_t i = 0; i < saveFiles.size(); i++)
    {
        bool isSelected = ((int)i == ui.loadMenuIndex);

        Color bgColor = isSelected ? buttonYellow : Fade(BLACK, 0.45f);
        Color borderColor = isSelected ? WHITE : Fade(WHITE, 0.18f);
        Color textColor = isSelected ? buttonDarkPurple : RAYWHITE;

        float rowX = screenW / 2.0f - rowWidth / 2.0f;
        float rowY = startY + i * (rowHeight + gap);

        DrawRectangle((int)rowX, (int)rowY, (int)rowWidth, (int)rowHeight, bgColor);
        DrawRectangleLinesEx({rowX, rowY, rowWidth, rowHeight}, 2, borderColor);

        std::string displayName = formatSaveDisplayName(saveFiles[i]);

        Vector2 textSize = MeasureTextEx(font8bit, displayName.c_str(), 26.0f, 2.0f);
        float textX = rowX + (rowWidth - textSize.x) / 2.0f;
        float textY = rowY + (rowHeight - textSize.y) / 2.0f;

        DrawTextEx(font8bit, displayName.c_str(), {textX, textY}, 26.0f, 2.0f, textColor);
    }
}

void drawSettingsScreen(const UIState &ui)
{
    drawParallaxBackground(1.0f);

    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();

    DrawTextEx(font8bit, "SETTINGS", {screenW / 2.0f - 140, screenH * 0.10f}, 52, 2, buttonYellow);

    std::vector<std::string> items;
    items.push_back(std::string("Screen Mode: ") + (ui.isFullscreen ? "Fullscreen" : "Windowed"));
    items.push_back(std::string("Resolution: ") + RESOLUTIONS[ui.resolutionIndex].label);
    items.push_back(std::string("Music: ") + (isMusicEnabled() ? "ON" : "OFF"));
    items.push_back("Music Volume: " + std::to_string((int)(getMusicVolume() * 100)) + "%");
    items.push_back(std::string("SFX: ") + (isSFXEnabled() ? "ON" : "OFF"));
    items.push_back("SFX Volume: " + std::to_string((int)(getSFXVolume() * 100)) + "%");
    items.push_back("Back");

    float startY = screenH * 0.25f;
    float gap = 60.0f;

    for (int i = 0; i < (int)items.size(); i++)
    {
        bool isSelected = (i == ui.settingsMenuIndex);

        Color bgColor = isSelected ? buttonYellow : Fade(BLACK, 0.45f);
        Color textColor = isSelected ? buttonDarkPurple : RAYWHITE;

        float rowX = screenW / 2.0f - 340.0f;
        float rowY = startY + i * gap;

        DrawRectangle((int)rowX, (int)rowY, 680, 46, bgColor);
        DrawTextEx(font8bit, items[i].c_str(), {rowX + 20, rowY + 8}, 28, 2, textColor);
    }
}

std::string formatSaveDisplayName(const std::string &fileName)
{
    if (fileName.size() == 24 &&
        fileName.rfind("save_", 0) == 0 &&
        fileName.substr(fileName.size() - 4) == ".txt")
    {
        std::string year = fileName.substr(5, 4);
        std::string month = fileName.substr(9, 2);
        std::string day = fileName.substr(11, 2);
        std::string hour = fileName.substr(14, 2);
        std::string minute = fileName.substr(16, 2);
        std::string second = fileName.substr(18, 2);

        return day + "/" + month + "/" + year + "  -  " +
               hour + ":" + minute + ":" + second;
    }

    return fileName;
}

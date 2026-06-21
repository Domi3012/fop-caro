#include "view.h"
#include "raylib.h"
#include "model.h"
#include "save_manager.h"
#include <string>
#include <vector>
#include <cmath>
#include <cctype>
#include "audio_manager.h"
#include "sprite_manager.h"

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

struct BoardLayout
{
    float boardPixelSize;
    float cellSize;
    float startX;
    float startY;
};

// Forward declarations — internal only
static void drawParallaxBackground(float speedMultiplier = 1.0f);
static void drawMenu(const UIState &ui);
static void drawMenuButton(const UIState &ui);
static void drawBoard(const MatchState &match, const UIState &ui);
static void drawStatusPanel(const MatchState &match, UIState &ui);
static void drawTurnBanner(const MatchState &match);
static void drawCharacters(const MatchState &match, float shiftX);
static void drawPauseOverlay(const UIState &ui);
static void drawPlayerPanel(const char *name, float displayHealth, int actualHealth, int maxHealth,
                            Color accent,
                            float x, float y, float barW, float barH,
                            float nameFontSize, float hpFontSize);
static void drawTurnIndicator(const MatchState &match, int screenW, int screenH);
static void drawFloatingTexts(UIState &ui);
static void drawWinningHighlight(const MatchState &match, const BoardLayout &layout);
static void drawUndoRedoBar(const UIState &ui);

// --- HAM RENDER TONG ---
void renderGame(const MatchState &match, UIState &ui)
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

    case SAVE_GAME:
        drawSaveGameScreen(ui);
        break;

    case NAME_INPUT:
        drawNameInputScreen(ui);
        break;

    case SETTINGS:
        drawSettingsScreen(ui);
        break;

    case GAME_INTRO:
        drawGameIntro(match, ui);
        break;

    case ATTACK_ANIMATION:
    case GAME_BOARD:
        drawCaroGame(match, ui);
        break;

    case ROUND_OVER:
    {
        drawCaroGame(match, ui);

        int screenW = GetScreenWidth();
        int screenH = GetScreenHeight();
        float fontSize = screenH * 0.1f;

        const char *xName = match.playerX.name.empty() ? "X" : match.playerX.name.c_str();
        const char *oName = match.playerO.name.empty() ? "O" : match.playerO.name.c_str();

        const char *msg;
        if (match.currentRound.result == X_WINS)
            msg = TextFormat("%s WINS THIS ROUND!", xName);
        else if (match.currentRound.result == O_WINS)
            msg = TextFormat("%s WINS THIS ROUND!", oName);
        else
            msg = "DRAW!";

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

static BoardLayout getBoardLayout(int screenW, int screenH)
{
    // Cạnh trên của 2 box thông tin nhân vật (drawPlayerPanel dùng panelPadding = 16)
    float panelTopY = screenH * 0.16f - 16.0f;

    // Chừa chỗ cho panel trái/phải
    float reservedSide = screenW * 0.28f;
    // Chừa phía dưới cho undo/redo bar + margin
    float reservedBottom = screenH * 0.08f;

    float maxBoardW = screenW - reservedSide * 2.0f;
    float maxBoardH = screenH - panelTopY - reservedBottom;

    float boardPixelSize = std::min(maxBoardW, maxBoardH);
    float cellSize = boardPixelSize / BOARD_SIZE;

    float startX = (screenW - boardPixelSize) / 2.0f;
    // Đặt cạnh trên bàn cờ = cạnh trên box nhân vật
    float startY = panelTopY;

    return {boardPixelSize, cellSize, startX, startY};
}

static void drawCharacters(const MatchState &match, const UIState &ui, float shiftX)
{
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();

    // Ground platform at 95% screen height
    float baseGroundY = screenH * 0.95f;

    // Sprite height is 20% of screen height
    float spriteH = screenH * 0.20f;
    float scale = spriteH / 64.0f;
    float spriteW = 64.0f * scale;

    float baseX_X = screenW * 0.15f + shiftX - spriteW / 2.0f;
    float baseX_O = screenW * 0.85f + shiftX - spriteW / 2.0f;
    float baseY = baseGroundY - spriteH;

    // Per-player sprite copies
    static CharacterSprite csX;
    static CharacterSprite csO;
    static CharacterType lastTypeX = (CharacterType)-1;
    static CharacterType lastTypeO = (CharacterType)-1;

    // Re-init khi nhân vật thay đổi
    if (match.playerX.character != lastTypeX)
    {
        csX = getCharacterSpriteCopy(match.playerX.character);
        lastTypeX = match.playerX.character;
    }
    if (match.playerO.character != lastTypeO)
    {
        csO = getCharacterSpriteCopy(match.playerO.character);
        lastTypeO = match.playerO.character;
    }

    // === ATTACK ANIMATION MODE ===
    if (ui.attackAnimPlaying && ui.attackStep >= 0 && ui.attackStep < ATTACK_STEP_COUNT)
    {
        bool isXAttacking = (ui.attackingPlayer == X);
        CharacterSprite &attackerCS = isXAttacking ? csX : csO;
        CharacterSprite &defenderCS = isXAttacking ? csO : csX;
        CharacterType attackerType  = isXAttacking ? match.playerX.character : match.playerO.character;

        float attackerBaseX = isXAttacking ? baseX_X : baseX_O;
        float defenderBaseX = isXAttacking ? baseX_O : baseX_X;
        bool  attackerFlip  = !isXAttacking;  // O luôn flip
        bool  defenderFlip  = isXAttacking;   // Defender là người còn lại

        const AttackStep *steps = getAttackSteps(attackerType);
        const AttackStep &step = steps[ui.attackStep];

        // --- Vẽ attacker ---
        // Offset resolution-independent
        float ox = step.offsetX * screenW;
        if (attackerFlip) ox = -ox;  // Đảo hướng cho Player O
        float oy = step.offsetY * screenH;

        SpriteAnimation &attackAnim = attackerCS.anims[ANIM_ATTACK];
        if (attackAnim.loaded)
        {
            attackAnim.currentFrame = step.attackerFrame;
            drawSprite(attackAnim, attackerBaseX + ox, baseY + oy, scale, attackerFlip);
        }

        // --- Vẽ defender (idle + hit reaction) ---
        float defShakeX = 0.0f;
        Color defTint = WHITE;
        if (step.defenderHit)
        {
            // Rung nhanh: sin tần số cao, biên độ nhỏ
            float time = (float)GetTime();
            defShakeX = std::sin(time * 60.0f) * screenW * 0.008f;
            defTint = {255, 255, 255, 200}; // Flash sáng
        }

        // Defender luôn ở idle animation
        setAnimation(defenderCS, ANIM_IDLE);
        updateCharacterAnimation(defenderCS, GetFrameTime());

        SpriteAnimation &defAnim = defenderCS.anims[defenderCS.currentAnim];
        if (defAnim.loaded)
        {
            float srcX = (float)(defAnim.currentFrame * SPRITE_FRAME_SIZE);
            float srcW = (float)SPRITE_FRAME_SIZE;
            float srcHt = (float)SPRITE_FRAME_SIZE;
            if (defenderFlip) srcW = -srcW;
            Rectangle src = { srcX, 0.0f, srcW, srcHt };
            float destW = SPRITE_FRAME_SIZE * scale;
            float destH = SPRITE_FRAME_SIZE * scale;
            Rectangle dest = { defenderBaseX + defShakeX, baseY, destW, destH };
            DrawTexturePro(defAnim.sheet, src, dest, {0, 0}, 0.0f, defTint);
        }

        // --- Vẽ effect frame trên defender (nếu có) ---
        if (step.effectFrame >= 0)
        {
            drawEffectFrame(attackerType, step.effectFrame,
                            defenderBaseX, baseY, scale, defenderFlip);
        }
    }
    else
    {
        // === NORMAL MODE (idle) ===
        updateCharacterAnimation(csX, GetFrameTime());
        updateCharacterAnimation(csO, GetFrameTime());
        drawCharacterSprite(csX, baseX_X, baseY, scale, false);
        drawCharacterSprite(csO, baseX_O, baseY, scale, true);
    }
}

// --- PARALLAX BACKGROUND ---
static void drawParallaxBackground(float speedMultiplier)
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
static void drawMenu(const UIState &ui)
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

static void drawMenuButton(const UIState &ui)
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

    const char *title = "SELECT GAME MODE";
    const char *modes[] = {"PLAYER VS PLAYER", "PLAYER VS ENVIRONMENT (BOT)"};
    const int totalOptions = 2;

    float titleFontSize = screenH * 0.07f;
    float startY = screenH * 0.38f;
    float gap = screenH * 0.035f;
    float rowWidth = screenW * 0.42f;
    float rowHeight = screenH * 0.075f;
    float optionFontSize = rowHeight * 0.42f;
    float spacing = 2.0f;

    Vector2 titleSize = MeasureTextEx(font8bit, title, titleFontSize, spacing);
    DrawTextEx(font8bit, title,
               {screenW / 2.0f - titleSize.x / 2.0f, screenH * 0.18f},
               titleFontSize, spacing, buttonYellow);

    for (int i = 0; i < totalOptions; i++)
    {
        bool isSelected = (i == ui.modeMenuIndex);

        float posX = screenW / 2.0f - rowWidth / 2.0f;
        float posY = startY + i * (rowHeight + gap);

        Color bgColor = isSelected ? buttonYellow : Fade(BLACK, 0.45f);
        Color borderColor = isSelected ? BLACK : buttonYellow;
        Color textColor = isSelected ? buttonDarkPurple : buttonYellow;

        DrawRectangle((int)posX, (int)posY, (int)rowWidth, (int)rowHeight, bgColor);
        DrawRectangleLinesEx({posX, posY, rowWidth, rowHeight}, 3, borderColor);

        Vector2 textSize = MeasureTextEx(font8bit, modes[i], optionFontSize, spacing);
        float textX = posX + (rowWidth - textSize.x) / 2.0f;
        float textY = posY + (rowHeight - textSize.y) / 2.0f;

        DrawTextEx(font8bit, modes[i], {textX, textY}, optionFontSize, spacing, textColor);
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
    const char *headerText;
    Color headerColor;
    if (ui.isSelectingX)
    {
        headerText = "PLAYER X IS CHOOSING";
        headerColor = RED;
    }
    else if (ui.isPVE)
    {
        headerText = "Player O (BOT) IS CHOOSING";
        headerColor = GRAY;
    }
    else
    {
        headerText = "PLAYER O IS CHOOSING";
        headerColor = BLUE;
    }
    float headerFontSize = screenH * 0.05f; // Slightly smaller to look more balanced
    Vector2 headerSize = MeasureTextEx(font8bit, headerText, headerFontSize, 2);
    DrawTextEx(font8bit, headerText, {screenW / 2.0f - headerSize.x / 2.0f, panelY + screenH * 0.03f}, headerFontSize, 2, headerColor);

    // === REDESIGNED SPLIT PANEL LAYOUT ===
    float contentY = panelY + screenH * 0.10f;
    float contentH = panelH - screenH * 0.22f;
    float contentW = panelW - screenW * 0.06f;
    float contentX = panelX + screenW * 0.03f;

    // 1. Left Box: Character Preview (Animated Sprite)
    float leftW = contentW * 0.40f;
    float leftH = contentH;
    float leftX = contentX;
    float leftY = contentY;

    // 2. Right Column
    float rightW = contentW * 0.55f;
    float rightX = contentX + leftW + contentW * 0.05f;
    float rightY = contentY;

    float nameBoxH = leftH * 0.20f;
    float statsBoxH = leftH * 0.75f;
    float statsBoxY = rightY + nameBoxH + leftH * 0.05f;

    // Determine current character details based on menu index
    Color charColor;
    const char *charName;
    int baseHp = 0;
    int baseDmg = 0;
    CharacterType currentType = ASSASSIN;

    switch (ui.characterMenuIndex)
    {
    case 1:
        charName = "ASSASSIN";
        charColor = PURPLE;
        baseHp = 650;
        baseDmg = 120;
        currentType = ASSASSIN;
        break;
    case 2:
        charName = "BRUISER";
        charColor = ORANGE;
        baseHp = 1600;
        baseDmg = 70;
        currentType = BRUISER;
        break;
    case 3:
        charName = "VAMPIRE";
        charColor = DARKGREEN;
        baseHp = 800;
        baseDmg = 100;
        currentType = VAMPIRE;
        break;
    case 4:
    default:
        charName = "SORCERER";
        charColor = SKYBLUE;
        baseHp = 750;
        baseDmg = 50;
        currentType = SORCERER;
        break;
    }

    // --- LEFT COLUMN: Sprite Preview ---
    DrawRectangle(leftX, leftY, leftW, leftH, Fade(charColor, 0.12f));
    DrawRectangleLinesEx({leftX, leftY, leftW, leftH}, 3, charColor);

    // Get and update animation
    CharacterSprite &cs = getCharacterSprite(currentType);
    setAnimation(cs, ANIM_IDLE);
    updateCharacterAnimation(cs, GetFrameTime());

    // Scale sprite relative to panel height (so it's resolution-independent!)
    float spriteSize = leftH * 0.65f;
    float spriteScale = spriteSize / 64.0f;
    float spriteX = leftX + (leftW - spriteSize) / 2.0f;
    float spriteY = leftY + (leftH - spriteSize) / 2.0f;

    drawCharacterSprite(cs, spriteX, spriteY, spriteScale, false);

    // --- RIGHT COLUMN: Top (Character Name) ---
    DrawRectangle(rightX, rightY, rightW, nameBoxH, Fade(charColor, 0.25f));
    DrawRectangleLinesEx({rightX, rightY, rightW, nameBoxH}, 3, charColor);

    float nameFontSize = nameBoxH * 0.6f;
    Vector2 nameSize = MeasureTextEx(font8bit, charName, nameFontSize, 2);
    DrawTextEx(font8bit, charName, {rightX + (rightW - nameSize.x) / 2.0f, rightY + (nameBoxH - nameSize.y) / 2.0f}, nameFontSize, 2, WHITE);

    // --- RIGHT COLUMN: Bottom (Stats & Skill Box) ---
    DrawRectangle(rightX, statsBoxY, rightW, statsBoxH, Fade(BLACK, 0.4f));
    DrawRectangleLinesEx({rightX, statsBoxY, rightW, statsBoxH}, 2, Fade(WHITE, 0.2f));

    float padX = rightW * 0.05f;
    float padY = statsBoxH * 0.08f;
    float rowH = statsBoxH * 0.13f;
    float statTextSize = statsBoxH * 0.08f;

    // HP Line
    std::string hpText = "BASE HP:  " + std::to_string(baseHp);
    DrawTextEx(font8bit, hpText.c_str(), {rightX + padX, statsBoxY + padY}, statTextSize, 1, GREEN);

    // Damage Line
    std::string dmgText = "BASE DMG: " + std::to_string(baseDmg);
    DrawTextEx(font8bit, dmgText.c_str(), {rightX + padX, statsBoxY + padY + rowH}, statTextSize, 1, ORANGE);

    // Skill Header
    DrawTextEx(font8bit, "SPECIAL SKILL:", {rightX + padX, statsBoxY + padY + rowH * 2.0f}, statsBoxH * 0.07f, 1, buttonYellow);

    // Skill Description Lines (Manually wrapped for pixel-perfect display)
    std::vector<std::string> skillLines;
    if (ui.characterMenuIndex == 1) {
        skillLines = {
            "+5 basic DMG per pair of turns (X - O).",
            "Resets to 120 when a player wins/loses",
            "the round."
        };
    } else if (ui.characterMenuIndex == 2) {
        skillLines = {
            "Thorns: Reflects 25% of any received",
            "damage back to the attacker passively."
        };
    } else if (ui.characterMenuIndex == 3) {
        skillLines = {
            "Lifesteal: Heals player health by 30%",
            "of actual damage dealt during successful",
            "attacks."
        };
    } else {
        skillLines = {
            "Burn: Winning a round applies a Burn stack.",
            "Opponent takes 5 DMG per stack every pair",
            "of turns (X - O). Stacks infinitely."
        };
    }

    float skillLineFontSize = statsBoxH * 0.072f;
    float startY = statsBoxY + padY + rowH * 3.1f;
    for (size_t i = 0; i < skillLines.size(); ++i)
    {
        DrawTextEx(font8bit, skillLines[i].c_str(),
                   {rightX + padX, startY + i * (skillLineFontSize * 1.35f)},
                   skillLineFontSize, 1, Fade(WHITE, 0.9f));
    }

    // --- FOOTER & NAVIGATION ---
    // Indicators (● ● ○)
    float dotGap = 40.0f;
    float dotsStartX = screenW / 2.0f - dotGap * 1.5f;
    float dotsY = panelY + panelH - screenH * 0.08f;
    for (int i = 1; i <= 4; i++)
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
    drawCharacters(match, ui, ui.introCamX);
}

// Nhom ban co
static void drawPauseOverlay(const UIState &ui)
{
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();

    DrawRectangle(0, 0, screenW, screenH, Fade(BLACK, 0.7f));

    float panelW = screenW * 0.32f;
    float panelH = screenH * 0.34f;
    float panelX = screenW / 2.0f - panelW / 2.0f;
    float panelY = screenH / 2.0f - panelH / 2.0f;

    DrawRectangle((int)panelX, (int)panelY, (int)panelW, (int)panelH, buttonDarkPurple);
    DrawRectangleLinesEx({panelX, panelY, panelW, panelH}, 4, buttonYellow);

    const char *title = "PAUSED";
    float titleSize = screenH * 0.06f;
    Vector2 titleMeasure = MeasureTextEx(font8bit, title, titleSize, 2.0f);
    DrawTextEx(font8bit, title,
               {screenW / 2.0f - titleMeasure.x / 2.0f, panelY + panelH * 0.10f},
               titleSize, 2.0f, WHITE);

    const char *options[] = {"Save Game", "Exit to Menu"};
    const int optionCount = 2;

    float optionW = panelW * 0.70f;
    float optionH = panelH * 0.18f;
    float gap = panelH * 0.10f;
    float startY = panelY + panelH * 0.36f;
    float optionFontSize = optionH * 0.42f;

    for (int i = 0; i < optionCount; ++i)
    {
        bool isSelected = (i == ui.pauseMenuIndex);

        Color bgColor = isSelected ? buttonYellow : Fade(BLACK, 0.35f);
        Color borderColor = isSelected ? BLACK : buttonYellow;
        Color textColor = isSelected ? buttonDarkPurple : buttonYellow;

        float optX = screenW / 2.0f - optionW / 2.0f;
        float optY = startY + i * (optionH + gap);

        DrawRectangle((int)optX, (int)optY, (int)optionW, (int)optionH, bgColor);
        DrawRectangleLinesEx({optX, optY, optionW, optionH}, 3, borderColor);

        Vector2 optSize = MeasureTextEx(font8bit, options[i], optionFontSize, 2.0f);
        DrawTextEx(font8bit, options[i],
                   {optX + (optionW - optSize.x) / 2.0f, optY + (optionH - optSize.y) / 2.0f},
                   optionFontSize, 2.0f, textColor);
    }
}

void drawCaroGame(const MatchState &match, UIState &ui)
{
    drawParallaxBackground(0.0f);
    drawCharacters(match, ui, 0.0f);
    drawStatusPanel(match, ui);
    drawBoard(match, ui);
    drawUndoRedoBar(ui);
    drawFloatingTexts(ui);
    if (ui.isPaused)
        drawPauseOverlay(ui);
}

static void drawBoard(const MatchState &match, const UIState &ui)
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

    // Vẽ highlight nhấp nháy các ô thắng (5 nước win)
    drawWinningHighlight(match, layout);
}

// --- WINNING CELLS BLINK ---
static void drawWinningHighlight(const MatchState &match, const BoardLayout &layout)
{
    const auto &cells = match.currentRound.winningCells;
    if (cells.empty())
        return;

    // Nhấp nháy: sử dụng sin() để tạo hiệu ứng blink mượt
    float time = (float)GetTime();
    float blinkAlpha = 0.3f + 0.5f * (0.5f + 0.5f * std::sin(time * 6.0f)); // 0.3 ~ 0.8

    Color winColor;
    if (match.currentRound.result == X_WINS)
        winColor = {255, 215, 0, (unsigned char)(blinkAlpha * 255)}; // Vàng gold
    else
        winColor = {0, 200, 255, (unsigned char)(blinkAlpha * 255)}; // Cyan

    for (auto &cell : cells)
    {
        int row = cell.first;
        int col = cell.second;
        float cellX = layout.startX + col * layout.cellSize;
        float cellY = layout.startY + row * layout.cellSize;

        DrawRectangle((int)cellX, (int)cellY, (int)layout.cellSize, (int)layout.cellSize, winColor);
        DrawRectangleLinesEx({cellX, cellY, layout.cellSize, layout.cellSize}, 3,
                             Fade(WHITE, blinkAlpha));
    }
}

// --- UNDO/REDO BAR ---
static void drawUndoRedoBar(const UIState &ui)
{
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();

    BoardLayout layout = getBoardLayout(screenW, screenH);

    // Thanh nằm ngay bên dưới bàn cờ
    float barY = layout.startY + layout.boardPixelSize + 10.0f;
    float barH = screenH * 0.045f;
    float barW = layout.boardPixelSize;
    float barX = layout.startX;

    // Background cho cả thanh
    DrawRectangle((int)barX, (int)barY, (int)barW, (int)barH, Fade(BLACK, 0.50f));

    // Chia đôi: nút trái = UNDO, nút phải = REDO
    float gap = 6.0f;
    float btnW = (barW - gap) / 2.0f;
    float btnH = barH;

    bool canUndo = !ui.undoStack.empty();
    bool canRedo = !ui.redoStack.empty();

    // --- Nút UNDO ---
    {
        float btnX = barX;
        Color bgColor   = canUndo ? Fade(buttonYellow, 0.85f) : Fade(buttonDarkPurple, 0.40f);
        Color borderClr = canUndo ? buttonYellow               : Fade(buttonYellow, 0.20f);
        Color textClr   = canUndo ? buttonDarkPurple           : Fade(buttonYellow, 0.30f);

        DrawRectangle((int)btnX, (int)barY, (int)btnW, (int)btnH, bgColor);
        DrawRectangleLinesEx({btnX, barY, btnW, btnH}, 2, borderClr);

        const char *label = "UNDO - Press Z";
        float fontSize = btnH * 0.48f;
        Vector2 sz = MeasureTextEx(font8bit, label, fontSize, 1);
        float tx = btnX + (btnW - sz.x) / 2.0f;
        float ty = barY + (btnH - sz.y) / 2.0f;
        DrawTextEx(font8bit, label, {tx, ty}, fontSize, 1, textClr);
    }

    // --- Nút REDO ---
    {
        float btnX = barX + btnW + gap;
        Color bgColor   = canRedo ? Fade(buttonYellow, 0.85f) : Fade(buttonDarkPurple, 0.40f);
        Color borderClr = canRedo ? buttonYellow               : Fade(buttonYellow, 0.20f);
        Color textClr   = canRedo ? buttonDarkPurple           : Fade(buttonYellow, 0.30f);

        DrawRectangle((int)btnX, (int)barY, (int)btnW, (int)btnH, bgColor);
        DrawRectangleLinesEx({btnX, barY, btnW, btnH}, 2, borderClr);

        const char *label = "REDO - Press Y";
        float fontSize = btnH * 0.48f;
        Vector2 sz = MeasureTextEx(font8bit, label, fontSize, 1);
        float tx = btnX + (btnW - sz.x) / 2.0f;
        float ty = barY + (btnH - sz.y) / 2.0f;
        DrawTextEx(font8bit, label, {tx, ty}, fontSize, 1, textClr);
    }
}

static void drawTurnBanner(const MatchState &match)
{
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();

    BoardLayout layout = getBoardLayout(screenW, screenH);

    const char *xName = match.playerX.name.empty() ? "PLAYER X" : match.playerX.name.c_str();
    const char *oName = match.playerO.name.empty() ? "PLAYER O" : match.playerO.name.c_str();
    const char *turnText = (match.currentRound.toMove == X)
                               ? TextFormat("TURN: %s", xName)
                               : TextFormat("TURN: %s", oName);
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
static void drawPlayerPanel(const char *name, float displayHealth, int actualHealth, int maxHealth,
                            Color accent,
                            float x, float y, float barW, float barH,
                            float nameFontSize, float hpFontSize)
{
    Color hpBg = Fade(RAYWHITE, 0.16f);
    Color hpBorder = Fade(WHITE, 0.90f);

    // Tính chiều cao tổng của panel dựa trên nội dung thực tế
    Vector2 nameSize = MeasureTextEx(font8bit, name, nameFontSize, 0);
    float nameBlockH = nameSize.y; // chiều cao tên
    float gap1 = 12.0f;            // khoảng cách tên -> thanh HP
    float gap2 = 10.0f;            // khoảng cách thanh HP -> text HP
    Vector2 hpTextSize = MeasureTextEx(font8bit, TextFormat("HP: %d/%d", actualHealth, maxHealth), hpFontSize, 0);
    float hpTextBlockH = hpTextSize.y;

    float contentH = nameBlockH + gap1 + barH + gap2 + hpTextBlockH;
    float panelPadding = 16.0f;
    float panelH = contentH + panelPadding * 2.0f;
    float panelW = barW + 40.0f;

    // Vẽ background panel
    DrawRectangle((int)(x - 20), (int)(y - panelPadding), (int)panelW, (int)panelH, Fade(BLACK, 0.50f));

    // Vẽ tên (căn dọc chính xác theo MeasureTextEx)
    DrawTextEx(font8bit, name, {x, y}, nameFontSize, 0, accent);

    // Thanh HP mượt (dùng displayHealth thay vì actualHealth)
    float hpFill = barW * (displayHealth / (float)maxHealth);
    if (hpFill < 0.0f) hpFill = 0.0f;
    float barY = y + nameBlockH + gap1;
    DrawRectangle((int)x, (int)barY, (int)barW, (int)barH, hpBg);
    DrawRectangle((int)x, (int)barY, (int)hpFill, (int)barH, accent);
    DrawRectangleLinesEx({x, barY, barW, barH}, 3, hpBorder);

    // Text HP (hiển thị số thực tế, không phải display)
    DrawTextEx(font8bit,
               TextFormat("HP: %d/%d", actualHealth, maxHealth),
               {x, barY + barH + gap2}, hpFontSize, 0, RAYWHITE);
}

static void drawTurnIndicator(const MatchState &match, int screenW, int screenH)
{
    BoardLayout layout = getBoardLayout(screenW, screenH);

    float turnFontSize = screenH * 0.04f;
    const char *xName = match.playerX.name.empty() ? "PLAYER X" : match.playerX.name.c_str();
    const char *oName = match.playerO.name.empty() ? "PLAYER O" : match.playerO.name.c_str();
    const char *turnText = (match.currentRound.toMove == X)
                               ? TextFormat("TURN: %s", xName)
                               : TextFormat("TURN: %s", oName);
    Color turnColor = (match.currentRound.toMove == X) ? RED : BLUE;

    Vector2 turnSize = MeasureTextEx(font8bit, turnText, turnFontSize, 0);
    float turnBoxW = turnSize.x + 60.0f;
    float turnBoxH = turnSize.y + 24.0f;
    float turnBoxX = screenW / 2.0f - turnBoxW / 2.0f;
    // Đặt ngay phía trên bàn cờ, cách 12px
    float turnBoxY = layout.startY - turnBoxH - 12.0f;

    DrawRectangle((int)turnBoxX, (int)turnBoxY, (int)turnBoxW, (int)turnBoxH, Fade(BLACK, 0.60f));
    DrawRectangleLinesEx({turnBoxX, turnBoxY, turnBoxW, turnBoxH}, 3, turnColor);
    DrawTextEx(font8bit, turnText,
               {turnBoxX + 30.0f, turnBoxY + 12.0f},
               turnFontSize, 0, RAYWHITE);
}

static void drawStatusPanel(const MatchState &match, UIState &ui)
{
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();

    // --- Smooth HP interpolation (lerp) ---
    float dt = GetFrameTime();
    float lerpSpeed = 2.5f; // tốc độ nội suy (càng lớn càng nhanh)

    float targetX = (float)match.playerX.health;
    float targetO = (float)match.playerO.health;

    // Lerp: displayHealth tiến dần về targetHealth
    ui.displayHealthX += (targetX - ui.displayHealthX) * lerpSpeed * dt;
    ui.displayHealthO += (targetO - ui.displayHealthO) * lerpSpeed * dt;

    // Snap khi đủ gần
    if (std::fabs(ui.displayHealthX - targetX) < 0.5f)
        ui.displayHealthX = targetX;
    if (std::fabs(ui.displayHealthO - targetO) < 0.5f)
        ui.displayHealthO = targetO;

    float nameFontSize = screenH * 0.06f;
    float hpFontSize = screenH * 0.03f;
    float barW = screenW * 0.20f;
    float barH = screenH * 0.04f;

    const char *xName = match.playerX.name.empty() ? "Player X" : match.playerX.name.c_str();
    const char *oName = match.playerO.name.empty() ? "Player O" : match.playerO.name.c_str();

    drawPlayerPanel(xName, ui.displayHealthX, match.playerX.health, match.playerX.maxHealth, RED,
                    screenW * 0.05f, screenH * 0.16f, barW, barH, nameFontSize, hpFontSize);
    drawPlayerPanel(oName, ui.displayHealthO, match.playerO.health, match.playerO.maxHealth, BLUE,
                    screenW * 0.95f - barW, screenH * 0.16f, barW, barH, nameFontSize, hpFontSize);

    drawTurnIndicator(match, screenW, screenH);
}

// --- FLOATING DAMAGE/HEAL TEXT ---
static void drawFloatingTexts(UIState &ui)
{
    float dt = GetFrameTime();
    int screenH = GetScreenHeight();
    float fontSize = screenH * 0.06f;

    for (int i = (int)ui.floatingTexts.size() - 1; i >= 0; --i)
    {
        auto &ft = ui.floatingTexts[i];
        ft.timer -= dt;

        if (ft.timer <= 0.0f)
        {
            ui.floatingTexts.erase(ui.floatingTexts.begin() + i);
            continue;
        }

        // Tiến trình: 1.0 -> 0.0
        float progress = ft.timer / ft.maxTimer;

        // Bay lên + fade out
        float offsetY = (1.0f - progress) * screenH * 0.08f;
        float alpha = progress;

        // Scale nhỏ dần
        float scale = 0.7f + 0.3f * progress;
        float currentFontSize = fontSize * scale;

        Vector2 textSize = MeasureTextEx(font8bit, ft.text.c_str(), currentFontSize, 2);
        float drawX = ft.x - textSize.x / 2.0f;
        float drawY = ft.y - offsetY;

        // Shadow
        DrawTextEx(font8bit, ft.text.c_str(),
                   {drawX + 3, drawY + 3}, currentFontSize, 2,
                   Fade(BLACK, alpha * 0.6f));
        // Main text
        DrawTextEx(font8bit, ft.text.c_str(),
                   {drawX, drawY}, currentFontSize, 2,
                   Fade(ft.color, alpha));
    }
}
// nhom game over: Lam tam
void drawGameOver(const MatchState &match, const UIState &ui)
{

    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();

    drawParallaxBackground(0.0f); // Lock background
    drawCharacters(match, ui, 0.0f);

    const char *xName = match.playerX.name.empty() ? "X" : match.playerX.name.c_str();
    const char *oName = match.playerO.name.empty() ? "O" : match.playerO.name.c_str();

    Vector2 measure = MeasureTextEx(font8bit, "GAME OVER", 0.25f * screenH, 0.0);
    const char *winMsg = (match.matchResult == X_WINS)
                             ? TextFormat("%s WINS", xName)
                             : TextFormat("%s WINS", oName);
    Vector2 measureStat = MeasureTextEx(font8bit, winMsg, 0.15f * screenH, 0.0f);
    DrawTextEx(font8bit, "GAME OVER", {screenW / 2 - measure.x / 2, screenH * 0.4f - measure.y / 2}, 0.25f * screenH, 0.0, buttonYellow);
    DrawTextEx(font8bit, winMsg,
               {screenW / 2 - measureStat.x / 2, screenH * 0.4f + measure.y / 2}, 0.15f * screenH, 0.0, BLACK);
}

void drawBotDifficultyScreen(const UIState &ui)
{
    drawParallaxBackground(1.0f);

    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();

    const char *title = "SELECT BOT DIFFICULTY";
    const char *options[] = {"EASY", "MEDIUM", "HARD"};
    const int totalOptions = 3;

    float titleFontSize = screenH * 0.07f;
    float startY = screenH * 0.34f;
    float gap = screenH * 0.03f;
    float rowWidth = screenW * 0.32f;
    float rowHeight = screenH * 0.07f;
    float optionFontSize = rowHeight * 0.42f;
    float spacing = 2.0f;

    Vector2 titleSize = MeasureTextEx(font8bit, title, titleFontSize, spacing);
    DrawTextEx(font8bit, title,
               {screenW / 2.0f - titleSize.x / 2.0f, screenH * 0.18f},
               titleFontSize, spacing, buttonYellow);

    for (int i = 0; i < totalOptions; i++)
    {
        bool isSelected = (i == ui.botDifficultyIndex);

        float posX = screenW / 2.0f - rowWidth / 2.0f;
        float posY = startY + i * (rowHeight + gap);

        Color bgColor = isSelected ? buttonYellow : Fade(BLACK, 0.45f);
        Color borderColor = isSelected ? BLACK : buttonYellow;
        Color textColor = isSelected ? buttonDarkPurple : buttonYellow;

        DrawRectangle((int)posX, (int)posY, (int)rowWidth, (int)rowHeight, bgColor);
        DrawRectangleLinesEx({posX, posY, rowWidth, rowHeight}, 3, borderColor);

        Vector2 textSize = MeasureTextEx(font8bit, options[i], optionFontSize, spacing);
        float textX = posX + (rowWidth - textSize.x) / 2.0f;
        float textY = posY + (rowHeight - textSize.y) / 2.0f;

        DrawTextEx(font8bit, options[i], {textX, textY}, optionFontSize, spacing, textColor);
    }
}
void drawLoadGameScreen(const UIState &ui, const std::vector<std::string> &saveFiles)
{
    drawParallaxBackground(1.0f);

    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();

    float titleFontSize = screenH * 0.07f;
    float rowWidth = screenW * 0.42f;
    float rowHeight = screenH * 0.07f;
    float startY = screenH * 0.22f;
    float gap = screenH * 0.02f;
    float rowFontSize = rowHeight * 0.42f;
    float spacing = 2.0f;

    const char *title = "SELECT SAVE FILE";
    Vector2 titleSize = MeasureTextEx(font8bit, title, titleFontSize, spacing);
    DrawTextEx(font8bit, title,
               {screenW / 2.0f - titleSize.x / 2.0f, screenH * 0.10f},
               titleFontSize, spacing, buttonYellow);

    if (saveFiles.empty())
    {
        const char *msg1 = "No saves found in /saves folder.";
        const char *msg2 = "Press ESC to return.";

        float msg1Size = screenH * 0.035f;
        float msg2Size = screenH * 0.025f;

        Vector2 m1 = MeasureTextEx(font8bit, msg1, msg1Size, 2.0f);
        Vector2 m2 = MeasureTextEx(font8bit, msg2, msg2Size, 2.0f);

        DrawTextEx(font8bit, msg1,
                   {screenW / 2.0f - m1.x / 2.0f, screenH * 0.48f},
                   msg1Size, 2.0f, GRAY);

        DrawTextEx(font8bit, msg2,
                   {screenW / 2.0f - m2.x / 2.0f, screenH * 0.55f},
                   msg2Size, 2.0f, DARKGRAY);
        return;
    }

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

        // Parse tên file để lấy tên đặt và ngày giờ
        std::string displayName;
        std::string dateTimeInfo;
        parseSaveFileName(saveFiles[i], displayName, dateTimeInfo);

        // Vẽ tên đặt (chính)
        float nameFontSize = rowFontSize;
        Vector2 nameSize = MeasureTextEx(font8bit, displayName.c_str(), nameFontSize, spacing);
        float nameX = rowX + rowWidth * 0.04f;
        float nameY = rowY + (rowHeight - nameSize.y) / 2.0f;
        DrawTextEx(font8bit, displayName.c_str(), {nameX, nameY}, nameFontSize, spacing, textColor);

        // Vẽ ngày giờ (nhỏ, bên phải)
        if (!dateTimeInfo.empty())
        {
            float dtFontSize = rowFontSize * 0.55f;
            Vector2 dtSize = MeasureTextEx(font8bit, dateTimeInfo.c_str(), dtFontSize, spacing);
            float dtX = rowX + rowWidth - dtSize.x - rowWidth * 0.04f;
            float dtY = rowY + (rowHeight - dtSize.y) / 2.0f;
            Color dtColor = isSelected ? Fade(buttonDarkPurple, 0.7f) : Fade(RAYWHITE, 0.55f);
            DrawTextEx(font8bit, dateTimeInfo.c_str(), {dtX, dtY}, dtFontSize, spacing, dtColor);
        }
    }

    // Footer hint
    const char *footer = "ENTER: Load  |  DEL/BKSP: Delete  |  ESC: Back";
    float footerSize = screenH * 0.022f;
    Vector2 footerMeasure = MeasureTextEx(font8bit, footer, footerSize, 1.0f);
    DrawTextEx(font8bit, footer,
               {screenW / 2.0f - footerMeasure.x / 2.0f, screenH * 0.92f},
               footerSize, 1.0f, Fade(WHITE, 0.45f));

    // --- Popup xác nhận xoá ---
    if (ui.showDeleteConfirm)
    {
        // Overlay mờ toàn màn hình
        DrawRectangle(0, 0, screenW, screenH, Fade(BLACK, 0.65f));

        // Panel xác nhận
        float popupW = screenW * 0.40f;
        float popupH = screenH * 0.22f;
        float popupX = (screenW - popupW) / 2.0f;
        float popupY = (screenH - popupH) / 2.0f;

        DrawRectangle((int)popupX, (int)popupY, (int)popupW, (int)popupH, buttonDarkPurple);
        DrawRectangleLinesEx({popupX, popupY, popupW, popupH}, 4, RED);

        // Tên file sẽ bị xoá
        std::string displayName, dateTimeInfo;
        parseSaveFileName(saveFiles[ui.loadMenuIndex], displayName, dateTimeInfo);

        const char *warnText = "Delete this save?";
        float warnSize = screenH * 0.04f;
        Vector2 warnMeasure = MeasureTextEx(font8bit, warnText, warnSize, 2.0f);
        DrawTextEx(font8bit, warnText,
                   {screenW / 2.0f - warnMeasure.x / 2.0f, popupY + popupH * 0.12f},
                   warnSize, 2.0f, RED);

        // Hiển thị tên file
        std::string fileDisplay = "\"" + displayName + "\"";
        if (!dateTimeInfo.empty())
            fileDisplay += "  (" + dateTimeInfo + ")";
        float fileSize = screenH * 0.028f;
        Vector2 fileMeasure = MeasureTextEx(font8bit, fileDisplay.c_str(), fileSize, 1.0f);
        DrawTextEx(font8bit, fileDisplay.c_str(),
                   {screenW / 2.0f - fileMeasure.x / 2.0f, popupY + popupH * 0.40f},
                   fileSize, 1.0f, buttonYellow);

        // Dòng hướng dẫn
        const char *confirmHint = "ENTER: Confirm Delete  |  ESC/BKSP/DEL: Cancel";
        float hintSize = screenH * 0.022f;
        Vector2 hintMeasure = MeasureTextEx(font8bit, confirmHint, hintSize, 1.0f);
        DrawTextEx(font8bit, confirmHint,
                   {screenW / 2.0f - hintMeasure.x / 2.0f, popupY + popupH * 0.68f},
                   hintSize, 1.0f, Fade(WHITE, 0.6f));
    }
}

void drawSaveGameScreen(const UIState &ui)
{
    drawParallaxBackground(1.0f);

    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();

    // Panel chính
    float panelW = screenW * 0.55f;
    float panelH = screenH * 0.45f;
    float panelX = (screenW - panelW) / 2.0f;
    float panelY = (screenH - panelH) / 2.0f;

    DrawRectangle(panelX, panelY, panelW, panelH, Fade(BLACK, 0.75f));
    DrawRectangleLinesEx({panelX, panelY, panelW, panelH}, 4, buttonYellow);

    // Tiêu đề
    const char *title = "SAVE GAME";
    float titleSize = screenH * 0.07f;
    Vector2 titleMeasure = MeasureTextEx(font8bit, title, titleSize, 2.0f);
    DrawTextEx(font8bit, title,
               {screenW / 2.0f - titleMeasure.x / 2.0f, panelY + panelH * 0.06f},
               titleSize, 2.0f, buttonYellow);

    // Hướng dẫn
    const char *hint = "Enter a name for your save (letters & numbers only):";
    float hintSize = screenH * 0.028f;
    Vector2 hintMeasure = MeasureTextEx(font8bit, hint, hintSize, 1.0f);
    DrawTextEx(font8bit, hint,
               {screenW / 2.0f - hintMeasure.x / 2.0f, panelY + panelH * 0.22f},
               hintSize, 1.0f, Fade(WHITE, 0.8f));

    // Ô nhập tên
    float inputBoxW = panelW * 0.75f;
    float inputBoxH = screenH * 0.06f;
    float inputBoxX = screenW / 2.0f - inputBoxW / 2.0f;
    float inputBoxY = panelY + panelH * 0.38f;

    Color inputBg = Fade(WHITE, 0.1f);
    Color inputBorder = ui.saveNameError ? RED : buttonYellow;
    DrawRectangle((int)inputBoxX, (int)inputBoxY, (int)inputBoxW, (int)inputBoxH, inputBg);
    DrawRectangleLinesEx({inputBoxX, inputBoxY, inputBoxW, inputBoxH}, 3, inputBorder);

    // Hiển thị tên đang nhập + con trỏ nhấp nháy
    float inputFontSize = inputBoxH * 0.55f;
    std::string displayText = ui.saveNameInput;
    // Con trỏ nhấp nháy
    if (((int)(GetTime() * 2.0f) % 2 == 0) && displayText.size() < 20)
        displayText += "_";

    Vector2 inputMeasure = MeasureTextEx(font8bit, displayText.c_str(), inputFontSize, 1.0f);
    DrawTextEx(font8bit, displayText.c_str(),
               {inputBoxX + 16.0f, inputBoxY + (inputBoxH - inputMeasure.y) / 2.0f},
               inputFontSize, 1.0f, RAYWHITE);

    // Thông báo lỗi
    if (ui.saveNameError)
    {
        const char *errMsg;
        if (ui.saveNameInput.empty())
            errMsg = "Name cannot be empty!";
        else
            errMsg = "Only letters (A-Z, a-z) and numbers (0-9) are allowed!";
        float errSize = screenH * 0.024f;
        Vector2 errMeasure = MeasureTextEx(font8bit, errMsg, errSize, 1.0f);
        DrawTextEx(font8bit, errMsg,
                   {screenW / 2.0f - errMeasure.x / 2.0f, inputBoxY + inputBoxH + 12.0f},
                   errSize, 1.0f, RED);
    }

    // Footer
    const char *footer = "ENTER: Save  |  ESC: Cancel";
    float footerSize = screenH * 0.025f;
    Vector2 footerMeasure = MeasureTextEx(font8bit, footer, footerSize, 1.0f);
    DrawTextEx(font8bit, footer,
               {screenW / 2.0f - footerMeasure.x / 2.0f, panelY + panelH - footerMeasure.y - 20.0f},
               footerSize, 1.0f, Fade(WHITE, 0.5f));
}

void drawNameInputScreen(const UIState &ui)
{
    drawParallaxBackground(1.0f);

    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();

    // Panel
    float panelW = screenW * 0.50f;
    float panelH = screenH * 0.40f;
    float panelX = (screenW - panelW) / 2.0f;
    float panelY = (screenH - panelH) / 2.0f;

    DrawRectangle(panelX, panelY, panelW, panelH, Fade(BLACK, 0.75f));
    DrawRectangleLinesEx({panelX, panelY, panelW, panelH}, 4, buttonYellow);

    // Tiêu đề
    const char *title = ui.isEnteringPlayerXName ? "ENTER PLAYER X NAME" : "ENTER PLAYER O NAME";
    float titleSize = screenH * 0.06f;
    Vector2 titleMeasure = MeasureTextEx(font8bit, title, titleSize, 2.0f);
    Color titleColor = ui.isEnteringPlayerXName ? RED : BLUE;
    DrawTextEx(font8bit, title,
               {screenW / 2.0f - titleMeasure.x / 2.0f, panelY + panelH * 0.08f},
               titleSize, 2.0f, titleColor);

    // Hint
    const char *hint = ui.isPVE ? "Enter your name (or press ENTER for default):"
                                : "Enter name (letters, numbers, spaces, - and '):";
    float hintSize = screenH * 0.026f;
    Vector2 hintMeasure = MeasureTextEx(font8bit, hint, hintSize, 1.0f);
    DrawTextEx(font8bit, hint,
               {screenW / 2.0f - hintMeasure.x / 2.0f, panelY + panelH * 0.26f},
               hintSize, 1.0f, Fade(WHITE, 0.7f));

    // Ô nhập
    float inputBoxW = panelW * 0.75f;
    float inputBoxH = screenH * 0.055f;
    float inputBoxX = screenW / 2.0f - inputBoxW / 2.0f;
    float inputBoxY = panelY + panelH * 0.42f;

    DrawRectangle((int)inputBoxX, (int)inputBoxY, (int)inputBoxW, (int)inputBoxH, Fade(WHITE, 0.1f));
    DrawRectangleLinesEx({inputBoxX, inputBoxY, inputBoxW, inputBoxH}, 3, buttonYellow);

    // Hiển thị text + con trỏ
    float inputFontSize = inputBoxH * 0.55f;
    std::string displayText = ui.nameInputBuffer;
    if (((int)(GetTime() * 2.0f) % 2 == 0) && displayText.size() < 20)
        displayText += "_";

    Vector2 inputMeasure = MeasureTextEx(font8bit, displayText.c_str(), inputFontSize, 1.0f);
    DrawTextEx(font8bit, displayText.c_str(),
               {inputBoxX + 12.0f, inputBoxY + (inputBoxH - inputMeasure.y) / 2.0f},
               inputFontSize, 1.0f, RAYWHITE);

    // Footer
    const char *footer = "ENTER: Confirm  |  ESC: Back to Menu";
    float footerSize = screenH * 0.024f;
    Vector2 footerMeasure = MeasureTextEx(font8bit, footer, footerSize, 1.0f);
    DrawTextEx(font8bit, footer,
               {screenW / 2.0f - footerMeasure.x / 2.0f, panelY + panelH - footerMeasure.y - 16.0f},
               footerSize, 1.0f, Fade(WHITE, 0.5f));
}

void drawSettingsScreen(const UIState &ui)
{
    drawParallaxBackground(1.0f);

    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();

    std::vector<std::string> items;
    items.push_back(std::string("Display Mode: ") + (ui.isFullscreen ? "Fullscreen" : "Windowed"));
    items.push_back(std::string("Resolution: ") + RESOLUTIONS[ui.resolutionIndex].label);
    items.push_back(std::string("Music: ") + (isMusicEnabled() ? "ON" : "OFF"));
    items.push_back(TextFormat("Music Volume: %d%%", (int)(getMusicVolume() * 100.0f)));
    items.push_back(std::string("SFX: ") + (isSFXEnabled() ? "ON" : "OFF"));
    items.push_back(TextFormat("SFX Volume: %d%%", (int)(getSFXVolume() * 100.0f)));
    items.push_back("Back");

    float titleFontSize = screenH * 0.07f;
    float rowWidth = screenW * 0.52f;
    float rowHeight = screenH * 0.065f;
    float startY = screenH * 0.22f;
    float gap = screenH * 0.018f;
    float rowFontSize = rowHeight * 0.40f;
    float spacing = 2.0f;

    const char *title = "SETTINGS";
    Vector2 titleSize = MeasureTextEx(font8bit, title, titleFontSize, spacing);
    DrawTextEx(font8bit, title,
               {screenW / 2.0f - titleSize.x / 2.0f, screenH * 0.10f},
               titleFontSize, spacing, buttonYellow);

    for (int i = 0; i < (int)items.size(); i++)
    {
        bool isSelected = (i == ui.settingsMenuIndex);
        bool resolutionLocked = (i == 1 && ui.isFullscreen);

        float rowX = screenW / 2.0f - rowWidth / 2.0f;
        float rowY = startY + i * (rowHeight + gap);

        Color bgColor;
        Color borderColor;
        Color textColor;

        if (resolutionLocked)
        {
            bgColor = Fade(BLACK, 0.28f);
            borderColor = Fade(WHITE, 0.12f);
            textColor = Fade(RAYWHITE, 0.35f);
        }
        else
        {
            bgColor = isSelected ? buttonYellow : Fade(BLACK, 0.45f);
            borderColor = isSelected ? WHITE : Fade(WHITE, 0.18f);
            textColor = isSelected ? buttonDarkPurple : RAYWHITE;
        }

        DrawRectangle((int)rowX, (int)rowY, (int)rowWidth, (int)rowHeight, bgColor);
        DrawRectangleLinesEx({rowX, rowY, rowWidth, rowHeight}, 2, borderColor);

        DrawTextEx(font8bit, items[i].c_str(),
                   {rowX + rowWidth * 0.04f, rowY + (rowHeight - rowFontSize) / 2.0f},
                   rowFontSize, spacing, textColor);

        if (resolutionLocked)
        {
            const char *locked = "LOCKED IN FULLSCREEN";
            float lockSize = rowHeight * 0.28f;
            Vector2 lockMeasure = MeasureTextEx(font8bit, locked, lockSize, 1.0f);
            DrawTextEx(font8bit, locked,
                       {rowX + rowWidth - lockMeasure.x - rowWidth * 0.04f, rowY + (rowHeight - lockMeasure.y) / 2.0f},
                       lockSize, 1.0f, Fade(RAYWHITE, 0.35f));
        }
    }
}

void parseSaveFileName(const std::string &fileName, std::string &displayName, std::string &dateTimeInfo)
{
    displayName.clear();
    dateTimeInfo.clear();

    // Kiểm tra đuôi .txt
    if (fileName.size() < 5 || fileName.substr(fileName.size() - 4) != ".txt")
    {
        displayName = fileName;
        return;
    }

    // Bỏ đuôi .txt
    std::string stem = fileName.substr(0, fileName.size() - 4);

    // Định dạng mới: <tên>_<YYYYMMDD_HHMMSS>
    // Tìm dấu _ cuối cùng ngăn cách timestamp (định dạng YYYYMMDD_HHMMSS = 15 kí tự)
    // Timestamp pattern: 8 chữ số + _ + 6 chữ số = 15 kí tự
    if (stem.size() > 16)
    {
        // Lấy 15 kí tự cuối, kiểm tra có khớp pattern YYYYMMDD_HHMMSS không
        std::string suffix = stem.substr(stem.size() - 15);
        bool match = (suffix.size() == 15 &&
                      std::isdigit(suffix[0]) && std::isdigit(suffix[1]) &&
                      std::isdigit(suffix[2]) && std::isdigit(suffix[3]) &&
                      std::isdigit(suffix[4]) && std::isdigit(suffix[5]) &&
                      std::isdigit(suffix[6]) && std::isdigit(suffix[7]) &&
                      suffix[8] == '_' &&
                      std::isdigit(suffix[9]) && std::isdigit(suffix[10]) &&
                      std::isdigit(suffix[11]) && std::isdigit(suffix[12]) &&
                      std::isdigit(suffix[13]) && std::isdigit(suffix[14]));

        if (match)
        {
            // Tên đặt = phần trước timestamp (bỏ dấu _)
            displayName = stem.substr(0, stem.size() - 16); // bỏ _<timestamp>

            // Parse ngày giờ
            std::string year = suffix.substr(0, 4);
            std::string month = suffix.substr(4, 2);
            std::string day = suffix.substr(6, 2);
            std::string hour = suffix.substr(9, 2);
            std::string minute = suffix.substr(11, 2);
            std::string second = suffix.substr(13, 2);

            dateTimeInfo = day + "/" + month + "/" + year + " " +
                           hour + ":" + minute + ":" + second;
            return;
        }
    }

    // Fallback: thử định dạng cũ "save_YYYYMMDD_HHMMSS.txt" (24 kí tự)
    if (fileName.size() == 24 &&
        fileName.rfind("save_", 0) == 0)
    {
        std::string year = fileName.substr(5, 4);
        std::string month = fileName.substr(9, 2);
        std::string day = fileName.substr(11, 2);
        std::string hour = fileName.substr(14, 2);
        std::string minute = fileName.substr(16, 2);
        std::string second = fileName.substr(18, 2);

        displayName = "AutoSave";
        dateTimeInfo = day + "/" + month + "/" + year + " " +
                       hour + ":" + minute + ":" + second;
        return;
    }

    // Không parse được
    displayName = fileName;
}

#include "controller.h"
#include "view.h"
#include "model.h"
#include "save_manager.h"
#include <ctime>
#include <cctype>
#include "bot_ai.h"
#include "audio_manager.h"

// Input helpers: gom W/S/A/D và mũi tên thành một lần kiểm tra
inline bool isDirUp() { return IsKeyPressed('W') || IsKeyPressed('w') || IsKeyPressed(KEY_UP); }
inline bool isDirDown() { return IsKeyPressed('S') || IsKeyPressed('s') || IsKeyPressed(KEY_DOWN); }
inline bool isDirLeft() { return IsKeyPressed('A') || IsKeyPressed('a') || IsKeyPressed(KEY_LEFT); }
inline bool isDirRight() { return IsKeyPressed('D') || IsKeyPressed('d') || IsKeyPressed(KEY_RIGHT); }
inline bool isConfirm() { return IsKeyPressed(KEY_ENTER); }

// Wrap index: cuộn vòng danh sách menu
inline int wrapPrevIndex(int current, int total)
{
    return total <= 0 ? 0 : (current - 1 + total) % total;
}
inline int wrapNextIndex(int current, int total)
{
    return total <= 0 ? 0 : (current + 1) % total;
}

// Bỏ qua mục index 1 (Resolution) khi đang ở chế độ Fullscreen,
// vì resolution không áp dụng cho fullscreen.
static int getPrevSettingsIndex(int current, bool isFullscreen)
{
    int idx = current;
    do
    {
        idx = wrapPrevIndex(idx, 7);
    } while (isFullscreen && idx == 1);
    return idx;
}
static int getNextSettingsIndex(int current, bool isFullscreen)
{
    int idx = current;
    do
    {
        idx = wrapNextIndex(idx, 7);
    } while (isFullscreen && idx == 1);
    return idx;
}

// Resolution helpers

// Kiểm tra độ phân giải có khớp với màn hình hiện tại không.
static bool isResolutionAllowed(int width, int height)
{
    int monitor = GetCurrentMonitor();
    return width <= GetMonitorWidth(monitor) && height <= GetMonitorHeight(monitor);
}

// Tìm index độ phân giải tiếp theo (step = +1 hoặc -1) mà màn hình hỗ trợ.
// Nếu không tìm được, giữ nguyên current.
static int stepResolutionIndex(int current, int step)
{
    for (int i = 1; i <= RESOLUTION_COUNT; i++)
    {
        int idx = (current + step * i % RESOLUTION_COUNT + RESOLUTION_COUNT) % RESOLUTION_COUNT;
        if (isResolutionAllowed(RESOLUTIONS[idx].width, RESOLUTIONS[idx].height))
        {
            return idx;
        }
    }
    return current;
}

// Đảm bảo resolutionIndex không vượt quá kích thước màn hình.
// Gọi khi chuyển từ cửa sổ sang fullscreen hoặc khi vào màn Settings.
static void clampResolutionIndexToMonitor(UIState &ui)
{
    if (isResolutionAllowed(RESOLUTIONS[ui.resolutionIndex].width, RESOLUTIONS[ui.resolutionIndex].height))
    {
        return; // Index hiện tại vẫn hợp lệ, không cần clamp
    }
    // Duyệt từ cao xuống thấp để chọn độ phân giải lớn nhất còn khớp
    for (int i = RESOLUTION_COUNT - 1; i >= 0; --i)
    {
        if (isResolutionAllowed(RESOLUTIONS[i].width, RESOLUTIONS[i].height))
        {
            ui.resolutionIndex = i;
            return;
        }
    }
    ui.resolutionIndex = 0; // Fallback: 640×360
}

// Áp dụng cài đặt fullscreen / độ phân giải vào cửa sổ raylib.
// Gọi mỗi khi người dùng thay đổi một trong hai cài đặt trên.
static void applyDisplaySettings(UIState &ui)
{
    // Clamp index trước để tránh truy cập ngoài mảng
    ui.resolutionIndex = (ui.resolutionIndex < 0) ? 0 : (ui.resolutionIndex >= RESOLUTION_COUNT) ? RESOLUTION_COUNT - 1
                                                                                                 : ui.resolutionIndex;

    const ResolutionOption &res = RESOLUTIONS[ui.resolutionIndex];
    int monitor = GetCurrentMonitor();
    int mWidth = GetMonitorWidth(monitor);
    int mHeight = GetMonitorHeight(monitor);

    if (ui.isFullscreen)
    {
        if (!IsWindowFullscreen())
        {
            SetWindowSize(mWidth, mHeight);
            SetWindowPosition(0, 0);
            ToggleFullscreen();
        }
    }
    else
    {
        if (IsWindowFullscreen())
            ToggleFullscreen();
        SetWindowSize(res.width, res.height);

        if (res.width >= mWidth && res.height >= mHeight)
        {
            MaximizeWindow();
        }
        else
        {
            int posX = (mWidth - res.width) / 2;
            int posY = (mHeight - res.height) / 2;
            SetWindowPosition(posX, posY);
        }
    }
}
// Xử lý một nước đi tại ô (x=row, y=col):
//   1. Kiểm tra nước đi hợp lệ.
//   2. Ghi nước đi vào board.
//   3. Kiểm tra kết quả round → nếu thắng thì tính sát thương và kiểm tra match.
//   4. Cập nhật ui.currentScreen tương ứng.
void processMoveAndResult(MatchState &match, UIState &ui, int x, int y)
{
    RoundState &round = match.currentRound;

    if (!checkValidMove(round, x, y))
    {
        return;
    }

    makeMove(round, x, y);

    // Ghi lại lịch sử nước đi
    Move m;
    m.x = x;
    m.y = y;
    m.type = round.board[x][y]; // Loại quân vừa đặt (X hoặc O)
    ui.moveHistory.push_back(m);

    if (round.turnCount > 0 && round.turnCount % 2 == 0)
    {
        if (match.playerX.sorcererStacks > 0)
        {
            match.playerX.health = std::max(match.playerX.health - 5 * match.playerX.sorcererStacks, 0);
        }
        if (match.playerO.sorcererStacks > 0)
        {
            match.playerO.health = std::max(match.playerO.health - 5 * match.playerO.sorcererStacks, 0);
        }

        RoundResult mr = checkMatchResult(match);
        if (mr != ONGOING)
        {
            match.matchResult = mr;
            ui.currentScreen = GAME_OVER;
            return;
        }
    }

    RoundResult rr = checkRoundResult(round, x, y);

    if (rr == X_WINS || rr == O_WINS)
    {
        round.result = rr;
        match.countRoundsPlayed++;

        // Ghi nhận HP trước khi attack để tính damage/heal
        Player &attacker = (rr == X_WINS) ? match.playerX : match.playerO;
        Player &defender = (rr == X_WINS) ? match.playerO : match.playerX;
        int defenderHpBefore = defender.health;
        int attackerHpBefore = attacker.health;

        executeAttack(attacker, defender, round.turnCount);

        // --- Spawn floating damage/heal text ---
        int screenW = GetScreenWidth();
        int screenH = GetScreenHeight();
        int damageDealt = defenderHpBefore - defender.health;
        int healAmount = attacker.health - attackerHpBefore;

        // Damage text (đỏ) — hiện trên panel bên bị đánh
        if (damageDealt > 0)
        {
            float dmgX, dmgY;
            if (rr == X_WINS) // defender = O → panel bên phải
            {
                dmgX = screenW * 0.85f;
                dmgY = screenH * 0.14f;
            }
            else // defender = X → panel bên trái
            {
                dmgX = screenW * 0.15f;
                dmgY = screenH * 0.14f;
            }
            UIState::FloatingText ft;
            ft.text = "-" + std::to_string(damageDealt);
            ft.color = RED;
            ft.x = dmgX;
            ft.y = dmgY;
            ft.timer = 1.8f;
            ft.maxTimer = 1.8f;
            ui.floatingTexts.push_back(ft);
        }

        // Heal text (xanh lá) — hiện trên panel bên attacker (Vampire)
        if (healAmount > 0)
        {
            float healX, healY;
            if (rr == X_WINS) // attacker = X → panel bên trái
            {
                healX = screenW * 0.15f;
                healY = screenH * 0.14f;
            }
            else // attacker = O → panel bên phải
            {
                healX = screenW * 0.85f;
                healY = screenH * 0.14f;
            }
            UIState::FloatingText ft;
            ft.text = "+" + std::to_string(healAmount);
            ft.color = GREEN;
            ft.x = healX;
            ft.y = healY;
            ft.timer = 1.8f;
            ft.maxTimer = 1.8f;
            ui.floatingTexts.push_back(ft);
        }

        // Kiểm tra xem trận đấu tổng đã có người thắng chưa
        RoundResult mr = checkMatchResult(match);
        if (mr == X_WINS || mr == O_WINS)
        {
            match.matchResult = mr;
            ui.currentScreen = GAME_OVER;
        }
        else
        {
            ui.currentScreen = ROUND_OVER;
        }
    }
    else if (rr == DRAW)
    {
        round.result = DRAW;
        match.countRoundsPlayed++;
        ui.currentScreen = ROUND_OVER;
    }
    // rr == ONGOING: không làm gì, game tiếp tục
}

// handleMainMenuInput:
// Lên xuống để di chuyển mainMenuIndex
// Enter để xác nhận -> chuyển sang CHARACTER_SELECTION
void handleMainMenuInput(UIState &ui)
{
    const int totalOptions = 4;

    if (isDirUp())
    {
        ui.mainMenuIndex = wrapPrevIndex(ui.mainMenuIndex, totalOptions);
        playSFX(SFX_CLICK);
    }

    if (isDirDown())
    {
        ui.mainMenuIndex = wrapNextIndex(ui.mainMenuIndex, totalOptions);
        playSFX(SFX_CLICK);
    }

    if (isConfirm())
    {
        playSFX(SFX_CLICK);
        switch (ui.mainMenuIndex)
        {
        case 0:
        {
            ui.currentScreen = MODE_SELECTION;
            ui.modeMenuIndex = 0;
            break;
        }
        case 1:
        {
            ui.currentScreen = LOAD_GAME;
            ui.loadMenuIndex = 0;
            break;
        }
        case 2:
        {
            ui.currentScreen = SETTINGS;
            ui.settingsMenuIndex = 0;
            clampResolutionIndexToMonitor(ui);
            break;
        }
        case 3:
        {
            ui.shouldExit = true;
            break;
        }
        }
    }
}

// handleCharSelectionInput:
// Lên xuống để chọn nhân vật (1 = ASSASSIN, 2 = BRUISER, 3 = VAMPIRE)
// Enter để xác nhận
// X chọn xong -> O chọn
// O chọn xong -> initMatch và chuyển sang GAME_BOARD
void handleCharSelectionInput(MatchState &match, UIState &ui)
{
    if (isDirLeft())
    {
        if (ui.characterMenuIndex > 1)
        {
            ui.characterMenuIndex--;
            playSFX(SFX_CLICK);
        }
    }

    if (isDirRight())
    {
<<<<<<< HEAD
        if (ui.characterMenuIndex < 3)
        {
=======
        if (ui.characterMenuIndex < 4) {
>>>>>>> 4286fe6 (feat: Add new character system and Sorcerer class)
            ui.characterMenuIndex++;
            playSFX(SFX_CLICK);
        }
    }

    if (isConfirm())
    {
        // ánh xạ index -> CharacterType
        CharacterType chosen;
        switch (ui.characterMenuIndex)
        {
        case 1:
            chosen = ASSASSIN;
            break;
        case 2:
            chosen = BRUISER;
            break;
        case 3:
            chosen = VAMPIRE;
            break;
        default:
            chosen = SORCERER;
            break;
        }

        if (ui.isSelectingX)
        {
            match.playerX.character = chosen;
            playSFX(SFX_CLICK);

            // Ca PVE lan PVP: chuyen sang buoc chon nhan vat thu 2
            ui.isSelectingX = false;
            ui.characterMenuIndex = 1;
        }
        else
        {
            // Buoc 2 da chon xong (O hoac Bot) -> bat dau game
            match.playerO.character = chosen;
            playSFX(SFX_CLICK);

            Player playerX;
            playerX.character = match.playerX.character;
            playerX.maxHealth = getBaseHealth(playerX.character);
            playerX.health = playerX.maxHealth;
            playerX.sorcererStacks = 0;

            Player playerO;
            playerO.character = match.playerO.character;
            playerO.maxHealth = getBaseHealth(playerO.character);
            playerO.health = playerO.maxHealth;
            playerO.sorcererStacks = 0;

            initMatch(match, playerX, playerO);
            ui.moveHistory.clear(); // Xoá lịch sử cho game mới
            ui.displayHealthX = (float)MAX_HEALTH;
            ui.displayHealthO = (float)MAX_HEALTH;
            ui.floatingTexts.clear();
            startGameIntro(ui);
        }
    }

    // ESC -> quay lại main menu
    if (IsKeyPressed(KEY_ESCAPE))
    {
        ui.currentScreen = MAIN_MENU;
        ui.mainMenuIndex = 0;
        ui.isSelectingX = true;
        ui.characterMenuIndex = 1;
        playMusic(BGM_MENU);
    }
}

// handleGameplayInput:
// Mũi tên để di chuyển con trỏ trên bàn cờ
// Enter để đặt quân tại ô đang trỏ
// Sau mỗi nước đi hợp lệ kiểm tra kết quả round rồi match
void handleGameplayInput(MatchState &match, UIState &ui)
{
    if (ui.isPaused)
    {
        const int pauseOptionCount = 2;

        if (isDirUp())
        {
            ui.pauseMenuIndex = wrapPrevIndex(ui.pauseMenuIndex, pauseOptionCount);
        }

        if (isDirDown())
        {
            ui.pauseMenuIndex = wrapNextIndex(ui.pauseMenuIndex, pauseOptionCount);
        }

        if (isConfirm())
        {
            if (ui.pauseMenuIndex == 0)
            {
                // Chuyển sang màn hình đặt tên save
                ui.saveNameInput.clear();
                ui.saveNameError = false;
                ui.currentScreen = SAVE_GAME;
            }
            else if (ui.pauseMenuIndex == 1)
            {
                ui.isPaused = false;
                ui.currentScreen = MAIN_MENU;
                ui.mainMenuIndex = 0;
                playMusic(BGM_MENU);
            }
        }

        if (IsKeyPressed(KEY_ESCAPE))
        {
            ui.isPaused = false;
        }

        return;
    }

    if (IsKeyPressed(KEY_ESCAPE))
    {
        ui.isPaused = true;
        ui.pauseMenuIndex = 0;
        return;
    }

    // Di chuyển con trỏ
    // W/S/up/down sẽ là cursorY vì di chuyển theo chiều dọc (col)
    if (isDirUp())
    {
        if (ui.cursorY > 0)
        {
            ui.cursorY--;
        }
    }

    if (isDirDown())
    {
        if (ui.cursorY < BOARD_SIZE - 1)
        {
            ui.cursorY++;
        }
    }
    // A/D/left/right điều khiển cursorX (cột — chiều ngang)
    if (isDirLeft())
    {
        if (ui.cursorX > 0)
        {
            ui.cursorX--;
        }
    }

    if (isDirRight())
    {
        if (ui.cursorX < BOARD_SIZE - 1)
        {
            ui.cursorX++;
        }
    }

    // Đặt quân
    if (isConfirm())
    {
        GameScreen prevGameScreen = ui.currentScreen;
        processMoveAndResult(match, ui, ui.cursorY, ui.cursorX);

        // Phat SFX dua theo ket qua sau khi dat quan
        if (ui.currentScreen == ROUND_OVER)
        {
            playSFX(SFX_WIN);
        }
        else if (ui.currentScreen == GAME_OVER)
        {
            playSFX(SFX_GAME_OVER);
        }
        else if (prevGameScreen == GAME_BOARD && ui.currentScreen == GAME_BOARD)
        {
            playSFX(SFX_PLACE);
        }

        if (ui.currentScreen != GAME_BOARD)
        {
            return;
        }

        // Chế độ PVE: sau nước đi của người chơi, bot đi ngay trong cùng frame
        if (ui.isPVE && match.currentRound.toMove == O)
        {
            auto botMove = getBestMove(match.currentRound, O, ui.botDifficulty);
            if (botMove.first == -1)
            {
                // Bot không tìm được ô hợp lệ → bàn cờ đầy, xử lý hòa thủ công
                match.currentRound.result = DRAW;
                match.countRoundsPlayed++;
                ui.currentScreen = ROUND_OVER;
                ui.roundOverTimer = 0.0f;
            }
            else
            {
                processMoveAndResult(match, ui, botMove.first, botMove.second);
            }
        }
    }
}

// handleRoundOverInput:
// Nhấn bất kỳ phím nào để tiếp tục
// Nếu match còn ONGOING -> khởi tạo round mới, quay lại GAME_BOARD
// Nếu match kết thúc   -> chuyển sang GAME_OVER
void handleRoundOverInput(MatchState &match, UIState &ui)
{
    ui.roundOverTimer += GetFrameTime();

    if (ui.roundOverTimer < 2.0f)
        return;

    RoundResult mr = checkMatchResult(match);
    if (mr == X_WINS || mr == O_WINS)
    {
        match.matchResult = mr;
        ui.currentScreen = GAME_OVER;
    }
    else
    {
        initRound(match.currentRound, match.countRoundsPlayed);

        if (ui.isPVE)
        {
            match.currentRound.toMove = X;
        }

        ui.moveHistory.clear(); // Xoá lịch sử cho round mới
        startMatch(ui);
    }
}

// handleGameOverInput:
// Enter -> quay lại MAIN_MENU
// ESC   -> thoát game (đóng cửa sổ raylib)
void handleGameOverInput(MatchState &match, UIState &ui)
{
    if (isConfirm())
    {
        ui.currentScreen = MAIN_MENU;
        ui.mainMenuIndex = 0;
        ui.isSelectingX = true;
        ui.characterMenuIndex = 1;
        playMusic(BGM_MENU);
    }

    if (IsKeyPressed(KEY_ESCAPE))
    {
        ui.currentScreen = MAIN_MENU;
        playMusic(BGM_MENU);
    }

    (void)match;
}

// startGameIntro:
// Cai dat introCamX qua ben phai man hinh va chuyen bien trang thai thanh GAME_INTRO
void startGameIntro(UIState &ui)
{
    ui.introCamX = (float)GetScreenWidth() * 5.0f;
    ui.roundOverTimer = 0.0f;
    ui.currentScreen = GAME_INTRO;
    playMusic(BGM_BATTLE); // Bat nhac nen tran dau ngay tu intro
}

// startMatch:
// Điểm thống nhất để khởi động game sau khi match đã được khởi tạo.
// Dùng chung cho cả New Game (sau initMatch) lẫn Load Game (sau loadGame)
// và bắt đầu round mới (sau initRound trong handleRoundOverInput).
void startMatch(UIState &ui)
{
    ui.cursorX = BOARD_SIZE / 2;
    ui.cursorY = BOARD_SIZE / 2;
    ui.roundOverTimer = 0.0f;
    ui.introCamX = 0.0f;
    ui.currentScreen = GAME_BOARD;
}

// handleModeSelectionInput:
// W/S hoac A/D de chon giua PVP (0) va PVE (1)
void handleModeSelectionInput(UIState &ui)
{
    const int modeCount = 2;

    if (isDirLeft() ||
        isDirUp())
    {
        ui.modeMenuIndex = wrapPrevIndex(ui.modeMenuIndex, modeCount);
    }

    if (isDirRight() ||
        isDirDown())
    {
        ui.modeMenuIndex = wrapNextIndex(ui.modeMenuIndex, modeCount);
    }

    if (isConfirm())
    {
        ui.isPVE = (ui.modeMenuIndex == 1);

        if (ui.isPVE)
        {
            ui.currentScreen = BOT_DIFFICULTY_SELECTION;
            ui.botDifficultyIndex = 0;
        }
        else
        {
            ui.currentScreen = CHARACTER_SELECTION;
            ui.isSelectingX = true;
            ui.characterMenuIndex = 1;
        }
    }

    if (IsKeyPressed(KEY_ESCAPE))
    {
        ui.currentScreen = MAIN_MENU;
    }
}

// handleGameIntroInput:
// Xu ly logic animation (giam introCamX) hoac skip (Enter/Space)
void handleGameIntroInput(MatchState &match, UIState &ui)
{
    // Skip intro
    if (isConfirm() || IsKeyPressed(KEY_SPACE))
    {
        ui.introCamX = 0.0f;
        startMatch(ui);
        return;
    }

    float dt = GetFrameTime();
    ui.roundOverTimer += dt;

    // Camera bay từ phải sang trái trong 3.5 giây
    const float totalTime = 3.5f;
    const float totalDistance = (float)GetScreenWidth() * 5.0f;

    float p = ui.roundOverTimer / totalTime; // Tiến độ [0, 1]
    if (p >= 1.0f)
    {
        ui.introCamX = 0.0f;
        startMatch(ui);
        return;
    }

    float ep_prime = 30.0f * p * p * (1.0f - p) * (1.0f - p);
    float velocity = ep_prime * totalDistance / totalTime;

    ui.introCamX -= velocity * dt;

    if (ui.introCamX <= 0.0f)
    {
        ui.introCamX = 0.0f;
        startMatch(ui);
    }

    (void)match;
}

// Dispatcher trung tâm — gọi đúng handler theo ui.currentScreen mỗi frame.
void handleInput(MatchState &match, UIState &ui)
{
    // Cache danh sách save file dưới dạng biến static:
    // chỉ refresh khi người dùng vừa chuyển sang màn LOAD_GAME.
    static std::vector<std::string> cachedSaveFiles;

    switch (ui.currentScreen)
    {
    case MAIN_MENU:
        handleMainMenuInput(ui);
        // Refresh ngay sau khi handleMainMenuInput() chuyển sang LOAD_GAME
        if (ui.currentScreen == LOAD_GAME)
            cachedSaveFiles = getSaveFilesList();
        break;

    case MODE_SELECTION:
        handleModeSelectionInput(ui);
        break;

    case CHARACTER_SELECTION:
        handleCharSelectionInput(match, ui);
        break;

    case GAME_INTRO:
        handleGameIntroInput(match, ui);
        break;

    case LOAD_GAME:
        handleLoadGameInput(match, ui, cachedSaveFiles);
        break;

    case SAVE_GAME:
        handleSaveGameInput(match, ui);
        break;

    case SETTINGS:
        handleSettingsInput(ui);
        break;

    case GAME_BOARD:
        handleGameplayInput(match, ui);
        break;

    case ROUND_OVER:
        handleRoundOverInput(match, ui);
        break;

    case GAME_OVER:
        handleGameOverInput(match, ui);
        break;

    case BOT_DIFFICULTY_SELECTION:
        handleBotDifficultyInput(ui);
        break;
    }
}

void handleLoadGameInput(MatchState &match, UIState &ui, std::vector<std::string> &saveFiles)
{
    // --- Đang trong popup xác nhận xoá ---
    if (ui.showDeleteConfirm)
    {
        if (isConfirm())
        {
            // Xác nhận xoá
            deleteSaveFile(saveFiles[ui.loadMenuIndex]);
            saveFiles = getSaveFilesList();
            if (saveFiles.empty())
                ui.loadMenuIndex = 0;
            else if (ui.loadMenuIndex >= (int)saveFiles.size())
                ui.loadMenuIndex = (int)saveFiles.size() - 1;
            ui.showDeleteConfirm = false;
        }
        else if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_BACKSPACE) || IsKeyPressed(KEY_DELETE))
        {
            // Huỷ xoá
            ui.showDeleteConfirm = false;
        }
        return; // Chặn mọi input khác khi đang confirm
    }

    if (saveFiles.empty())
    {
        if (IsKeyPressed(KEY_ESCAPE) || isConfirm())
        {
            ui.currentScreen = MAIN_MENU;
        }
        return;
    }

    const int totalSaves = (int)saveFiles.size();

    if (isDirUp())
    {
        ui.loadMenuIndex = wrapPrevIndex(ui.loadMenuIndex, totalSaves);
    }

    if (isDirDown())
    {
        ui.loadMenuIndex = wrapNextIndex(ui.loadMenuIndex, totalSaves);
    }

    if (isConfirm())
    {
        ui.moveHistory.clear();
        if (loadGame(match, ui.moveHistory, saveFiles[ui.loadMenuIndex]))
        {
            ui.displayHealthX = (float)match.playerX.health;
            ui.displayHealthO = (float)match.playerO.health;
            ui.floatingTexts.clear();
            startGameIntro(ui);
        }
    }

    // Yêu cầu xác nhận xoá file save
    if (IsKeyPressed(KEY_DELETE) || IsKeyPressed(KEY_BACKSPACE))
    {
        ui.showDeleteConfirm = true;
    }

    if (IsKeyPressed(KEY_ESCAPE))
    {
        ui.currentScreen = MAIN_MENU;
    }
}

void handleSaveGameInput(MatchState &match, UIState &ui)
{
    // ESC: quay lại game (pause)
    if (IsKeyPressed(KEY_ESCAPE))
    {
        ui.currentScreen = GAME_BOARD;
        ui.isPaused = true;
        ui.pauseMenuIndex = 0;
        return;
    }

    // Backspace: xoá kí tự cuối
    if (IsKeyPressed(KEY_BACKSPACE))
    {
        if (!ui.saveNameInput.empty())
        {
            ui.saveNameInput.pop_back();
            ui.saveNameError = false;
        }
        return;
    }

    // Enter: xác nhận lưu
    if (isConfirm())
    {
        if (ui.saveNameInput.empty())
        {
            ui.saveNameError = true;
            return;
        }

        // Kiểm tra chỉ chứa chữ và số
        bool valid = true;
        for (char c : ui.saveNameInput)
        {
            if (!std::isalnum(static_cast<unsigned char>(c)))
            {
                valid = false;
                break;
            }
        }

        if (!valid)
        {
            ui.saveNameError = true;
            return;
        }

        // Tạo timestamp
        time_t t = time(NULL);
        struct tm timeinfo;
#ifdef _WIN32
        localtime_s(&timeinfo, &t);
#else
        localtime_r(&t, &timeinfo);
#endif
        char timeBuf[32];
        std::strftime(timeBuf, sizeof(timeBuf), "%Y%m%d_%H%M%S", &timeinfo);

        // Tạo tên file: <tên>_<ngày giờ>.txt
        std::string filename = ui.saveNameInput + "_" + timeBuf + ".txt";

        saveGame(match, ui.moveHistory, filename);
        ui.isPaused = false;
        ui.currentScreen = GAME_BOARD;
        return;
    }

    // Nhập kí tự (chỉ nhận chữ, số, không nhận kí tự đặc biệt)
    int key = GetCharPressed();
    while (key > 0)
    {
        // Chỉ cho phép chữ cái (A-Z, a-z) và số (0-9)
        if ((key >= 'A' && key <= 'Z') || (key >= 'a' && key <= 'z') || (key >= '0' && key <= '9'))
        {
            if (ui.saveNameInput.size() < 20) // Giới hạn 20 kí tự
            {
                ui.saveNameInput += (char)key;
            }
        }
        ui.saveNameError = false;
        key = GetCharPressed();
    }
}

void handleSettingsInput(UIState &ui)
{
    const int SETTINGS_COUNT = 7;

    if (isDirUp())
    {
        ui.settingsMenuIndex = getPrevSettingsIndex(ui.settingsMenuIndex, ui.isFullscreen);
        playSFX(SFX_CLICK);
    }

    if (isDirDown())
    {
        ui.settingsMenuIndex = getNextSettingsIndex(ui.settingsMenuIndex, ui.isFullscreen);
        playSFX(SFX_CLICK);
    }

    bool left = isDirLeft();
    bool right = isDirRight();
    bool enter = isConfirm();

    switch (ui.settingsMenuIndex)
    {
    case 0:
    {
        if (left || right || enter)
        {
            ui.isFullscreen = !ui.isFullscreen;
            clampResolutionIndexToMonitor(ui);
            applyDisplaySettings(ui);

            if (ui.isFullscreen)
            {
                ui.settingsMenuIndex = getNextSettingsIndex(0, true);
            }

            playSFX(SFX_CLICK);
        }
        break;
    }

    case 1:
    {
        if (ui.isFullscreen)
        {
            break;
        }

        if (left)
        {
            ui.resolutionIndex = stepResolutionIndex(ui.resolutionIndex, -1);
            applyDisplaySettings(ui);
            playSFX(SFX_CLICK);
        }

        if (right)
        {
            ui.resolutionIndex = stepResolutionIndex(ui.resolutionIndex, 1);
            applyDisplaySettings(ui);
            playSFX(SFX_CLICK);
        }
        break;
    }

    case 2:
    {
        if (left || right || enter)
        {
            toggleMusicEnabled();
            playSFX(SFX_CLICK);
        }
        break;
    }

    case 3:
    {
        if (left)
        {
            setMusicVolume(getMusicVolume() - 0.1f);
        }

        if (right)
        {
            setMusicVolume(getMusicVolume() + 0.1f);
        }
        break;
    }

    case 4:
    {
        if (left || right || enter)
        {
            toggleSFXEnabled();
            playSFX(SFX_CLICK);
        }
        break;
    }

    case 5:
    {
        if (left)
        {
            setSFXVolume(getSFXVolume() - 0.1f);
        }

        if (right)
        {
            setSFXVolume(getSFXVolume() + 0.1f);
        }
        break;
    }

    case 6:
    {
        if (enter || IsKeyPressed(KEY_ESCAPE))
        {
            ui.currentScreen = MAIN_MENU;
            ui.mainMenuIndex = 0;
            playSFX(SFX_CLICK);
        }
        break;
    }
    }

    if (IsKeyPressed(KEY_ESCAPE))
    {
        ui.currentScreen = MAIN_MENU;
        ui.mainMenuIndex = 0;
        playSFX(SFX_CLICK);
    }

    (void)SETTINGS_COUNT;
}
void handleBotDifficultyInput(UIState &ui)
{
    const int totalOptions = 3;

    if (isDirUp())
    {
        ui.botDifficultyIndex = (ui.botDifficultyIndex - 1 + totalOptions) % totalOptions;
    }

    if (isDirDown())
    {
        ui.botDifficultyIndex = (ui.botDifficultyIndex + 1) % totalOptions;
    }

    if (isConfirm())
    {
        ui.botDifficulty = static_cast<BotDifficulty>(ui.botDifficultyIndex);

        ui.currentScreen = CHARACTER_SELECTION;
        ui.isSelectingX = true;
        ui.characterMenuIndex = 1;
    }

    if (IsKeyPressed(KEY_ESCAPE))
    {
        ui.currentScreen = MODE_SELECTION;
        ui.modeMenuIndex = 1;
    }
}

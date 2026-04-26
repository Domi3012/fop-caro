#include "controller.h"
#include "view.h"
#include "model.h"
#include "save_manager.h"
#include <ctime>
#include "bot_ai.h"
#include "audio_manager.h"

// Các hàm static xử lí về lựa chọn Index
static int wrapPrevIndex(int current, int total) {
    if (total <= 0)
    {
        return 0;
    }

    return (current - 1 + total) % total;
}

static int wrapNextIndex(int current, int total) {
    if (total <= 0)
    {
        return 0;
    }

    return (current + 1) % total;
}

static int getPrevSettingsIndex(int current, bool isFullscreen) {
    int idx = current;

    do
    {
        idx = wrapPrevIndex(idx, 7);
    }
    while (isFullscreen && idx == 1);

    return idx;
}

static int getNextSettingsIndex(int current, bool isFullscreen) {
    int idx = current;

    do
    {
        idx = wrapNextIndex(idx, 7);
    }
    while (isFullscreen && idx == 1);

    return idx;
}

// Các hàm static xử lí về Resolution

static bool isResolutionAllowed(int width, int height) {
    int monitor = GetCurrentMonitor();
    return width <= GetMonitorWidth(monitor) && height <= GetMonitorHeight(monitor);
}

static int findPrevAllowedResolutionIndex(int current) {
    for (int step = 1; step <= RESOLUTION_COUNT; step++)
    {
        int idx = (current - step + RESOLUTION_COUNT) % RESOLUTION_COUNT;
        if (isResolutionAllowed(RESOLUTIONS[idx].width, RESOLUTIONS[idx].height))
        {
            return idx;
        }
    }

    return current;
}

static int findNextAllowedResolutionIndex(int current) {
    for (int step = 1; step <= RESOLUTION_COUNT; step++)
    {
        int idx = (current + step) % RESOLUTION_COUNT;
        if (isResolutionAllowed(RESOLUTIONS[idx].width, RESOLUTIONS[idx].height))
        {
            return idx;
        }
    }

    return current;
}

static void clampResolutionIndexToMonitor(UIState& ui) {
    if (!isResolutionAllowed(RESOLUTIONS[ui.resolutionIndex].width,
        RESOLUTIONS[ui.resolutionIndex].height))
    {
        for (int i = RESOLUTION_COUNT - 1; i >= 0; --i)
        {
            if (isResolutionAllowed(RESOLUTIONS[i].width, RESOLUTIONS[i].height))
            {
                ui.resolutionIndex = i;
                return;
            }
        }

        ui.resolutionIndex = 0;
    }
}
static int clampResolutionIndex(int index) {
    if (index < 0)
    {
        return 0;
    }

    if (index >= RESOLUTION_COUNT)
    {
        return RESOLUTION_COUNT - 1;
    }

    return index;
}
static void applyDisplaySettings(UIState& ui) {
    ui.resolutionIndex = clampResolutionIndex(ui.resolutionIndex);
    const ResolutionOption& res = RESOLUTIONS[ui.resolutionIndex];

    int monitor = GetCurrentMonitor();
    int monitorWidth = GetMonitorWidth(monitor);
    int monitorHeight = GetMonitorHeight(monitor);

    if (ui.isFullscreen)
    {
        if (!IsWindowFullscreen())
        {
            SetWindowSize(monitorWidth, monitorHeight);
            SetWindowPosition(0, 0);
            ToggleFullscreen();
        }
    }
    else
    {
        if (IsWindowFullscreen())
        {
            ToggleFullscreen();
        }

        SetWindowSize(res.width, res.height);
        SetWindowPosition(
            (monitorWidth - res.width) / 2,
            (monitorHeight - res.height) / 2);
    }
}
void processMoveAndResult(MatchState &match, UIState &ui, int x, int y)
{
    RoundState &round = match.currentRound;

    if (!checkValidMove(round, x, y))
    {
        return;
    }

    makeMove(round, x, y);
    RoundResult rr = checkRoundResult(round, x, y);

    if (rr == X_WINS || rr == O_WINS)
    {
        round.result = rr;
        match.countRoundsPlayed++;

        Player &attacker = (rr == X_WINS) ? match.playerX : match.playerO;
        Player &defender = (rr == X_WINS) ? match.playerO : match.playerX;
        executeAttack(attacker, defender, round.turnCount);

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
}

// handleMainMenuInput:
// Lên xuống để di chuyển mainMenuIndex
// Enter để xác nhận -> chuyển sang CHARACTER_SELECTION
void handleMainMenuInput(UIState &ui)
{
    const int totalOptions = 4;

    if (IsKeyPressed('W') || IsKeyPressed('w') || IsKeyPressed(KEY_UP))
    {
        ui.mainMenuIndex = wrapPrevIndex(ui.mainMenuIndex, totalOptions);
        playSFX(SFX_CLICK);
    }

    if (IsKeyPressed('S') || IsKeyPressed('s') || IsKeyPressed(KEY_DOWN))
    {
        ui.mainMenuIndex = wrapNextIndex(ui.mainMenuIndex, totalOptions);
        playSFX(SFX_CLICK);
    }

    if (IsKeyPressed(KEY_ENTER))
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
    if (IsKeyPressed('A') || IsKeyPressed('a') || IsKeyPressed(KEY_LEFT))
    {
        if (ui.characterMenuIndex > 1) {
            ui.characterMenuIndex--;
            playSFX(SFX_CLICK);
        }
    }

    if (IsKeyPressed('D') || IsKeyPressed('d') || IsKeyPressed(KEY_RIGHT))
    {
        if (ui.characterMenuIndex < 3) {
            ui.characterMenuIndex++;
            playSFX(SFX_CLICK);
        }
    }

    if (IsKeyPressed(KEY_ENTER))
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
        default:
            chosen = VAMPIRE;
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
            playerX.health = MAX_HEALTH;

            Player playerO;
            playerO.character = match.playerO.character;
            playerO.health = MAX_HEALTH;

            initMatch(match, playerX, playerO);
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

        if (IsKeyPressed('W') || IsKeyPressed('w') || IsKeyPressed(KEY_UP))
        {
            ui.pauseMenuIndex = wrapPrevIndex(ui.pauseMenuIndex, pauseOptionCount);
        }

        if (IsKeyPressed('S') || IsKeyPressed('s') || IsKeyPressed(KEY_DOWN))
        {
            ui.pauseMenuIndex = wrapNextIndex(ui.pauseMenuIndex, pauseOptionCount);
        }

        if (IsKeyPressed(KEY_ENTER))
        {
            if (ui.pauseMenuIndex == 0)
            {
                time_t t = time(NULL);
                struct tm timeinfo;

#ifdef _WIN32
                // Cách dùng của Microsoft Visual Studio
                localtime_s(&timeinfo, &t);
#else
                // Cách dùng chuẩn POSIX cho Linux/macOS
                localtime_r(&t, &timeinfo);
#endif

                char buffer[64];
                std::strftime(buffer, sizeof(buffer), "save_%Y%m%d_%H%M%S.txt", &timeinfo);

                saveGame(match, buffer);
                ui.isPaused = false;
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
    if (IsKeyPressed('W') || IsKeyPressed('w') || IsKeyPressed(KEY_UP))
    {
        if (ui.cursorY > 0)
        {
            ui.cursorY--;
        }
    }

    if (IsKeyPressed('S') || IsKeyPressed('s') || IsKeyPressed(KEY_DOWN))
    {
        if (ui.cursorY < BOARD_SIZE - 1)
        {
            ui.cursorY++;
        }
    }
    // A/D/right/left sẽ là cursorX vì di chuyển theo chiều dọc (col)
    if (IsKeyPressed('A') || IsKeyPressed('a') || IsKeyPressed(KEY_LEFT))
    {
        if (ui.cursorX > 0)
        {
            ui.cursorX--;
        }
    }

    if (IsKeyPressed('D') || IsKeyPressed('d') || IsKeyPressed(KEY_RIGHT))
    {
        if (ui.cursorX < BOARD_SIZE - 1)
        {
            ui.cursorX++;
        }
    }

    // Đặt quân
    if (IsKeyPressed(KEY_ENTER))
    {
        GameScreen prevGameScreen = ui.currentScreen;
        processMoveAndResult(match, ui, ui.cursorY, ui.cursorX);
        
        // Phat SFX dua theo ket qua sau khi dat quan
        if (ui.currentScreen == ROUND_OVER) {
            playSFX(SFX_WIN);
        } else if (ui.currentScreen == GAME_OVER) {
            playSFX(SFX_GAME_OVER);
        } else if (prevGameScreen == GAME_BOARD && ui.currentScreen == GAME_BOARD) {
            playSFX(SFX_PLACE);
        }

        if (ui.currentScreen != GAME_BOARD)
        {
            return;
        }

        if (ui.isPVE && match.currentRound.toMove == O)
        {
            auto botMove = getBestMove(match.currentRound, O, ui.botDifficulty);
            if (botMove.first == -1) 
            {
                match.currentRound.result = DRAW;   // Ghi nhận Hòa
                match.countRoundsPlayed++;          // Tăng số round đã chơi
                ui.currentScreen = ROUND_OVER;      // Chuyển màn hình kết thúc
                ui.roundOverTimer = 0.0f;           // Reset timer cho hiệu ứng chuyển cảnh
            }
            else{
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

        startMatch(ui);
    }
}

// handleGameOverInput:
// Enter -> quay lại MAIN_MENU
// ESC   -> thoát game (đóng cửa sổ raylib)
void handleGameOverInput(MatchState &match, UIState &ui)
{
    if (IsKeyPressed(KEY_ENTER))
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

    if (IsKeyPressed('A') || IsKeyPressed('a') || IsKeyPressed(KEY_LEFT) ||
        IsKeyPressed('W') || IsKeyPressed('w') || IsKeyPressed(KEY_UP))
    {
        ui.modeMenuIndex = wrapPrevIndex(ui.modeMenuIndex, modeCount);
    }

    if (IsKeyPressed('D') || IsKeyPressed('d') || IsKeyPressed(KEY_RIGHT) ||
        IsKeyPressed('S') || IsKeyPressed('s') || IsKeyPressed(KEY_DOWN))
    {
        ui.modeMenuIndex = wrapNextIndex(ui.modeMenuIndex, modeCount);
    }

    if (IsKeyPressed(KEY_ENTER))
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
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE))
    {
        ui.introCamX = 0.0f;
        startMatch(ui);
        return;
    }

    float dt = GetFrameTime();
    ui.roundOverTimer += dt;

    float totalTime = 3.5f; // Epic intro keo dai 3.5s
    float totalDistance = (float)GetScreenWidth() * 5.0f;

    // Su dung Smootherstep ease-in-out: E'(p) = 30 * p^2 * (1-p)^2
    float p = ui.roundOverTimer / totalTime;
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

// handleInput:
// Dispatcher trung tâm, gọi đúng handler theo màn hình hiện tại
void handleInput(MatchState &match, UIState &ui)
{
    // Lấy danh sách save files một lần để tránh gọi nhiều lần trong 1 frame
    static std::vector<std::string> cachedSaveFiles;

    switch (ui.currentScreen)
    {
    case MAIN_MENU:
        handleMainMenuInput(ui);
        // Khi vừa chuyển vào màn LOAD_GAME, refresh danh sách file save
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

void handleLoadGameInput(MatchState &match, UIState &ui, const std::vector<std::string> &saveFiles)
{
    if (saveFiles.empty())
    {
        if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_ENTER))
        {
            ui.currentScreen = MAIN_MENU;
        }
        return;
    }

    const int totalSaves = (int)saveFiles.size();

    if (IsKeyPressed('W') || IsKeyPressed('w') || IsKeyPressed(KEY_UP))
    {
        ui.loadMenuIndex = wrapPrevIndex(ui.loadMenuIndex, totalSaves);
    }

    if (IsKeyPressed('S') || IsKeyPressed('s') || IsKeyPressed(KEY_DOWN))
    {
        ui.loadMenuIndex = wrapNextIndex(ui.loadMenuIndex, totalSaves);
    }

    if (IsKeyPressed(KEY_ENTER))
    {
        if (loadGame(match, saveFiles[ui.loadMenuIndex]))
        {
            startGameIntro(ui);
        }
    }

    if (IsKeyPressed(KEY_ESCAPE))
    {
        ui.currentScreen = MAIN_MENU;
    }
}

void handleSettingsInput(UIState& ui) {
    const int SETTINGS_COUNT = 7;

    if (IsKeyPressed('W') || IsKeyPressed('w') || IsKeyPressed(KEY_UP))
    {
        ui.settingsMenuIndex = getPrevSettingsIndex(ui.settingsMenuIndex, ui.isFullscreen);
        playSFX(SFX_CLICK);
    }

    if (IsKeyPressed('S') || IsKeyPressed('s') || IsKeyPressed(KEY_DOWN))
    {
        ui.settingsMenuIndex = getNextSettingsIndex(ui.settingsMenuIndex, ui.isFullscreen);
        playSFX(SFX_CLICK);
    }

    bool left = IsKeyPressed('A') || IsKeyPressed('a') || IsKeyPressed(KEY_LEFT);
    bool right = IsKeyPressed('D') || IsKeyPressed('d') || IsKeyPressed(KEY_RIGHT);
    bool enter = IsKeyPressed(KEY_ENTER);

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
            ui.resolutionIndex = findPrevAllowedResolutionIndex(ui.resolutionIndex);
            applyDisplaySettings(ui);
            playSFX(SFX_CLICK);
        }

        if (right)
        {
            ui.resolutionIndex = findNextAllowedResolutionIndex(ui.resolutionIndex);
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
void handleBotDifficultyInput(UIState& ui) {
    const int totalOptions = 3;

    if (IsKeyPressed('W') || IsKeyPressed('w') || IsKeyPressed(KEY_UP))
    {
        ui.botDifficultyIndex = (ui.botDifficultyIndex - 1 + totalOptions) % totalOptions;
    }

    if (IsKeyPressed('S') || IsKeyPressed('s') || IsKeyPressed(KEY_DOWN))
    {
        ui.botDifficultyIndex = (ui.botDifficultyIndex + 1) % totalOptions;
    }

    if (IsKeyPressed(KEY_ENTER))
    {
        switch (ui.botDifficultyIndex)
        {
        case 0:
        {
            ui.botDifficulty = EASY;
            break;
        }
        case 1:
        {
            ui.botDifficulty = MEDIUM;
            break;
        }
        case 2:
        {
            ui.botDifficulty = HARD;
            break;
        }
        }

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

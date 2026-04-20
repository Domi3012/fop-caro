#include "controller.h"
#include "view.h"
#include "save_manager.h"
#include <ctime>
#include "audio_manager.h"

static int wrapPrevIndex(int current, int total)
{
    if (total <= 0)
    {
        return 0;
    }

    return (current - 1 + total) % total;
}

//
static int wrapNextIndex(int current, int total)
{
    if (total <= 0)
    {
        return 0;
    }

    return (current + 1) % total;
}

// Áp dụng cài đặt hiển thị mới: Fullscreen và Resolution
static void applyDisplaySettings(UIState &ui)
{
    const ResolutionOption &res = RESOLUTIONS[ui.resolutionIndex];

    if (ui.isFullscreen)
    {
        if (!IsWindowFullscreen())
        {
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

        int monitor = GetCurrentMonitor();
        int monitorWidth = GetMonitorWidth(monitor);
        int monitorHeight = GetMonitorHeight(monitor);

        SetWindowPosition(
            (monitorWidth - res.width) / 2,
            (monitorHeight - res.height) / 2);
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
    }

    if (IsKeyPressed('S') || IsKeyPressed('s') || IsKeyPressed(KEY_DOWN))
    {
        ui.mainMenuIndex = wrapNextIndex(ui.mainMenuIndex, totalOptions);
    }

    if (IsKeyPressed(KEY_ENTER))
    {
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
        if (ui.characterMenuIndex > 1)
            ui.characterMenuIndex--;
    }

    if (IsKeyPressed('D') || IsKeyPressed('d') || IsKeyPressed(KEY_RIGHT))
    {
        if (ui.characterMenuIndex < 3) // 3 nhân vật: index 1,2,3
            ui.characterMenuIndex++;
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
            // Lưu tạm nhân vật X vào match, O sẽ ghi đè sau
            match.playerX.character = chosen;

            // Chuyển sang cho O chọn
            ui.isSelectingX = false;
            ui.characterMenuIndex = 1;
        }
        else
        {
            // O vừa chọn xong
            match.playerO.character = chosen;

            // Tạo 2 player hoàn chỉnh rồi khởi tạo match
            Player playerX;
            playerX.character = match.playerX.character;
            playerX.health = MAX_HEALTH;

            Player playerO;
            playerO.character = match.playerO.character;
            playerO.health = MAX_HEALTH;

            initMatch(match, playerX, playerO);

            // Bat dau intro animation truoc khi vao GAME_BOARD
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
                localtime_s(&timeinfo, &t);

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
        RoundState &round = match.currentRound;

        if (!checkValidMove(round, ui.cursorY, ui.cursorX))
            return;

        makeMove(round, ui.cursorY, ui.cursorX);

        RoundResult rr = checkRoundResult(round, ui.cursorY, ui.cursorX);

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
        // ONGOING: không làm gì, tiếp tục chơi
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
        startMatch(ui); // reset cursor + timer + chuyển sang GAME_BOARD
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
    }

    if (IsKeyPressed(KEY_ESCAPE))
    {
        ui.currentScreen = MAIN_MENU; // Thay vì CloseWindow()
    }

    (void)match;
}

// startGameIntro:
// Cai dat introCamX qua ben phai man hinh va chuyen bien trang thai thanh GAME_INTRO
void startGameIntro(UIState &ui)
{
    ui.introCamX = (float)GetScreenWidth() * 5.0f; // Bat dau xa x5
    ui.roundOverTimer = 0.0f;                      // Su dung tam time nay cho intro
    ui.currentScreen = GAME_INTRO;
}

// startMatch:
// Điểm thống nhất để khởi động game sau khi match đã được khởi tạo.
// Dùng chung cho cả New Game (sau initMatch) lẫn Load Game (sau loadGame)
// và bắt đầu round mới (sau initRound trong handleRoundOverInput).
void startMatch(UIState &ui)
{
    ui.cursorX = BOARD_SIZE / 2;
    ui.cursorY = BOARD_SIZE / 2;
    ui.roundOverTimer = 0.0f; // luôn reset timer khi bắt đầu game/round
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
        ui.currentScreen = CHARACTER_SELECTION;
        ui.isSelectingX = true;
        ui.characterMenuIndex = 1;
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
            startMatch(ui);
        }
    }

    if (IsKeyPressed(KEY_ESCAPE))
    {
        ui.currentScreen = MAIN_MENU;
    }
}

void handleSettingsInput(UIState &ui)
{
    const int SETTINGS_COUNT = 7;

    if (IsKeyPressed('W') || IsKeyPressed('w') || IsKeyPressed(KEY_UP))
    {
        ui.settingsMenuIndex = wrapPrevIndex(ui.settingsMenuIndex, SETTINGS_COUNT);
    }

    if (IsKeyPressed('S') || IsKeyPressed('s') || IsKeyPressed(KEY_DOWN))
    {
        ui.settingsMenuIndex = wrapNextIndex(ui.settingsMenuIndex, SETTINGS_COUNT);
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
            applyDisplaySettings(ui);
        }
        break;
    }

    case 1:
    {
        if (left)
        {
            ui.resolutionIndex = wrapPrevIndex(ui.resolutionIndex, RESOLUTION_COUNT);

            if (!ui.isFullscreen)
            {
                applyDisplaySettings(ui);
            }
        }

        if (right)
        {
            ui.resolutionIndex = wrapNextIndex(ui.resolutionIndex, RESOLUTION_COUNT);

            if (!ui.isFullscreen)
            {
                applyDisplaySettings(ui);
            }
        }
        break;
    }

    case 2:
    {
        if (left || right || enter)
        {
            toggleMusicEnabled();
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
        }
        break;
    }
    }

    if (IsKeyPressed(KEY_ESCAPE))
    {
        ui.currentScreen = MAIN_MENU;
        ui.mainMenuIndex = 0;
    }
}
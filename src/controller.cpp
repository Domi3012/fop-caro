#include "controller.h"
#include "view.h"
#include "save_manager.h"
#include "audio_manager.h"
#include <ctime>


// thêm helper đồng bộ nhạc nền theo màn hình hiện tại
static void syncAudioForScreen(const UIState& ui) {
    switch (ui.currentScreen) {
    case MAIN_MENU:
    case MODE_SELECTION:
    case CHARACTER_SELECTION:
    case LOAD_GAME:
    case SETTINGS:
        playMusic(BGM_MENU);
        break;

    case GAME_INTRO:
    case GAME_BOARD:
    case ROUND_OVER:
    case GAME_OVER:
        playMusic(BGM_BATTLE);
        break;
    }
}

// handleMainMenuInput:
// Lên xuống để di chuyển mainMenuIndex
// Enter để xác nhận -> chuyển sang CHARACTER_SELECTION
void handleMainMenuInput(UIState& ui) {
    if (IsKeyPressed('W') || IsKeyPressed('w') || IsKeyPressed(KEY_UP))
    {
        if (ui.mainMenuIndex > 0) {
            ui.mainMenuIndex--;
            playSFX(SFX_CLICK); // thêm âm thanh di chuyển menu chính
        }
    }

    if (IsKeyPressed('S') || IsKeyPressed('s') || IsKeyPressed(KEY_DOWN))
    {
        if (ui.mainMenuIndex < 3) { // 4 options: index 0,1,2,3 
            ui.mainMenuIndex++;
            playSFX(SFX_CLICK); // thêm âm thanh di chuyển menu chính
        }
    }

    if (IsKeyPressed(KEY_ENTER))
    {
        //Thêm các option sau tại đây
        switch (ui.mainMenuIndex) {
            // Option 0: Start Game
        case(0):
            ui.currentScreen = MODE_SELECTION;
            ui.modeMenuIndex = 0;
            playSFX(SFX_CLICK); // thêm âm thanh xác nhận Start Game
            break;
        case(1):
            ui.currentScreen = LOAD_GAME;
            ui.loadMenuIndex = 0;
            playSFX(SFX_CLICK); // thêm âm thanh xác nhận Load Game
            break;
            // Option 2: Settings
        case(2):
            ui.currentScreen = SETTINGS;
            ui.settingsMenuIndex = 0;
            playSFX(SFX_CLICK); // thêm âm thanh xác nhận Settings
            break;
            // Option 3: Exit game
        case(3):
            ui.shouldExit = true;
            playSFX(SFX_CLICK); // thêm âm thanh xác nhận thoát game
            break;
        }
    }
}


// handleCharSelectionInput:
// Lên xuống để chọn nhân vật (1 = ASSASSIN, 2 = BRUISER, 3 = VAMPIRE)
// Enter để xác nhận
// X chọn xong -> O chọn
// O chọn xong -> initMatch và chuyển sang GAME_BOARD
void handleCharSelectionInput(MatchState& match, UIState& ui) {
    if (IsKeyPressed('A') || IsKeyPressed('a') || IsKeyPressed(KEY_LEFT))
    {
        if (ui.characterMenuIndex > 1) {
            ui.characterMenuIndex--;
            playSFX(SFX_CLICK); // thêm âm thanh di chuyển chọn nhân vật
        }
    }

    if (IsKeyPressed('D') || IsKeyPressed('d') || IsKeyPressed(KEY_RIGHT))
    {
        if (ui.characterMenuIndex < 3) { // 3 nhân vật: index 1,2,3
            ui.characterMenuIndex++;
            playSFX(SFX_CLICK); // thêm âm thanh di chuyển chọn nhân vật
        }
    }

    if (IsKeyPressed(KEY_ENTER))
    {
        // ánh xạ index -> CharacterType
        CharacterType chosen;
        switch (ui.characterMenuIndex)
        {
        case 1:  chosen = ASSASSIN; break;
        case 2:  chosen = BRUISER;  break;
        default: chosen = VAMPIRE;  break;
        }

        if (ui.isSelectingX)
        {
            // Lưu tạm nhân vật X vào match, O sẽ ghi đè sau
            match.playerX.character = chosen;

            // Chuyển sang cho O chọn
            ui.isSelectingX = false;
            ui.characterMenuIndex = 1;
            playSFX(SFX_CLICK); // thêm âm thanh xác nhận nhân vật X
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
            playSFX(SFX_CLICK); // thêm âm thanh xác nhận nhân vật O và bắt đầu trận
        }
    }

    // ESC -> quay lại main menu
    if (IsKeyPressed(KEY_ESCAPE))
    {
        ui.currentScreen = MAIN_MENU;
        ui.mainMenuIndex = 0;
        ui.isSelectingX = true;
        ui.characterMenuIndex = 1;
        playSFX(SFX_CLICK); // thêm âm thanh quay lại main menu từ chọn nhân vật
    }
}


// handleGameplayInput:
// Mũi tên để di chuyển con trỏ trên bàn cờ
// Enter để đặt quân tại ô đang trỏ
// Sau mỗi nước đi hợp lệ kiểm tra kết quả round rồi match
void handleGameplayInput(MatchState& match, UIState& ui) {
    if (ui.isPaused) {
        if (IsKeyPressed('W') || IsKeyPressed('w') || IsKeyPressed(KEY_UP)) {
            if (ui.pauseMenuIndex > 0) {
                ui.pauseMenuIndex--;
                playSFX(SFX_CLICK); // thêm âm thanh di chuyển pause menu
            }
        }
        if (IsKeyPressed('S') || IsKeyPressed('s') || IsKeyPressed(KEY_DOWN)) {
            if (ui.pauseMenuIndex < 1) {
                ui.pauseMenuIndex++;
                playSFX(SFX_CLICK); // thêm âm thanh di chuyển pause menu
            }
        }
        if (IsKeyPressed(KEY_ENTER)) {
            if (ui.pauseMenuIndex == 0) {
                // Save game
                time_t t = time(NULL);
                struct tm timeinfo;
                localtime_s(&timeinfo, &t);

                char buffer[64];
                // Thêm dấu & trước timeinfo ở dòng dưới đây:
                std::strftime(buffer, sizeof(buffer), "save_%Y%m%d_%H%M%S.txt", &timeinfo);

                saveGame(match, buffer);
                ui.isPaused = false; // resumes game after save
                playSFX(SFX_CLICK); // thêm âm thanh lưu game trong pause menu
            }
            else if (ui.pauseMenuIndex == 1) {
                // Exit
                ui.isPaused = false;
                ui.currentScreen = MAIN_MENU;
                ui.mainMenuIndex = 0;
                playSFX(SFX_CLICK); // thêm âm thanh thoát về menu từ pause menu
            }
        }
        if (IsKeyPressed(KEY_ESCAPE)) {
            ui.isPaused = false;
            playSFX(SFX_CLICK); // thêm âm thanh tiếp tục game từ pause menu
        }
        return;
    }

    if (IsKeyPressed(KEY_ESCAPE)) {
        ui.isPaused = true;
        ui.pauseMenuIndex = 0;
        playSFX(SFX_CLICK); // thêm âm thanh mở pause menu
        return;
    }

    // Di chuyển con trỏ
    // W/S/up/down sẽ là cursorY vì di chuyển theo chiều dọc (col)
    if (IsKeyPressed('W') || IsKeyPressed('w') || IsKeyPressed(KEY_UP))
    {
        if (ui.cursorY > 0) {
            ui.cursorY--;
            playSFX(SFX_CLICK); // thêm âm thanh di chuyển con trỏ dọc
        }
    }

    if (IsKeyPressed('S') || IsKeyPressed('s') || IsKeyPressed(KEY_DOWN))
    {
        if (ui.cursorY < BOARD_SIZE - 1) {
            ui.cursorY++;
            playSFX(SFX_CLICK); // thêm âm thanh di chuyển con trỏ dọc
        }
    }
    // A/D/right/left sẽ là cursorX vì di chuyển theo chiều dọc (col)
    if (IsKeyPressed('A') || IsKeyPressed('a') || IsKeyPressed(KEY_LEFT))
    {
        if (ui.cursorX > 0) {
            ui.cursorX--;
            playSFX(SFX_CLICK); // thêm âm thanh di chuyển con trỏ ngang
        }
    }

    if (IsKeyPressed('D') || IsKeyPressed('d') || IsKeyPressed(KEY_RIGHT))
    {
        if (ui.cursorX < BOARD_SIZE - 1) {
            ui.cursorX++;
            playSFX(SFX_CLICK); // thêm âm thanh di chuyển con trỏ ngang
        }
    }

    // Đặt quân
    if (IsKeyPressed(KEY_ENTER))
    {
        RoundState& round = match.currentRound;

        if (!checkValidMove(round, ui.cursorY, ui.cursorX))
            return;

        makeMove(round, ui.cursorY, ui.cursorX);
        playSFX(SFX_PLACE); // thêm âm thanh đặt quân hợp lệ

        RoundResult rr = checkRoundResult(round, ui.cursorY, ui.cursorX);

        if (rr == X_WINS || rr == O_WINS)
        {
            round.result = rr;
            match.countRoundsPlayed++;

            Player& attacker = (rr == X_WINS) ? match.playerX : match.playerO;
            Player& defender = (rr == X_WINS) ? match.playerO : match.playerX;
            executeAttack(attacker, defender, round.turnCount);
            playSFX(SFX_ATTACK); // thêm âm thanh tấn công sau khi thắng round
            if (attacker.character == VAMPIRE)
                playSFX(SFX_HEAL); // thêm âm thanh hồi máu cho Vampire

            RoundResult mr = checkMatchResult(match);
            if (mr == X_WINS || mr == O_WINS)
            {
                match.matchResult = mr;
                ui.currentScreen = GAME_OVER;
                playSFX(SFX_WIN); // thêm âm thanh xác nhận thắng round quyết định
                playSFX(SFX_GAME_OVER); // thêm âm thanh kết thúc trận
            }
            else
            {
                ui.currentScreen = ROUND_OVER;
                playSFX(SFX_WIN); // thêm âm thanh thắng round
            }
        }
        else if (rr == DRAW)
        {
            round.result = DRAW;
            match.countRoundsPlayed++;
            ui.currentScreen = ROUND_OVER;
            playSFX(SFX_CLICK); // thêm âm thanh kết thúc round hòa
        }
        // ONGOING: không làm gì, tiếp tục chơi
    }
}


// handleRoundOverInput:
// Nhấn bất kỳ phím nào để tiếp tục
// Nếu match còn ONGOING -> khởi tạo round mới, quay lại GAME_BOARD
// Nếu match kết thúc   -> chuyển sang GAME_OVER
void handleRoundOverInput(MatchState& match, UIState& ui) {
    ui.roundOverTimer += GetFrameTime();

    if (ui.roundOverTimer < 2.0f)
        return;

    RoundResult mr = checkMatchResult(match);
    if (mr == X_WINS || mr == O_WINS)
    {
        match.matchResult = mr;
        ui.currentScreen = GAME_OVER;
        playSFX(SFX_GAME_OVER); // thêm âm thanh vào màn game over
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
void handleGameOverInput(MatchState& match, UIState& ui) {
    if (IsKeyPressed(KEY_ENTER))
    {
        ui.currentScreen = MAIN_MENU;
        ui.mainMenuIndex = 0;
        ui.isSelectingX = true;
        ui.characterMenuIndex = 1;
        playSFX(SFX_CLICK); // thêm âm thanh xác nhận quay lại main menu từ game over
    }

    if (IsKeyPressed(KEY_ESCAPE))
    {
        ui.currentScreen = MAIN_MENU; // Thay vì CloseWindow()
        playSFX(SFX_CLICK); // thêm âm thanh quay lại main menu từ game over bằng ESC
    }

    (void)match;
}


// startGameIntro:
// Cai dat introCamX qua ben phai man hinh va chuyen bien trang thai thanh GAME_INTRO
void startGameIntro(UIState& ui) {
    ui.introCamX = (float)GetScreenWidth() * 5.0f; // Bat dau xa x5
    ui.roundOverTimer = 0.0f; // Su dung tam time nay cho intro
    ui.currentScreen = GAME_INTRO;
    playMusic(BGM_BATTLE); // thêm nhạc nền battle khi bắt đầu intro
}


// startMatch:
// Điểm thống nhất để khởi động game sau khi match đã được khởi tạo.
// Dùng chung cho cả New Game (sau initMatch) lẫn Load Game (sau loadGame)
// và bắt đầu round mới (sau initRound trong handleRoundOverInput).
void startMatch(UIState& ui) {
    ui.cursorX = BOARD_SIZE / 2;
    ui.cursorY = BOARD_SIZE / 2;
    ui.roundOverTimer = 0.0f; // luôn reset timer khi bắt đầu game/round
    ui.introCamX = 0.0f;
    ui.currentScreen = GAME_BOARD;
    playMusic(BGM_BATTLE); // thêm nhạc nền battle khi vào game board
}

// handleModeSelectionInput:
// W/S hoac A/D de chon giua PVP (0) va PVE (1)
void handleModeSelectionInput(UIState& ui) {
    if (IsKeyPressed('A') || IsKeyPressed('a') || IsKeyPressed(KEY_LEFT) ||
        IsKeyPressed('W') || IsKeyPressed('w') || IsKeyPressed(KEY_UP)) {
        if (ui.modeMenuIndex > 0) {
            ui.modeMenuIndex--;
            playSFX(SFX_CLICK); // thêm âm thanh di chuyển chọn mode
        }
    }
    if (IsKeyPressed('D') || IsKeyPressed('d') || IsKeyPressed(KEY_RIGHT) ||
        IsKeyPressed('S') || IsKeyPressed('s') || IsKeyPressed(KEY_DOWN)) {
        if (ui.modeMenuIndex < 1) {
            ui.modeMenuIndex++;
            playSFX(SFX_CLICK); // thêm âm thanh di chuyển chọn mode
        }
    }
    if (IsKeyPressed(KEY_ENTER)) {
        ui.isPVE = (ui.modeMenuIndex == 1);
        ui.currentScreen = CHARACTER_SELECTION;
        ui.isSelectingX = true;
        ui.characterMenuIndex = 1;
        playSFX(SFX_CLICK); // thêm âm thanh xác nhận mode chơi
    }
    if (IsKeyPressed(KEY_ESCAPE)) {
        ui.currentScreen = MAIN_MENU;
        playSFX(SFX_CLICK); // thêm âm thanh quay lại main menu từ chọn mode
    }
}

// handleGameIntroInput:
// Xu ly logic animation (giam introCamX) hoac skip (Enter/Space)
void handleGameIntroInput(MatchState& match, UIState& ui) {
    // Skip intro
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
        ui.introCamX = 0.0f;
        startMatch(ui);
        playSFX(SFX_CLICK); // thêm âm thanh skip intro
        return;
    }

    float dt = GetFrameTime();
    ui.roundOverTimer += dt;

    float totalTime = 3.5f; // Epic intro keo dai 3.5s
    float totalDistance = (float)GetScreenWidth() * 5.0f;

    // Su dung Smootherstep ease-in-out: E'(p) = 30 * p^2 * (1-p)^2
    float p = ui.roundOverTimer / totalTime;
    if (p >= 1.0f) {
        ui.introCamX = 0.0f;
        startMatch(ui);
        return;
    }

    float ep_prime = 30.0f * p * p * (1.0f - p) * (1.0f - p);
    float velocity = ep_prime * totalDistance / totalTime;

    ui.introCamX -= velocity * dt;

    if (ui.introCamX <= 0.0f) {
        ui.introCamX = 0.0f;
        startMatch(ui);
    }

    (void)match;
}

// handleInput:
// Dispatcher trung tâm, gọi đúng handler theo màn hình hiện tại
void handleInput(MatchState& match, UIState& ui) {
    // Lấy danh sách save files một lần để tránh gọi nhiều lần trong 1 frame
    static std::vector<std::string> cachedSaveFiles;

    switch (ui.currentScreen)
    {
    case MAIN_MENU:
        handleMainMenuInput(ui);
        // Khi vừa chuyển vào màn LOAD_GAME, refresh danh sách file save
        if (ui.currentScreen == LOAD_GAME)
            cachedSaveFiles = getSaveFilesList();
        syncAudioForScreen(ui); // thêm đồng bộ nhạc nền sau khi xử lý main menu
        break;

    case MODE_SELECTION:
        handleModeSelectionInput(ui);
        syncAudioForScreen(ui); // thêm đồng bộ nhạc nền sau khi xử lý chọn mode
        break;

    case CHARACTER_SELECTION:
        handleCharSelectionInput(match, ui);
        syncAudioForScreen(ui); // thêm đồng bộ nhạc nền sau khi xử lý chọn nhân vật
        break;

    case GAME_INTRO:
        handleGameIntroInput(match, ui);
        syncAudioForScreen(ui); // thêm đồng bộ nhạc nền sau khi xử lý intro
        break;

    case LOAD_GAME:
        handleLoadGameInput(match, ui, cachedSaveFiles);
        syncAudioForScreen(ui); // thêm đồng bộ nhạc nền sau khi xử lý load game
        break;

    case SETTINGS:
        handleSettingsInput(ui);
        syncAudioForScreen(ui); // thêm đồng bộ nhạc nền sau khi xử lý settings
        break;

    case GAME_BOARD:
        handleGameplayInput(match, ui);
        syncAudioForScreen(ui); // thêm đồng bộ nhạc nền sau khi xử lý gameplay
        break;

    case ROUND_OVER:
        handleRoundOverInput(match, ui);
        syncAudioForScreen(ui); // thêm đồng bộ nhạc nền sau khi xử lý round over
        break;

    case GAME_OVER:
        handleGameOverInput(match, ui);
        syncAudioForScreen(ui); // thêm đồng bộ nhạc nền sau khi xử lý game over
        break;
    }
}

void handleLoadGameInput(MatchState& match, UIState& ui, const std::vector<std::string>& saveFiles) {
    // Nếu không có file save, ấn phím bất kỳ để thoát ra Main Menu
    if (saveFiles.empty()) {
        if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_ENTER)) {
            ui.currentScreen = MAIN_MENU;
            playSFX(SFX_CLICK); // thêm âm thanh thoát load game khi không có save
        }
        return;
    }

    if (IsKeyPressed('W') || IsKeyPressed('w') || IsKeyPressed(KEY_UP)) {
        if (ui.loadMenuIndex > 0) {
            ui.loadMenuIndex--;
            playSFX(SFX_CLICK); // thêm âm thanh di chuyển danh sách save
        }
    }

    if (IsKeyPressed('S') || IsKeyPressed('s') || IsKeyPressed(KEY_DOWN)) {
        if (ui.loadMenuIndex < (int)saveFiles.size() - 1) {
            ui.loadMenuIndex++;
            playSFX(SFX_CLICK); // thêm âm thanh di chuyển danh sách save
        }
    }

    // Chọn file để load
    if (IsKeyPressed(KEY_ENTER)) {
        // Dùng startMatch() để thống nhất flow chuyển sang GAME_BOARD
        if (loadGame(match, saveFiles[ui.loadMenuIndex])) {
            startMatch(ui);
            playSFX(SFX_CLICK); // thêm âm thanh load game thành công
        }
    }

    // Bấm ESC để quay lại
    if (IsKeyPressed(KEY_ESCAPE)) {
        ui.currentScreen = MAIN_MENU;
        playSFX(SFX_CLICK); // thêm âm thanh quay lại main menu từ load game
    }
}

void handleSettingsInput(UIState& ui) {
    // Bấm ESC để quay lại Main Menu
    if (IsKeyPressed(KEY_ESCAPE)) {
        ui.currentScreen = MAIN_MENU;
        playSFX(SFX_CLICK); // thêm âm thanh quay lại main menu từ settings
    }
}

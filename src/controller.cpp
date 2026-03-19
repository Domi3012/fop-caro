#include "controller.h"
#include "view.h"

// handleMainMenuInput:
// Lên xuống để di chuyển mainMenuIndex
// Enter để xác nhận -> chuyển sang CHARACTER_SELECTION
void handleMainMenuInput(UIState& ui) {
    if (IsKeyPressed('W') || IsKeyPressed('w') || IsKeyPressed(KEY_UP))
    {
        if (ui.mainMenuIndex > 0)
            ui.mainMenuIndex--;
    }

    if (IsKeyPressed('S') || IsKeyPressed('s') || IsKeyPressed(KEY_DOWN))
    {
        // có 2 option nên giới hạn là 1
        // nếu thêm option thì sửa giới hạn này
        if (ui.mainMenuIndex < 1) 
            ui.mainMenuIndex++;
    }

    if (IsKeyPressed(KEY_ENTER))
    {
        //Thêm các option sau tại đây
        switch (ui.mainMenuIndex) {
        // Option 0: Start Game
        case(0):
            ui.currentScreen = CHARACTER_SELECTION;
            ui.isSelectingX = true;
            ui.characterMenuIndex = 1;
            break;
        // Option 1: Exit game
        case(1):
            unloadView();
            CloseWindow();
            exit(0);
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
    if (IsKeyPressed('W') || IsKeyPressed('w') || IsKeyPressed(KEY_UP))
    {
        if (ui.characterMenuIndex > 1)
            ui.characterMenuIndex--;
    }

    if (IsKeyPressed('S') || IsKeyPressed('s') || IsKeyPressed(KEY_DOWN))
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

            // Con trỏ bắt đầu ở giữa bàn cờ
            ui.cursorX = BOARD_SIZE / 2;
            ui.cursorY = BOARD_SIZE / 2;

            ui.currentScreen = GAME_BOARD;
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
void handleGameplayInput(MatchState& match, UIState& ui) {
    // Di chuyển con trỏ
    // W/S/up/down sẽ là cursorY vì di chuyển theo chiều dọc (col)
    if (IsKeyPressed('W') || IsKeyPressed('w') || IsKeyPressed(KEY_UP))
    {
        if (ui.cursorY > 0){
            ui.cursorY--;
        }
    }

    if (IsKeyPressed('S') || IsKeyPressed('s') || IsKeyPressed(KEY_DOWN))
    {
        if (ui.cursorY < BOARD_SIZE - 1) {
            ui.cursorY++;
        }
    }
    // A/D/right/left sẽ là cursorX vì di chuyển theo chiều dọc (col)
    if (IsKeyPressed('A') || IsKeyPressed('a') || IsKeyPressed(KEY_LEFT))
    {
        if (ui.cursorX > 0) {
            ui.cursorX--;
        }
    }

    if (IsKeyPressed('D') || IsKeyPressed('d') || IsKeyPressed(KEY_RIGHT))
    {
        if (ui.cursorX < BOARD_SIZE - 1) {
            ui.cursorX++;
        }
    }

    // Đặt quân
    if (IsKeyPressed(KEY_ENTER))
    {
        RoundState& round = match.currentRound;

        if (!checkValidMove(round, ui.cursorY, ui.cursorX))
            return;

        makeMove(round, ui.cursorY, ui.cursorX);

        RoundResult rr = checkRoundResult(round, ui.cursorY, ui.cursorX);

        if (rr == X_WINS || rr == O_WINS)
        {
            round.result = rr;
            match.countRoundsPlayed++;

            Player& attacker = (rr == X_WINS) ? match.playerX : match.playerO;
            Player& defender = (rr == X_WINS) ? match.playerO : match.playerX;
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
void handleRoundOverInput(MatchState& match, UIState& ui) {
    static float timer = 0.0f;

    timer += GetFrameTime();

    if (timer < 2.0f)
        return;

    timer = 0.0f;

    RoundResult mr = checkMatchResult(match);
    if (mr == X_WINS || mr == O_WINS)
    {
        match.matchResult = mr;
        ui.currentScreen = GAME_OVER;
    }
    else
    {
        initRound(match.currentRound, match.countRoundsPlayed);
        ui.cursorX = BOARD_SIZE / 2;
        ui.cursorY = BOARD_SIZE / 2;
        ui.currentScreen = GAME_BOARD;
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
    }

    if (IsKeyPressed(KEY_ESCAPE))
    {
        unloadView();
        CloseWindow();
        exit(0);
    }

    (void)match;
}


// handleInput:
// Dispatcher trung tâm, gọi đúng handler theo màn hình hiện tại
void handleInput(MatchState& match, UIState& ui) {
    switch (ui.currentScreen)
    {
    case MAIN_MENU:
        handleMainMenuInput(ui);
        break;

    case CHARACTER_SELECTION:
        handleCharSelectionInput(match, ui);
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
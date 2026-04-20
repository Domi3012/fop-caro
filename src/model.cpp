#include "model.h"
#include <cstdlib> // dùng cho rand()


// ======================================================
// Các hàm khởi tạo
// ======================================================


// initMatch: khởi tạo toàn bộ trạng thái của một ván đấu
// - gán player X và player O
// - reset máu về MAX_HEALTH
// - reset số round đã chơi
// - gọi initRound để tạo round đầu tiên
void initMatch(MatchState& matchState, const Player& playerX, const Player& playerY)
{
    // gán thông tin player
    matchState.playerX = playerX;
    matchState.playerO = playerY;

    // đảm bảo máu bắt đầu là MAX_HEALTH
    matchState.playerX.health = MAX_HEALTH;
    matchState.playerO.health = MAX_HEALTH;

    // chưa chơi round nào
    matchState.countRoundsPlayed = 0;

    // kết quả match đang tiếp diễn
    matchState.matchResult = ONGOING;

    // khởi tạo round đầu tiên
    initRound(matchState.currentRound, matchState.countRoundsPlayed);
}



// initRound: khởi tạo một round mới
// roundCount dùng để quyết định ai đi trước
// vòng chẵn -> X đi trước
// vòng lẻ -> O đi trước
void initRound(RoundState& roundState, int roundCount)
{
    // reset số lượt
    roundState.turnCount = 0;

    // round chưa có kết quả
    roundState.result = ONGOING;

    // xóa bàn cờ cũ nếu có
    roundState.board.clear();

    // tạo bàn cờ BOARD_SIZE x BOARD_SIZE
    for (int i = 0; i < BOARD_SIZE; i++)
    {
        vector<PlayerType> row;

        for (int j = 0; j < BOARD_SIZE; j++)
        {
            // ban đầu tất cả ô đều trống
            row.push_back(NONE);
        }

        roundState.board.push_back(row);
    }

    // quyết định người đi trước
    if (roundCount % 2 == 0)
        roundState.toMove = X;
    else
        roundState.toMove = O;
}



// ======================================================
// Các hàm xử lý logic trò chơi
// ======================================================


// checkValidMove:
// kiểm tra một nước đi có hợp lệ hay không
// điều kiện:
// 1. phải nằm trong bàn cờ
// 2. ô đó chưa có quân
// 3. round chưa kết thúc
bool checkValidMove(const RoundState& roundState, int x, int y)
{
    // kiểm tra ngoài biên bàn cờ
    if (x < 0 || y < 0 || x >= BOARD_SIZE || y >= BOARD_SIZE)
        return false;

    // ô đã có quân
    if (roundState.board[x][y] != NONE)
        return false;

    // round đã kết thúc
    if (roundState.result != ONGOING)
        return false;

    return true;
}



// makeMove:
// thực hiện một nước đi
// - đặt quân lên bàn cờ
// - tăng turnCount
// - đổi lượt người chơi
void makeMove(RoundState& roundState, int x, int y)
{
    // nếu nước đi không hợp lệ thì bỏ qua
    if (checkValidMove(roundState, x, y) == false)
        return;

    // đặt quân của người chơi hiện tại
    roundState.board[x][y] = roundState.toMove;

    // tăng số lượt
    roundState.turnCount++;

    // đổi lượt người chơi
    if (roundState.toMove == X)
        roundState.toMove = O;
    else
        roundState.toMove = X;
}



// checkRoundResult:
// kiểm tra sau mỗi nước đi xem round đã kết thúc chưa
// điều kiện thắng: 5 quân liên tiếp
// Khi phát hiện người thắng: lưu tọa độ 5 ô đó vào winningCells trong RoundState
RoundResult checkRoundResult(RoundState& roundState, int lastMoveX, int lastMoveY)
{
    PlayerType player = roundState.board[lastMoveX][lastMoveY];
    if (player == NONE) return ONGOING;

    // Các hướng kiểm tra: Ngang, Dọc, Chéo xuôi, Chéo ngược
    int dx[] = {1, 0, 1, 1}; 
    int dy[] = {0, 1, 1, -1};

    for (int i = 0; i < 4; i++) {
        int count = 1;
        vector<pair<int, int>> cells = {{lastMoveX, lastMoveY}};

        // Kiểm tra hướng tiến
        int nx = lastMoveX + dx[i], ny = lastMoveY + dy[i];
        while (nx >= 0 && nx < BOARD_SIZE && ny >= 0 && ny < BOARD_SIZE && roundState.board[nx][ny] == player) {
            count++;
            cells.push_back({nx, ny});
            nx += dx[i]; ny += dy[i];
        }

        // Kiểm tra hướng lùi
        nx = lastMoveX - dx[i]; ny = lastMoveY - dy[i];
        while (nx >= 0 && nx < BOARD_SIZE && ny >= 0 && ny < BOARD_SIZE && roundState.board[nx][ny] == player) {
            count++;
            cells.push_back({nx, ny});
            nx -= dx[i]; ny -= dy[i];
        }

        if (count >= 5) {
            // Chỉ lấy đúng 5 ô đầu tiên để highlight (hoặc cả chuỗi nếu muốn)
            roundState.winningCells = cells; 
            return (player == X) ? X_WINS : O_WINS;
        }
    }

    // Xử lý kết quả HÒA: Nếu bàn cờ đầy mà không ai thắng
    if (roundState.turnCount >= BOARD_SIZE * BOARD_SIZE) {
        return DRAW;
    }

    return ONGOING;
}



// executeAttack:
// sau khi kết thúc một round thắng
// người thắng sẽ gây damage lên đối thủ
// damage phụ thuộc vào loại character
void executeAttack(Player& attacker, Player& defender, int turnCount)
{
    int damage = 0;

    int boardTotal = BOARD_SIZE * BOARD_SIZE;

    int fifthBoard = boardTotal / 5;
    int threeFifthBoard = (boardTotal * 3) / 5;


    // ==============================
    // ASSASSIN
    // ==============================
    if (attacker.character == ASSASSIN)
    {
        // late game -> kết liễu
        if (turnCount > threeFifthBoard)
        {
            damage = defender.health;
        }

        // mid game -> damage 60-80
        else if (turnCount > fifthBoard)
        {
            damage = 60 + (turnCount - fifthBoard) * 20 / (threeFifthBoard - fifthBoard);
        }

        // early game
        else
        {
            damage = 35;
        }
    }


    // ==============================
    // BRUISER
    // ==============================
    else if (attacker.character == BRUISER)
    {
        damage = 60;
    }


    // ==============================
    // VAMPIRE
    // ==============================
    else if (attacker.character == VAMPIRE)
    {
        damage = 50;

        int heal = 15 + rand() % 11;

        attacker.health = attacker.health + heal;

        if (attacker.health > MAX_HEALTH)
            attacker.health = MAX_HEALTH;
    }


    // trừ máu defender
    defender.health = defender.health - damage;

    if (defender.health < 0)
        defender.health = 0;
}



// checkMatchResult:
// kiểm tra xem trận đấu đã kết thúc chưa
// nếu máu một bên = 0 thì bên kia thắng
RoundResult checkMatchResult(const MatchState& matchState)
{
    if (matchState.playerX.health <= 0)
        return O_WINS;

    if (matchState.playerO.health <= 0)
        return X_WINS;

    return ONGOING;
}

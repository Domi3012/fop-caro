#include "model.h"
#include <cstdlib>


// ============================================================
// Hằng số nội bộ cho executeAttack
// ============================================================

static constexpr int ASSASSIN_EARLY_DAMAGE      = 35;
static constexpr int ASSASSIN_MID_DAMAGE_BASE   = 60;
static constexpr int ASSASSIN_MID_DAMAGE_BONUS  = 20;

static constexpr int BRUISER_DAMAGE             = 60;

static constexpr int VAMPIRE_DAMAGE             = 50;
static constexpr int VAMPIRE_HEAL_BASE          = 15;
static constexpr int VAMPIRE_HEAL_RANDOM        = 11; // rand() % VAMPIRE_HEAL_RANDOM


// ============================================================
// Khởi tạo
// ============================================================

void initMatch(MatchState& matchState,
               const Player& playerX,
               const Player& playerO)
{
    matchState.playerX = playerX;
    matchState.playerO = playerO;

    matchState.playerX.health    = MAX_HEALTH;
    matchState.playerO.health    = MAX_HEALTH;
    matchState.countRoundsPlayed = 0;
    matchState.matchResult       = ONGOING;

    initRound(matchState.currentRound, matchState.countRoundsPlayed);
}

void initRound(RoundState& roundState, int roundCount)
{
    roundState.turnCount = 0;
    roundState.result    = ONGOING;

    // Khởi tạo bàn cờ trống BOARD_SIZE x BOARD_SIZE
    roundState.board.assign(BOARD_SIZE,
                            vector<PlayerType>(BOARD_SIZE, NONE));

    // Vòng chẵn → X đi trước, vòng lẻ → O đi trước
    roundState.toMove = (roundCount % 2 == 0) ? X : O;
}


// ============================================================
// Logic trò chơi
// ============================================================

bool checkValidMove(const RoundState& roundState, int x, int y)
{
    if (x < 0 || x >= BOARD_SIZE || y < 0 || y >= BOARD_SIZE)
        return false;

    if (roundState.board[x][y] != NONE)
        return false;

    if (roundState.result != ONGOING)
        return false;

    return true;
}

void makeMove(RoundState& roundState, int x, int y)
{
    if (!checkValidMove(roundState, x, y))
        return;

    roundState.board[x][y] = roundState.toMove;
    roundState.turnCount++;
    roundState.toMove = (roundState.toMove == X) ? O : X;
}


// Đếm quân liên tiếp của `player` từ (x, y) theo hướng (dx, dy) và ngược lại
static int countDir(const vector<vector<PlayerType>>& board,
                    int x, int y,
                    int dx, int dy,
                    PlayerType player)
{
    int count = 0;

    // Chiều thuận
    for (int nx = x + dx, ny = y + dy;
         nx >= 0 && nx < BOARD_SIZE && ny >= 0 && ny < BOARD_SIZE
         && board[nx][ny] == player;
         nx += dx, ny += dy)
        count++;

    // Chiều ngược
    for (int nx = x - dx, ny = y - dy;
         nx >= 0 && nx < BOARD_SIZE && ny >= 0 && ny < BOARD_SIZE
         && board[nx][ny] == player;
         nx -= dx, ny -= dy)
        count++;

    return count;
}

RoundResult checkRoundResult(const RoundState& roundState, int lastMoveX, int lastMoveY)
{
    PlayerType player = roundState.board[lastMoveX][lastMoveY];
    if (player == NONE)
        return ONGOING;

    // Bốn hướng: ngang, dọc, chéo \, chéo /
    const int dirs[4][2] = { {1,0}, {0,1}, {1,1}, {1,-1} };

    for (auto& d : dirs)
    {
        // +1 cho ô vừa đặt
        int total = 1 + countDir(roundState.board,
                                  lastMoveX, lastMoveY,
                                  d[0], d[1],
                                  player);
        if (total >= WIN_LENGTH)
            return (player == X) ? X_WINS : O_WINS;
    }

    if (roundState.turnCount == BOARD_SIZE * BOARD_SIZE)
        return DRAW;

    return ONGOING;
}

void executeAttack(Player& attacker, Player& defender, int turnCount)
{
    int damage     = 0;
    int boardTotal = BOARD_SIZE * BOARD_SIZE;

    // Mốc early/mid/late game theo tỉ lệ số ô
    int earlyEnd = boardTotal / 5;
    int midEnd   = (boardTotal * 3) / 5;

    switch (attacker.character)
    {
    case ASSASSIN:
        if (turnCount > midEnd)
            damage = defender.health;                          // Kết liễu (late game)
        else if (turnCount > earlyEnd)
            damage = ASSASSIN_MID_DAMAGE_BASE
                   + (turnCount - earlyEnd)
                   * ASSASSIN_MID_DAMAGE_BONUS
                   / (midEnd - earlyEnd);                      // Scale tuyến tính (mid game)
        else
            damage = ASSASSIN_EARLY_DAMAGE;                    // Cố định (early game)
        break;

    case BRUISER:
        damage = BRUISER_DAMAGE;
        break;

    case VAMPIRE:
        damage = VAMPIRE_DAMAGE;
        {
            int heal = VAMPIRE_HEAL_BASE + rand() % VAMPIRE_HEAL_RANDOM;
            attacker.health = std::min(attacker.health + heal, MAX_HEALTH);
        }
        break;
    }

    defender.health = std::max(defender.health - damage, 0);
}

RoundResult checkMatchResult(const MatchState& matchState)
{
    if (matchState.playerX.health <= 0) return O_WINS;
    if (matchState.playerO.health <= 0) return X_WINS;
    return ONGOING;
}


// ============================================================
// Bảng độ phân giải
// ============================================================

const ResolutionOption RESOLUTIONS[] =
{
    {1280,  720,  "1280x720 (16:9)" },
    {1366,  768,  "1366x768 (16:9)" },
    {1600,  900,  "1600x900 (16:9)" },
    {1920, 1080,  "1920x1080 (16:9)"},
    {1920, 1200,  "1920x1200 (16:10)"},
    {2560, 1440,  "2560x1440 (16:9)" },
    {2560, 1600,  "2560x1600 (16:10)"},
    {2880, 1620,  "2880x1620 (16:9)" },
    {2880, 1800,  "2880x1800 (16:10)"},
    {3840, 2160,  "3840x2160 (16:9)" },
};

const int RESOLUTION_COUNT = sizeof(RESOLUTIONS) / sizeof(RESOLUTIONS[0]);

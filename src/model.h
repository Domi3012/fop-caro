#pragma once

#include <string>
#include <vector>

using std::string;
using std::vector;

// ============================================================
// I. Hằng số
// ============================================================

constexpr int MAX_HEALTH = 100; // Máu tối đa của mỗi nhân vật
constexpr int BOARD_SIZE = 12;  // Kích thước bàn cờ (12 x 12)

// Số quân liên tiếp tối thiểu để thắng
constexpr int WIN_LENGTH = 5;

// ============================================================
// II. Enum
// ============================================================

enum PlayerType
{
    X,
    O,
    NONE
};
enum CharacterType
{
    ASSASSIN,
    BRUISER,
    VAMPIRE,
    SORCERER
};
enum RoundResult
{
    ONGOING,
    DRAW,
    X_WINS,
    O_WINS
};

// ============================================================
// III. Struct dữ liệu trò chơi
// ============================================================

struct Player
{
    string name;
    CharacterType character;
    int health = MAX_HEALTH;
    int maxHealth = MAX_HEALTH;
    int sorcererStacks = 0;
};

struct RoundState
{
    PlayerType toMove;
    int turnCount;
    vector<vector<PlayerType>> board;
    RoundResult result;
    vector<std::pair<int, int>> winningCells;
};

struct MatchState
{
    Player playerX;
    Player playerO;
    RoundState currentRound;
    int countRoundsPlayed;
    RoundResult matchResult;
};

struct Move
{
    int x;           // row
    int y;           // col
    PlayerType type; // X or O
};

struct ResolutionOption
{
    int width;
    int height;
    const char *label;
};

extern const ResolutionOption RESOLUTIONS[];
extern const int RESOLUTION_COUNT;

// ============================================================
// IV. Hàm khởi tạo
// ============================================================

void initMatch(MatchState &matchState,
               const Player &playerX,
               const Player &playerO);

// roundCount chẵn → X đi trước; lẻ → O đi trước
void initRound(RoundState &roundState, int roundCount);

// ============================================================
// V. Hàm xử lý logic trò chơi
// ============================================================

bool checkValidMove(const RoundState &roundState, int x, int y);
void makeMove(RoundState &roundState, int x, int y);
RoundResult checkRoundResult(RoundState &roundState, int lastMoveX, int lastMoveY);
void executeAttack(Player &attacker, Player &defender, int turnCount);
RoundResult checkMatchResult(const MatchState &matchState);
int getBaseHealth(CharacterType type);

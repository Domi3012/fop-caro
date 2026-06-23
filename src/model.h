#pragma once

#include <string>
#include <vector>

using std::string;
using std::vector;

// ============================================================
// I. Hằng số
// ============================================================

constexpr int BOARD_SIZE = 13;  // Kích thước bàn cờ (13 x 13)

// Số quân liên tiếp tối thiểu để thắng
constexpr int WIN_LENGTH = 5;

// Undo/Redo capacity
constexpr int MAX_UNDO_CAPACITY = 100; // Maximum number of moves that can be undone

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
    int health = 0;
    int maxHealth = 0;
    int baseDamage = 0;        // Sát thương cơ bản (có thể thay đổi, VD: Assassin)
    int sorcererStacks = 0;    // Số stack poison của Sorcerer đang chịu
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

// Move record with undo status for undo/redo functionality
struct MoveRecord
{
    int row;              // 0 to BOARD_SIZE-1
    int col;              // 0 to BOARD_SIZE-1
    PlayerType player;    // X or O
    bool isUndone;        // true if this move has been undone
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

// ============================================================
// VI. Hàm xử lý tên người chơi
// ============================================================

// Validates if a character is allowed in player names
// Accepts: alphanumeric (A-Z, a-z, 0-9), space, hyphen (-), apostrophe ('), Vietnamese Unicode
bool isValidNameChar(int key);

// Removes leading and trailing whitespace from a string
string trimWhitespace(const string& str);

int getBaseHealth(CharacterType type);
int getBaseDamage(CharacterType type);

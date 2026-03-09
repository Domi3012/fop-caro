#ifndef MODEL_H
#define MODEL_H

#include <string>

#define BOARD_SIZE 19

using std::string;

enum class CellState {
    Empty,
    X,
    O
};

enum class GameStatus {
    Ongoing,
    X_Wins,
    O_Wins,
    Draw
};

enum class GameType {
    HumanVsHuman,
    HumanVsAI,
};

struct GameState { // Struct to hold the game state
    GameType gameType;  
    string xName;
    string oName;
    
    // Match = tran dau tong, round = 1 van caro
    // Match info:
    GameStatus matchStatus;
    int xHealth;
    int oHealth;
    int damagePerRound;
    
    // Round info:
    CellState board[BOARD_SIZE][BOARD_SIZE];
    int roundCount;
    GameStatus roundStatus;
    CellState turn; // 'X' or 'O'
    int turnCount;
};

// Setup
void InitGameState(GameState& state, GameType type, string p1Name, string p2Name);
void ResetBoard(GameState& state);

// Logic
bool CheckWinner(const GameState& state, int lastMoveRow, int lastMoveCol);


struct AppSetting {
    int musicVolumePercentage; // 0 to 100
    int sfxVolumePercentage;   // 0 to 100
};


#endif // MODEL_H
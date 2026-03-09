#include "model.h"
#include <algorithm> // for min();

using std::min;

void InitGameState(GameState& state, GameType type, string p1Name, string p2Name) {
    state.gameType = type;
    // To be done    

}

void ResetBoard(GameState& state) {
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            state.board[i][j] = CellState::Empty;
        }
    }

}


bool CheckWinner(const GameState& state, int lastMoveRow, int lastMoveCol) {
    CellState lastMovePlayer = state.board[lastMoveRow][lastMoveCol];
    // 4 phuong de check
    // moi phuong co 2 chieu
    // moi chieu co 2 thong so dr (huong doi theo hang) va dc (huong doi theo cot)
    const int direction[][4][2] = { 
        {{1, 0}, {-1, 0}}, // doc
        {{0, 1}, {0, -1}}, // ngang
        {{1, 1}, {-1, -1}}, // duong cheo phu
        {{1, -1}, {-1, 1}}  // duong cheo chinh
    };

    for (int d = 0; d < 4; d++) { // Vong lap cho phuong (4 phuong)
        int count = 1;
        for (int dir = 0; dir < 2; dir++) { // Vong lap cho chieu (2 chieu)
            int dr = direction[d][dir][0];
            int dc = direction[d][dir][1];
            int r = lastMoveRow + dr;
            int c = lastMoveCol + dc;
            while (state.board[r][c] == lastMovePlayer &&
                   r >= 0 && r < BOARD_SIZE &&
                   c >= 0 && c < BOARD_SIZE    
            ) { // Neu cung x hoac cung o va dang trong pham vi ban co
                count++;
                // di tiep theo huong do
                r += dr; 
                c += dc;
            }
        }
        if (count >= 5) {
            return true; // nguoi choi last move thang
        }
    }
    return false; // tro choi tiep tuc
}

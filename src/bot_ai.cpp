#include "bot_ai.h"
#include <vector>
#include <algorithm>
#include <iostream>
using namespace std;

// Bảng điểm Heuristic cơ bản
long long attackScore[] = {0, 10, 100, 1000, 10000, 100000}; // Điểm tấn công
long long defendScore[] = {0, 5, 50, 500, 5000, 50000};      // Điểm phòng thủ (thấp hơn để ưu tiên công)

long long evaluateCell(const RoundState &rs, int x, int y, PlayerType botType)
{
    long long totalScore = 0;
    PlayerType opponent = (botType == X) ? O : X;

    int dx[] = {1, 0, 1, 1};
    int dy[] = {0, 1, 1, -1};

    for (int i = 0; i < 4; i++)
    {
        // Tính điểm tấn công cho Bot
        int countBot = 0;
        for (int step = 1; step <= 4; step++)
        {
            int nx = x + dx[i] * step, ny = y + dy[i] * step;
            if (nx >= 0 && nx < BOARD_SIZE && ny >= 0 && ny < BOARD_SIZE && rs.board[nx][ny] == botType)
                countBot++;
            else
                break;
        }
        // Tương tự cho phía đối diện
        totalScore += attackScore[countBot];

        // Tính điểm phòng thủ (chặn đối thủ)
        int countOp = 0;
        for (int step = 1; step <= 4; step++)
        {
            int nx = x + dx[i] * step, ny = y + dy[i] * step;
            if (nx >= 0 && nx < BOARD_SIZE && ny >= 0 && ny < BOARD_SIZE && rs.board[nx][ny] == opponent)
                countOp++;
            else
                break;
        }
        totalScore += defendScore[countOp];
    }
    return totalScore;
}

std::pair<int, int> getBestMove(const RoundState &roundState, PlayerType botType, BotDifficulty difficulty)
{
    if (difficulty == EASY)
    {
        // Đánh ngẫu nhiên vào ô trống
        for (int i = 0; i < BOARD_SIZE; i++)
            for (int j = 0; j < BOARD_SIZE; j++)
                if (roundState.board[i][j] == NONE)
                    return {i, j};
    }

    // MEDIUM & HARD: Tìm ô có điểm cao nhất
    long long maxScore = -1;
    pair<int, int> bestMove = {-1, -1};

    for (int i = 0; i < BOARD_SIZE; i++)
    {
        for (int j = 0; j < BOARD_SIZE; j++)
        {
            if (roundState.board[i][j] == NONE)
            {
                long long score = evaluateCell(roundState, i, j, botType);
                if (score > maxScore)
                {
                    maxScore = score;
                    bestMove = {i, j};
                }
            }
        }
    }
    return bestMove;
}

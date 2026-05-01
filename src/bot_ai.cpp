#include "bot_ai.h"
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <climits>
using namespace std;
 
// ================================================================
// BẢNG ĐIỂM
// scoreTable[số quân liên tiếp][số đầu mở]
// openEnds: 0 = bị chặn 2 đầu, 1 = mở 1 đầu, 2 = mở 2 đầu
// ================================================================
static long long scoreTable[6][3] = {
    {0,        0,        0       }, // 0 quân
    {0,        10,       20      }, // 1 quân
    {0,        100,      200     }, // 2 quân
    {0,        1000,     3000    }, // 3 quân
    {0,        80000,    150000  }, // 4 quân — sắp thắng
    {0,        1000000,  1000000 }, // 5 quân = thắng
};
 
struct LineInfo {
    int count;
    int openEnds;
};
 
static LineInfo countLine(const vector<vector<PlayerType>> &board,
                          int x, int y, int dx, int dy, PlayerType player)
{
    int count    = 1;
    int openEnds = 0;
 
    // --- Chiều thuận (+dx, +dy) ---
    int nx = x + dx, ny = y + dy;
    while (nx >= 0 && nx < BOARD_SIZE && ny >= 0 && ny < BOARD_SIZE
           && board[nx][ny] == player)
    {
        count++;
        nx += dx;
        ny += dy;
    }
    if (nx >= 0 && nx < BOARD_SIZE && ny >= 0 && ny < BOARD_SIZE
        && board[nx][ny] == NONE)
        openEnds++;
 
    // --- Chiều ngược (-dx, -dy) ---
    nx = x - dx; ny = y - dy;
    while (nx >= 0 && nx < BOARD_SIZE && ny >= 0 && ny < BOARD_SIZE
           && board[nx][ny] == player)
    {
        count++;
        nx -= dx;
        ny -= dy;
    }
    if (nx >= 0 && nx < BOARD_SIZE && ny >= 0 && ny < BOARD_SIZE
        && board[nx][ny] == NONE)
        openEnds++;
 
    return {count, openEnds};
}
 
long long evaluateCell(const RoundState &rs, int x, int y, PlayerType player)
{
    // Đặt quân tạm — board là vector nên cast an toàn
    const_cast<vector<vector<PlayerType>>&>(rs.board)[x][y] = player;
 
    const int dx[] = {1, 0, 1,  1};
    const int dy[] = {0, 1, 1, -1};
 
    long long score = 0;
    for (int i = 0; i < 4; i++)
    {
        LineInfo li = countLine(rs.board, x, y, dx[i], dy[i], player);
        int cnt     = min(li.count, 5);
        score      += scoreTable[cnt][li.openEnds];
    }
 
    // Hoàn tác
    const_cast<vector<vector<PlayerType>>&>(rs.board)[x][y] = NONE;
    return score;
}

 
// Tổng điểm công + thủ cho một ô
static long long cellScore(const RoundState &rs, int x, int y, PlayerType botType)
{
    PlayerType opp = (botType == X) ? O : X;
    long long  atk = evaluateCell(rs, x, y, botType);
    long long  def = evaluateCell(rs, x, y, opp);
    return atk * 2 + def; // Tấn công ưu tiên hơn phòng thủ
}
 
// Kiểm tra ô (x, y) có ít nhất một quân đã đánh trong bán kính `radius`
static bool isNearPiece(const RoundState &rs, int x, int y, int radius = 2)
{
    int rMin = max(0, x - radius), rMax = min(BOARD_SIZE - 1, x + radius);
    int cMin = max(0, y - radius), cMax = min(BOARD_SIZE - 1, y + radius);
    for (int i = rMin; i <= rMax; i++)
        for (int j = cMin; j <= cMax; j++)
            if (rs.board[i][j] != NONE)
                return true;
    return false;
}
 
static bool isBoardEmpty(const RoundState &rs)
{
    for (int i = 0; i < BOARD_SIZE; i++)
        for (int j = 0; j < BOARD_SIZE; j++)
            if (rs.board[i][j] != NONE)
                return false;
    return true;
}
 
static vector<pair<int,int>> getCandidates(const RoundState &rs, int radius)
{
    vector<pair<int,int>> result;
    for (int i = 0; i < BOARD_SIZE; i++)
        for (int j = 0; j < BOARD_SIZE; j++)
            if (rs.board[i][j] == NONE && isNearPiece(rs, i, j, radius))
                result.push_back({i, j});
    return result;
}
 
pair<int, int> getBestMove(const RoundState &roundState,
                            PlayerType botType,
                            BotDifficulty difficulty)
{
    // Bàn trống → đánh giữa bàn
    if (isBoardEmpty(roundState))
        return {BOARD_SIZE / 2, BOARD_SIZE / 2};
 
    PlayerType opp = (botType == X) ? O : X;
 
    // ----------------------------------------------------------------
    // EASY: đánh ngẫu nhiên vào ô trống gần quân đã có
    // ----------------------------------------------------------------
    if (difficulty == EASY)
    {
        srand((unsigned)time(nullptr));
        vector<pair<int,int>> cands = getCandidates(roundState, 2);
        if (!cands.empty())
            return cands[rand() % cands.size()];
 
        // Fallback: ô trống bất kỳ
        for (int i = 0; i < BOARD_SIZE; i++)
            for (int j = 0; j < BOARD_SIZE; j++)
                if (roundState.board[i][j] == NONE)
                    return {i, j};
    }
 
    // ----------------------------------------------------------------
    // MEDIUM & HARD: Heuristic nhiều bước ưu tiên
    // ----------------------------------------------------------------
    const long long WIN_SCORE   = scoreTable[5][1]; // 1.000.000 — thắng ngay
    const long long OPEN4_SCORE = scoreTable[4][2]; // 150.000  — 4 quân 2 đầu mở
 
    // Bước 1: Đánh thắng ngay nếu có thể
    for (int i = 0; i < BOARD_SIZE; i++)
        for (int j = 0; j < BOARD_SIZE; j++)
            if (roundState.board[i][j] == NONE)
                if (evaluateCell(roundState, i, j, botType) >= WIN_SCORE)
                    return {i, j};
 
    // Bước 2: Chặn nếu đối thủ sắp thắng
    for (int i = 0; i < BOARD_SIZE; i++)
        for (int j = 0; j < BOARD_SIZE; j++)
            if (roundState.board[i][j] == NONE)
                if (evaluateCell(roundState, i, j, opp) >= WIN_SCORE)
                    return {i, j};
 
    // Bước 3: Tạo 4 quân liên tiếp 2 đầu mở (buộc đối thủ thủ)
    for (int i = 0; i < BOARD_SIZE; i++)
        for (int j = 0; j < BOARD_SIZE; j++)
            if (roundState.board[i][j] == NONE)
                if (evaluateCell(roundState, i, j, botType) >= OPEN4_SCORE)
                    return {i, j};
 
    // Bước 4: Chặn đối thủ tạo 4 quân liên tiếp 2 đầu mở
    for (int i = 0; i < BOARD_SIZE; i++)
        for (int j = 0; j < BOARD_SIZE; j++)
            if (roundState.board[i][j] == NONE)
                if (evaluateCell(roundState, i, j, opp) >= OPEN4_SCORE)
                    return {i, j};
 
    // Bước 5: Greedy — ô có tổng điểm công + thủ cao nhất trong vùng lân cận
    int radius = (difficulty == HARD) ? 3 : 2;
    vector<pair<int,int>> cands = getCandidates(roundState, radius);
 
    if (cands.empty())
        cands = getCandidates(roundState, 4);
 
    long long     maxScore = LLONG_MIN;
    pair<int,int> bestMove = {-1, -1};
 
    for (auto &[r, c] : cands)
    {
        long long score = cellScore(roundState, r, c, botType);
        if (score > maxScore)
        {
            maxScore = score;
            bestMove = {r, c};
        }
    }
 
    if (bestMove.first == -1)
        for (int i = 0; i < BOARD_SIZE; i++)
            for (int j = 0; j < BOARD_SIZE; j++)
                if (roundState.board[i][j] == NONE)
                    return {i, j};
 
    return bestMove;
}
 

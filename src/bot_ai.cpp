#include "bot_ai.h"
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <climits>

using namespace std;


// ============================================================
// Bảng điểm heuristic
// scoreTable[số quân liên tiếp][số đầu mở]
//   openEnds: 0 = bị chặn 2 đầu | 1 = mở 1 đầu | 2 = mở 2 đầu
// ============================================================

static const long long SCORE_TABLE[6][3] =
{
    {0,       0,        0      }, // 0 quân
    {0,      10,       20      }, // 1 quân
    {0,     100,      200      }, // 2 quân
    {0,    1000,     3000      }, // 3 quân
    {0,   80000,   150000      }, // 4 quân — sắp thắng
    {0, 1000000,  1000000      }, // 5 quân — thắng
};

static constexpr long long SCORE_WIN   = 1000000; // Thắng ngay
static constexpr long long SCORE_OPEN4 = 150000;  // 4 quân 2 đầu mở

// Hệ số ưu tiên tấn công so với phòng thủ
static constexpr int ATTACK_WEIGHT = 2;


// ============================================================
// Kiểm tra trạng thái bàn cờ
// ============================================================

static bool isBoardEmpty(const RoundState& rs)
{
    for (int i = 0; i < BOARD_SIZE; i++)
        for (int j = 0; j < BOARD_SIZE; j++)
            if (rs.board[i][j] != NONE)
                return false;
    return true;
}

// Trả về true nếu ô (x, y) có ít nhất một quân trong bán kính `radius`
static bool isNearPiece(const RoundState& rs, int x, int y, int radius)
{
    int rMin = max(0, x - radius), rMax = min(BOARD_SIZE - 1, x + radius);
    int cMin = max(0, y - radius), cMax = min(BOARD_SIZE - 1, y + radius);

    for (int i = rMin; i <= rMax; i++)
        for (int j = cMin; j <= cMax; j++)
            if (rs.board[i][j] != NONE)
                return true;
    return false;
}

// Lấy danh sách các ô trống nằm gần quân đã đặt trong bán kính `radius`
static vector<pair<int,int>> getCandidates(const RoundState& rs, int radius)
{
    vector<pair<int,int>> result;
    for (int i = 0; i < BOARD_SIZE; i++)
        for (int j = 0; j < BOARD_SIZE; j++)
            if (rs.board[i][j] == NONE && isNearPiece(rs, i, j, radius))
                result.push_back({i, j});
    return result;
}


// ============================================================
// Đánh giá điểm
// ============================================================

// Đếm số quân liên tiếp + số đầu mở theo một hướng (dx, dy)
struct LineInfo { int count; int openEnds; };

static LineInfo countLine(const vector<vector<PlayerType>>& board,
                          int x, int y, int dx, int dy, PlayerType player)
{
    int count    = 1;
    int openEnds = 0;

    // Chiều thuận
    int nx = x + dx, ny = y + dy;
    while (nx >= 0 && nx < BOARD_SIZE && ny >= 0 && ny < BOARD_SIZE
           && board[nx][ny] == player)
    { count++; nx += dx; ny += dy; }

    if (nx >= 0 && nx < BOARD_SIZE && ny >= 0 && ny < BOARD_SIZE
        && board[nx][ny] == NONE)
        openEnds++;

    // Chiều ngược
    nx = x - dx; ny = y - dy;
    while (nx >= 0 && nx < BOARD_SIZE && ny >= 0 && ny < BOARD_SIZE
           && board[nx][ny] == player)
    { count++; nx -= dx; ny -= dy; }

    if (nx >= 0 && nx < BOARD_SIZE && ny >= 0 && ny < BOARD_SIZE
        && board[nx][ny] == NONE)
        openEnds++;

    return {count, openEnds};
}

long long evaluateCell(const RoundState& rs, int x, int y, PlayerType player)
{
    // Đặt quân tạm (board là vector nên cast an toàn)
    const_cast<vector<vector<PlayerType>>&>(rs.board)[x][y] = player;

    static const int DX[] = {1, 0, 1,  1};
    static const int DY[] = {0, 1, 1, -1};

    long long score = 0;
    for (int i = 0; i < 4; i++)
    {
        LineInfo li = countLine(rs.board, x, y, DX[i], DY[i], player);
        int cnt     = min(li.count, 5);
        score      += SCORE_TABLE[cnt][li.openEnds];
    }

    // Hoàn tác
    const_cast<vector<vector<PlayerType>>&>(rs.board)[x][y] = NONE;
    return score;
}

// Tổng điểm công + thủ cho một ô
static long long cellScore(const RoundState& rs, int x, int y, PlayerType botType)
{
    PlayerType opp = (botType == X) ? O : X;
    return evaluateCell(rs, x, y, botType) * ATTACK_WEIGHT
         + evaluateCell(rs, x, y, opp);
}


// ============================================================
// Tìm nước đi theo từng ưu tiên
// ============================================================

// Tìm ô trống đầu tiên có evaluateCell >= threshold cho `player`
// Trả về {-1,-1} nếu không tìm thấy
static pair<int,int> findMoveAboveThreshold(const RoundState& rs,
                                             PlayerType player,
                                             long long threshold)
{
    for (int i = 0; i < BOARD_SIZE; i++)
        for (int j = 0; j < BOARD_SIZE; j++)
            if (rs.board[i][j] == NONE
                && evaluateCell(rs, i, j, player) >= threshold)
                return {i, j};
    return {-1, -1};
}

// Greedy: chọn ô trong `candidates` có cellScore cao nhất
static pair<int,int> greedyBest(const RoundState& rs,
                                 const vector<pair<int,int>>& candidates,
                                 PlayerType botType)
{
    long long     maxScore = LLONG_MIN;
    pair<int,int> best     = {-1, -1};

    for (auto& [r, c] : candidates)
    {
        long long score = cellScore(rs, r, c, botType);
        if (score > maxScore)
        {
            maxScore = score;
            best     = {r, c};
        }
    }
    return best;
}

// Fallback: ô trống đầu tiên trên bàn cờ
static pair<int,int> firstEmpty(const RoundState& rs)
{
    for (int i = 0; i < BOARD_SIZE; i++)
        for (int j = 0; j < BOARD_SIZE; j++)
            if (rs.board[i][j] == NONE)
                return {i, j};
    return {-1, -1};
}


// ============================================================
// Hàm chính
// ============================================================

pair<int,int> getBestMove(const RoundState& roundState,
                           PlayerType        botType,
                           BotDifficulty     difficulty)
{
    // Bàn trống → đánh giữa bàn
    if (isBoardEmpty(roundState))
        return {BOARD_SIZE / 2, BOARD_SIZE / 2};

    PlayerType opp = (botType == X) ? O : X;

    // -------------------------------------------------------
    // EASY: đánh ngẫu nhiên vào ô trống gần quân đã có
    // -------------------------------------------------------
    if (difficulty == EASY)
    {
        srand((unsigned)time(nullptr));
        vector<pair<int,int>> cands = getCandidates(roundState, 2);
        if (!cands.empty())
            return cands[rand() % cands.size()];
        return firstEmpty(roundState);
    }

    // -------------------------------------------------------
    // MEDIUM & HARD: Heuristic nhiều bước ưu tiên
    // -------------------------------------------------------

    // Bước 1: Đánh thắng ngay nếu có thể
    auto move = findMoveAboveThreshold(roundState, botType, SCORE_WIN);
    if (move.first != -1) return move;

    // Bước 2: Chặn đối thủ thắng ngay
    move = findMoveAboveThreshold(roundState, opp, SCORE_WIN);
    if (move.first != -1) return move;

    // Bước 3: Tạo 4 quân 2 đầu mở (buộc đối thủ phải thủ)
    move = findMoveAboveThreshold(roundState, botType, SCORE_OPEN4);
    if (move.first != -1) return move;

    // Bước 4: Chặn đối thủ tạo 4 quân 2 đầu mở
    move = findMoveAboveThreshold(roundState, opp, SCORE_OPEN4);
    if (move.first != -1) return move;

    // Bước 5: Greedy trong vùng lân cận
    int radius = (difficulty == HARD) ? 3 : 2;
    vector<pair<int,int>> cands = getCandidates(roundState, radius);

    if (cands.empty())
        cands = getCandidates(roundState, 4);

    move = greedyBest(roundState, cands, botType);
    if (move.first != -1) return move;

    return firstEmpty(roundState);
}

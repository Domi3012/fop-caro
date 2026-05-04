#pragma once
#include "model.h"
#include <utility>

// Ba mức độ khó của Bot AI
enum BotDifficulty
{
    EASY,   // Ngẫu nhiên trong vùng lân cận quân đã đặt
    MEDIUM, // Heuristic cơ bản: biết chặn và tạo lợi thế
    HARD    // Heuristic toàn diện với tìm kiếm vùng lân cận rộng hơn
};

// Đánh giá điểm của một ô nếu đặt quân `player` vào đó (không thay đổi board)
long long evaluateCell(const RoundState& rs, int x, int y, PlayerType player);

// Trả về tọa độ nước đi tốt nhất cho bot (không thay đổi board)
std::pair<int,int> getBestMove(const RoundState& roundState,
                               PlayerType        botType,
                               BotDifficulty     difficulty);

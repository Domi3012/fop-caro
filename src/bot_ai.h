#pragma once
#include "model.h"
#include <utility> // Để dùng std::pair

// Định nghĩa 3 mức độ khó cho Bot AI
enum BotDifficulty {
    EASY,   // Dễ: Thường đánh ngẫu nhiên hoặc chỉ biết đánh nối tiếp quân của mình.
    MEDIUM, // Trung bình: Heuristic cơ bản, biết chặn khi đối thủ có 3-4 quân và biết tự tạo lợi thế.
    HARD    // Khó: Heuristic toàn diện (chấm điểm công/thủ phức tạp) hoặc dùng Minimax Alpha-Beta độ sâu thấp.
};

// Hàm tính toán và trả về tọa độ (X, Y) tốt nhất cho Bot
// Hàm này KHÔNG làm thay đổi bàn cờ, nó chỉ đọc dữ liệu và "gợi ý" nước đi
std::pair<int, int> getBestMove(const RoundState& roundState, PlayerType botType, BotDifficulty difficulty);
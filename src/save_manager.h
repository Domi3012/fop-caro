#pragma once

#include "model.h"
#include <string>
#include <vector>

// ========================================================
// Save/Load API
// ========================================================

// Lưu trạng thái hiện tại của game xuống file (VD: saves/save_20260415.txt)
// Trả về true nếu lưu thành công, false nếu lỗi (hết ổ cứng, sai quyền...)
bool saveGame(const MatchState &match, const std::string &filename);

// Đọc file và nạp dữ liệu đè lên matchState hiện tại
// Trả về true nếu đọc thành công, false nếu lỗi
bool loadGame(MatchState &match, const std::string &filename);

// ========================================================
// Directory Management API
// ========================================================

// Quét thư mục "saves/" và trả về danh sách tên các file save để View vẽ lên
// màn hình Các file được sắp xếp theo chiều giảm dần
std::vector<std::string> getSaveFilesList();
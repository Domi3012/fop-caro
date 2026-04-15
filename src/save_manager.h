pragma once

#include "model.h"
#include <string>
#include <vector>

// Lưu trạng thái hiện tại của game xuống file (VD: saves/save_20260415.txt)
// Trả về true nếu lưu thành công, false nếu lỗi (hết ổ cứng, sai quyền...)
bool saveGame(const MatchState& match, const std::string& filename);

// Đọc file và nạp dữ liệu đè lên matchState hiện tại
bool loadGame(MatchState& match, const std::string& filename);

// Quét thư mục "saves/" và trả về danh sách tên các file save để View vẽ lên màn hình
std::vector<std::string> getSaveFilesList();
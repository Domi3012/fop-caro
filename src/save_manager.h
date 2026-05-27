#pragma once

#include "model.h"
#include <string>
#include <vector>

// ========================================================
// Save/Load API
// ========================================================

// Lưu trạng thái hiện tại của game xuống file (VD: saves/MySave_20260415_120000.txt)
// moveHistory: lịch sử đầy đủ các nước đi với trạng thái undo
// undoStack: stack các nước có thể undo
// redoStack: stack các nước có thể redo
// Trả về true nếu lưu thành công, false nếu lỗi (hết ổ cứng, sai quyền...)
bool saveGame(const MatchState &match,
              const std::vector<MoveRecord> &moveHistory,
              const std::vector<MoveRecord> &undoStack,
              const std::vector<MoveRecord> &redoStack,
              const std::string &filename);

// Đọc file và nạp dữ liệu đè lên matchState hiện tại
// moveHistory, undoStack, redoStack (output): các stack được nạp từ file
// Trả về true nếu đọc thành công, false nếu lỗi
bool loadGame(MatchState &match,
              std::vector<MoveRecord> &moveHistory,
              std::vector<MoveRecord> &undoStack,
              std::vector<MoveRecord> &redoStack,
              const std::string &filename);

// ========================================================
// Directory Management API
// ========================================================

// Quét thư mục "saves/" và trả về danh sách tên các file save để View vẽ lên
// màn hình Các file được sắp xếp theo chiều giảm dần
std::vector<std::string> getSaveFilesList();

// Xoá file save trong thư mục "saves/"
// Trả về true nếu xoá thành công, false nếu lỗi (file không tồn tại, sai quyền...)
bool deleteSaveFile(const std::string &filename);
#include "save_manager.h"
#include "model.h"
#include <fstream>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

// Lưu trạng thái hiện tại của game xuống file
bool saveGame(const MatchState& match, const std::string& filename) {
    // Đảm bảo thư mục saves tồn tại
    if (!fs::exists("saves")) {
        fs::create_directory("saves");
    }

    std::ofstream out("saves/" + filename);
    if (!out.is_open()) return false;

    std::string nameX = match.playerX.Name.empty() ? "Player 1" : match.playerX.Name;
    std::string nameO = match.playerO.Name.empty() ? "Player 2" : match.playerO.Name;

    // Lưu thông tin Player X
    out << nameX << "\n" 
        << (int)match.playerX.character << " " 
        << match.playerX.health << "\n";

    // Lưu thông tin Player O
    out << nameO << "\n" 
        << (int)match.playerO.character << " " 
        << match.playerO.health << "\n";

    // Lưu trạng thái Round
    out << (int)match.currentRound.toMove << " " 
        << match.currentRound.turnCount << " " 
        << (int)match.currentRound.result << "\n";

    // Lưu bàn cờ (Board)
    for (int i = 0; i < BOARD_SIZE; ++i) {
        for (int j = 0; j < BOARD_SIZE; ++j) {
            out << (int)match.currentRound.board[i][j] << " ";
        }
        out << "\n";
    }

    // Lưu Match State
    out << match.countRoundsPlayed << " " << (int)match.matchResult << "\n";

    out.close();
    return true;
}

// Đọc file và nạp dữ liệu
bool loadGame(MatchState& match, const std::string& filename) {
    std::ifstream in("saves/" + filename);
    if (!in.is_open()) return false;

    int charType, toMove, roundResult, matchResult;

    // Đọc Player X
    std::getline(in >> std::ws, match.playerX.Name);
    in >> charType >> match.playerX.health;
    match.playerX.character = static_cast<CharacterType>(charType);

    // Đọc Player O
    std::getline(in >> std::ws, match.playerO.Name);
    in >> charType >> match.playerO.health;
    match.playerO.character = static_cast<CharacterType>(charType);

    // Đọc Round
    in >> toMove >> match.currentRound.turnCount >> roundResult;
    match.currentRound.toMove = static_cast<PlayerType>(toMove);
    match.currentRound.result = static_cast<RoundResult>(roundResult);

    // Đọc bàn cờ
    match.currentRound.board.assign(BOARD_SIZE, std::vector<PlayerType>(BOARD_SIZE, NONE));
    for (int i = 0; i < BOARD_SIZE; ++i) {
        for (int j = 0; j < BOARD_SIZE; ++j) {
            int cellVal;
            in >> cellVal;
            match.currentRound.board[i][j] = static_cast<PlayerType>(cellVal);
        }
    }

    // Đọc Match State
    in >> match.countRoundsPlayed >> matchResult;
    match.matchResult = static_cast<RoundResult>(matchResult);

    in.close();
    return true;
}

// Quét thư mục saves/
std::vector<std::string> getSaveFilesList() {
    std::vector<std::string> files;
    if (!fs::exists("saves")) return files;

    for (const auto& entry : fs::directory_iterator("saves")) {
        if (entry.is_regular_file()) {
            files.push_back(entry.path().filename().string());
        }
    }
    return files;
}
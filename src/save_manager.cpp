#include "save_manager.h"
#include "model.h"
#include <fstream>
#include <filesystem>
#include <iostream>
#include <algorithm>

namespace fs = std::filesystem;

namespace
{
    // === Helper functions for Saving ===

    void savePlayer(std::ofstream &out, const Player &player, const std::string &defaultName)
    {
        std::string name = player.name.empty() ? defaultName : player.name;
        out << name << "\n"
            << static_cast<int>(player.character) << " "
            << player.health << "\n";
    }

    void saveRound(std::ofstream &out, const RoundState &round)
    {
        out << static_cast<int>(round.toMove) << " "
            << round.turnCount << " "
            << static_cast<int>(round.result) << "\n";

        for (int i = 0; i < BOARD_SIZE; ++i)
        {
            for (int j = 0; j < BOARD_SIZE; ++j)
            {
                out << static_cast<int>(round.board[i][j]) << " ";
            }
            out << "\n";
        }
    }

    // === Helper functions for Loading ===

    void loadPlayer(std::ifstream &in, Player &player)
    {
        int charType;
        std::getline(in >> std::ws, player.name);
        in >> charType >> player.health;
        player.character = static_cast<CharacterType>(charType);
    }

    void loadRound(std::ifstream &in, RoundState &round)
    {
        int toMove, roundResult;
        in >> toMove >> round.turnCount >> roundResult;
        round.toMove = static_cast<PlayerType>(toMove);
        round.result = static_cast<RoundResult>(roundResult);

        round.board.assign(BOARD_SIZE, std::vector<PlayerType>(BOARD_SIZE, NONE));
        for (int i = 0; i < BOARD_SIZE; ++i)
        {
            for (int j = 0; j < BOARD_SIZE; ++j)
            {
                int cellVal;
                in >> cellVal;
                round.board[i][j] = static_cast<PlayerType>(cellVal);
            }
        }
    }

    void ensureSaveDirectoryExists()
    {
        if (!fs::exists("saves"))
        {
            fs::create_directory("saves");
        }
    }
}

// Lưu trạng thái hiện tại của game xuống file
bool saveGame(const MatchState &match, const std::vector<Move> &moveHistory, const std::string &filename)
{
    ensureSaveDirectoryExists();

    std::ofstream out("saves/" + filename);
    if (!out.is_open())
        return false;

    savePlayer(out, match.playerX, "Player 1");
    savePlayer(out, match.playerO, "Player 2");

    saveRound(out, match.currentRound);

    out << match.countRoundsPlayed << " " << static_cast<int>(match.matchResult) << "\n";

    // Lưu lịch sử nước đi
    out << moveHistory.size() << "\n";
    for (const auto &m : moveHistory)
    {
        out << m.x << " " << m.y << " " << static_cast<int>(m.type) << "\n";
    }

    out.close();
    return true;
}

// Đọc file và nạp dữ liệu
bool loadGame(MatchState &match, std::vector<Move> &moveHistory, const std::string &filename)
{
    std::ifstream in("saves/" + filename);
    if (!in.is_open())
        return false;

    loadPlayer(in, match.playerX);
    loadPlayer(in, match.playerO);

    loadRound(in, match.currentRound);

    int matchResult;
    in >> match.countRoundsPlayed >> matchResult;
    match.matchResult = static_cast<RoundResult>(matchResult);

    // Đọc lịch sử nước đi
    moveHistory.clear();
    int moveCount;
    in >> moveCount;
    for (int i = 0; i < moveCount; i++)
    {
        Move m;
        int typeInt;
        in >> m.x >> m.y >> typeInt;
        m.type = static_cast<PlayerType>(typeInt);
        moveHistory.push_back(m);
    }

    in.close();
    return true;
}

// Quét thư mục saves/
std::vector<std::string> getSaveFilesList()
{
    std::vector<std::string> files;

    if (!fs::exists("saves"))
    {
        return files;
    }

    for (const auto &entry : fs::directory_iterator("saves"))
    {
        if (entry.is_regular_file())
        {
            files.push_back(entry.path().filename().string());
        }
    }

    std::sort(files.begin(), files.end(), std::greater<std::string>());
    return files;
}

// Xoá file save
bool deleteSaveFile(const std::string &filename)
{
    std::string path = "saves/" + filename;
    if (!fs::exists(path))
        return false;

    std::error_code ec;
    fs::remove(path, ec);
    return !ec; // true nếu không có lỗi
}
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
            << player.health << " "
            << player.maxHealth << " "
            << player.baseDamage << " "
            << player.sorcererStacks << "\n";
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
        in >> charType >> player.health >> player.maxHealth >> player.baseDamage >> player.sorcererStacks;
        player.character = static_cast<CharacterType>(charType);
    }

    // Load player từ format cũ (chỉ có charType + health)
    void loadPlayerLegacy(std::ifstream &in, Player &player)
    {
        int charType;
        std::getline(in >> std::ws, player.name);
        in >> charType >> player.health;
        player.character = static_cast<CharacterType>(charType);
        // Khôi phục maxHealth và baseDamage từ character type
        player.maxHealth = getBaseHealth(player.character);
        player.baseDamage = getBaseDamage(player.character);
        player.sorcererStacks = 0;
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
bool saveGame(const MatchState &match,
              const std::vector<MoveRecord> &moveHistory,
              const std::vector<MoveRecord> &undoStack,
              const std::vector<MoveRecord> &redoStack,
              const std::string &filename)
{
    ensureSaveDirectoryExists();

    std::ofstream out("saves/" + filename);
    if (!out.is_open())
        return false;

    // NAMES section
    out << "NAMES\n";
    out << (match.playerX.name.empty() ? "Player X" : match.playerX.name) << "\n";
    out << (match.playerO.name.empty() ? "Player O" : match.playerO.name) << "\n";

    savePlayer(out, match.playerX, "Player 1");
    savePlayer(out, match.playerO, "Player 2");

    saveRound(out, match.currentRound);

    out << match.countRoundsPlayed << " " << static_cast<int>(match.matchResult) << "\n";

    // MOVE_HISTORY section
    out << "MOVE_HISTORY\n";
    out << moveHistory.size() << "\n";
    for (const auto &m : moveHistory)
    {
        out << m.row << " " << m.col << " "
            << static_cast<int>(m.player) << " "
            << (m.isUndone ? 1 : 0) << "\n";
    }

    // UNDO_STACK section
    out << "UNDO_STACK\n";
    out << undoStack.size() << "\n";
    for (const auto &m : undoStack)
    {
        out << m.row << " " << m.col << " "
            << static_cast<int>(m.player) << " "
            << (m.isUndone ? 1 : 0) << "\n";
    }

    // REDO_STACK section
    out << "REDO_STACK\n";
    out << redoStack.size() << "\n";
    for (const auto &m : redoStack)
    {
        out << m.row << " " << m.col << " "
            << static_cast<int>(m.player) << " "
            << (m.isUndone ? 1 : 0) << "\n";
    }

    out.close();
    return true;
}

// Đọc file và nạp dữ liệu
bool loadGame(MatchState &match,
              std::vector<MoveRecord> &moveHistory,
              std::vector<MoveRecord> &undoStack,
              std::vector<MoveRecord> &redoStack,
              const std::string &filename)
{
    std::ifstream in("saves/" + filename);
    if (!in.is_open())
        return false;

    moveHistory.clear();
    undoStack.clear();
    redoStack.clear();

    // Đọc token đầu tiên để xác định format
    std::string firstToken;
    in >> firstToken;

    if (firstToken == "NAMES")
    {
        // === FORMAT MỚI (có section markers) ===
        std::getline(in >> std::ws, match.playerX.name);
        std::getline(in >> std::ws, match.playerO.name);

        loadPlayer(in, match.playerX);
        loadPlayer(in, match.playerO);
        loadRound(in, match.currentRound);

        int matchResult;
        in >> match.countRoundsPlayed >> matchResult;
        match.matchResult = static_cast<RoundResult>(matchResult);

        std::string section;
        while (in >> section)
        {
            if (section == "MOVE_HISTORY" || section == "UNDO_STACK" || section == "REDO_STACK")
            {
                auto &target = (section == "MOVE_HISTORY") ? moveHistory
                               : (section == "UNDO_STACK") ? undoStack
                                                           : redoStack;
                int count;
                in >> count;
                for (int i = 0; i < count; i++)
                {
                    MoveRecord m;
                    int typeInt, undoneInt;
                    in >> m.row >> m.col >> typeInt >> undoneInt;
                    m.player = static_cast<PlayerType>(typeInt);
                    m.isUndone = (undoneInt != 0);
                    target.push_back(m);
                }
            }
        }
    }
    else
    {
        // === FORMAT CŨ (không có NAMES section) ===
        // firstToken là tên player X
        match.playerX.name = firstToken;
        int charType;
        in >> charType >> match.playerX.health;
        match.playerX.character = static_cast<CharacterType>(charType);
        match.playerX.maxHealth = getBaseHealth(match.playerX.character);
        match.playerX.baseDamage = getBaseDamage(match.playerX.character);
        match.playerX.sorcererStacks = 0;

        loadPlayerLegacy(in, match.playerO);

        loadRound(in, match.currentRound);

        int matchResult;
        in >> match.countRoundsPlayed >> matchResult;
        match.matchResult = static_cast<RoundResult>(matchResult);

        // Đọc move history cũ (Move struct: x, y, type)
        int moveCount;
        if (in >> moveCount)
        {
            for (int i = 0; i < moveCount; i++)
            {
                MoveRecord m;
                int typeInt;
                in >> m.row >> m.col >> typeInt;
                m.player = static_cast<PlayerType>(typeInt);
                m.isUndone = false;
                moveHistory.push_back(m);
                undoStack.push_back(m);
            }
        }
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
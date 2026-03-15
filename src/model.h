#pragma once

#include <string>
#include <vector>

using std::string;
using std::vector;


// I. Các hằng số
// 1. Hằng số chung
int const MAX_HEALTH = 100; // máu mặc định cho mọi nhân vật
int const BOARD_SIZE = 12; // 12 x 12 ô cờ caro


// II. Các enum:

enum PlayerType {
	X, O, NONE
};

enum CharacterType {
	ASSASSIN, BRUISER, VAMPIRE
};

enum RoundResult{
	ONGOING, DRAW, X_WINS, O_WINS
};


// III. Các struct mô hình dữ liệu trò chơi:

// Mô hình 1 người chơi (mỗi ván có 2 người)
struct Player {
	string Name;
	CharacterType character; // Loại nhân vật
	int health = MAX_HEALTH; // Mặc định là 100
};

// Mô hình một vòng đấu (một game cờ caro)
struct RoundState {
	PlayerType toMove;
	int turnCount; // Số lượt đã chơi
	vector<vector<PlayerType>> board; // Bàn cờ
	RoundResult result; // Kết quả của vòng đấu
};

// Mô hình một ván đấu (gồm nhiều game cờ caro)
struct MatchState {
	Player playerX;
	Player playerO;
	RoundState currentRound;
	int countRoundsPlayed; // Số vòng đã chơi
	RoundResult matchResult; // Kết quả của ván đấu
};


// IV. Các hàm cần viết (to do của nhóm Model):
// Các hàm khởi tạo
void initMatch(MatchState& matchState, const Player& playerX, const Player& playerY); // Gọi initRound và set giá trị
void initRound(RoundState& roundState, int roundCount); // Vòng lẻ X đi trước, vòng chẵn O đi trước

// Các hàm xử lý logic trò chơi
bool checkValidMove(const RoundState& roundState, int x, int y); // Kiểm tra nước đi hợp lệ, cần viết để bổ trợ mấy lớp khác
void makeMove(RoundState& roundState, int x, int y); // Thực hiện nước đi, cập nhật board và turnCount
RoundResult checkRoundResult(const RoundState& roundState, int lastMoveX, int lastMoveY); // Kiểm tra kết quả vòng đấu sau mỗi nước đi
void executeAttack(Player& attacker, Player& defender, int turnCount); // Thực hiện tính toán damage raw và áp dụng các hiệu ứng dựa trên loại nhân vật. Nhớ xử lí các trường hợp biên < 0 hay > 100
RoundResult checkMatchResult(const MatchState& matchState); // Kiểm tra kết quả ván đấu sau mỗi vòng đấu (sau mỗi lần attack)

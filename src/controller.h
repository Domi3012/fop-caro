#pragma once

#include "model.h"
#include "raylib.h"

enum GameScreen {
	MAIN_MENU,
	MODE_SELECTION,       // Chon PVP hay PVE
	CHARACTER_SELECTION,
	GAME_INTRO,           // Animation nhap canh truoc khi vao game board
	GAME_BOARD,
	ROUND_OVER,
	GAME_OVER,
	LOAD_GAME,
	SETTINGS
};

struct UIState {
	bool shouldExit = false;

	GameScreen currentScreen;

	// Main menu
	int mainMenuIndex;

	// Mode selection (PVP / PVE)
	int modeMenuIndex;  // 0 = PVP, 1 = PVE
	bool isPVE;         // true = choi vs bot

	// Character selection
	bool isSelectingX;      // true = dang chon cho X
	int characterMenuIndex; // 1=ASSASSIN 2=BRUISER 3=VAMPIRE

	// Game board
	int cursorX;
	int cursorY;

	// Load menu
	int loadMenuIndex;

	// Settings menu
	int settingsMenuIndex;

	// Round over timer
	float roundOverTimer = 0.0f;

	// Intro animation: camera offset bat dau lon, giam dan ve 0
	float introCamX = 0.0f;
};

// === HANDLERS ===
void handleMainMenuInput(UIState& ui);
void handleModeSelectionInput(UIState& ui);
void handleCharSelectionInput(MatchState& match, UIState& ui);
void handleGameIntroInput(MatchState& match, UIState& ui);
void handleGameplayInput(MatchState& match, UIState& ui);
void handleRoundOverInput(MatchState& match, UIState& ui);
void handleGameOverInput(MatchState& match, UIState& ui);

void handleInput(MatchState& match, UIState& ui);

void handleLoadGameInput(MatchState& match, UIState& ui, const std::vector<std::string>& saveFiles);
void handleSettingsInput(UIState& ui);

// Chuyen sang GAME_BOARD (dung sau initMatch hoac loadGame)
void startMatch(UIState& ui);

// Bat dau intro animation (goi sau khi initMatch xong)
void startGameIntro(UIState& ui);

void applyCameraAndScaling();

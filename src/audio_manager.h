#pragma once

// ========================================================
// Audio Types
// ========================================================

// Định nghĩa các loại âm thanh hiệu ứng (SFX)
enum SoundEffect {
    SFX_CLICK,      // Bấm menu
    SFX_PLACE,      // Đặt cờ
    SFX_ATTACK,     // Tấn công
    SFX_HEAL,       // Hồi máu (Vampire)
    SFX_WIN,        // Thắng round
    SFX_GAME_OVER   // Kết thúc game
};

// Định nghĩa các loại nhạc nền (BGM)
enum MusicTrack {
    BGM_MENU,
    BGM_BATTLE
};

// ========================================================
// Lifecycle API
// ========================================================

void initAudio();       // Gọi 1 lần ở main.cpp (InitAudioDevice & Load files)
void unloadAudio();     // Gọi 1 lần ở cuối main.cpp
void updateAudioStream(); // BẮT BUỘC gọi liên tục trong vòng lặp while để nhạc không bị vấp

// ========================================================
// Playback API
// ========================================================

void playSFX(SoundEffect sfx);
void playMusic(MusicTrack track);
void stopMusic();

// ========================================================
// Settings API (Dùng cho màn hình Settings)
// ========================================================

void setMusicVolume(float volume); // 0.0f đến 1.0f
void setSFXVolume(float volume);
void toggleMusicEnabled();
void toggleSFXEnabled();

// ========================================================
// State Query API
// ========================================================

bool isMusicEnabled();
bool isSFXEnabled();
float getMusicVolume();
float getSFXVolume();
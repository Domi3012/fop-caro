#pragma once

// Định nghĩa các loại âm thanh để code gọn hơn
enum SoundEffect {
    SFX_CLICK,      // Bấm menu
    SFX_PLACE,      // Đặt cờ
    SFX_ATTACK,     // Tấn công
    SFX_HEAL,       // Hồi máu (Vampire)
    SFX_WIN,        // Thắng round
    SFX_GAME_OVER   // Kết thúc game
};

enum MusicTrack {
    BGM_MENU,
    BGM_BATTLE
};

// API cho vòng đời âm thanh
void initAudio();       // Gọi 1 lần ở main.cpp (InitAudioDevice & Load files)
void unloadAudio();     // Gọi 1 lần ở cuối main.cpp
void updateAudioStream(); // BẮT BUỘC gọi liên tục trong vòng lặp while để nhạc không bị vấp

// API phát âm thanh
void playSFX(SoundEffect sfx);
void playMusic(MusicTrack track);
void stopMusic();

// API cài đặt (Dùng cho màn hình Settings)
void setMusicVolume(float volume); // 0.0f đến 1.0f
void setSFXVolume(float volume);
void toggleMusicEnabled();
void toggleSFXEnabled();
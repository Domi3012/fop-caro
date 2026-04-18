#include "audio_manager.h"
#include "raylib.h"

#include <algorithm>

namespace {
    constexpr int SOUND_COUNT = static_cast<int>(SFX_GAME_OVER) + 1;
    constexpr int MUSIC_COUNT = static_cast<int>(BGM_BATTLE) + 1;

    Sound g_sfx[SOUND_COUNT]{};
    Music g_music[MUSIC_COUNT]{};

    bool g_sfxLoaded[SOUND_COUNT]{};
    bool g_musicLoaded[MUSIC_COUNT]{};

    bool g_audioInitialized = false;
    bool g_musicEnabled = true;
    bool g_sfxEnabled = true;

    float g_musicVolume = 0.6f;
    float g_sfxVolume = 0.8f;

    int g_currentMusic = -1;

    float clamp01(float value) {
        return std::max(0.0f, std::min(1.0f, value));
    }

    const char* getSFXPath(SoundEffect sfx) {
        switch (sfx) {
            case SFX_CLICK:     return "assets/sounds/sfx_click.wav";
            case SFX_PLACE:     return "assets/sounds/sfx_place.wav";
            case SFX_ATTACK:    return "assets/sounds/sfx_attack.wav";
            case SFX_HEAL:      return "assets/sounds/sfx_heal.wav";
            case SFX_WIN:       return "assets/sounds/sfx_win.wav";
            case SFX_GAME_OVER: return "assets/sounds/sfx_game_over.wav";
            default:            return nullptr;
        }
    }

    const char* getMusicPath(MusicTrack track) {
        switch (track) {
            case BGM_MENU:   return "assets/sounds/bgm_menu.ogg";
            case BGM_BATTLE: return "assets/sounds/bgm_battle.ogg";
            default:         return nullptr;
        }
    }

    bool isValidSFX(SoundEffect sfx) {
        const int index = static_cast<int>(sfx);
        return index >= 0 && index < SOUND_COUNT;
    }

    bool isValidTrack(MusicTrack track) {
        const int index = static_cast<int>(track);
        return index >= 0 && index < MUSIC_COUNT;
    }

    void applySFXVolume() {
        const float appliedVolume = g_sfxEnabled ? g_sfxVolume : 0.0f;

        for (int i = 0; i < SOUND_COUNT; ++i) {
            if (g_sfxLoaded[i]) {
                SetSoundVolume(g_sfx[i], appliedVolume);
            }
        }
    }

    void applyMusicVolume() {
        const float appliedVolume = g_musicEnabled ? g_musicVolume : 0.0f;

        for (int i = 0; i < MUSIC_COUNT; ++i) {
            if (g_musicLoaded[i]) {
                SetMusicVolume(g_music[i], appliedVolume);
            }
        }
    }
}

void initAudio() {
    if (g_audioInitialized) {
        return;
    }

    if (!IsAudioDeviceReady()) {
        InitAudioDevice();
    }

    for (int i = 0; i < SOUND_COUNT; ++i) {
        const SoundEffect sfx = static_cast<SoundEffect>(i);
        const char* path = getSFXPath(sfx);

        if (path == nullptr || !FileExists(path)) {
            TraceLog(LOG_WARNING, "[Audio] Missing SFX file: %s", path ? path : "(null)");
            continue;
        }

        g_sfx[i] = LoadSound(path);
        g_sfxLoaded[i] = true;
    }

    for (int i = 0; i < MUSIC_COUNT; ++i) {
        const MusicTrack track = static_cast<MusicTrack>(i);
        const char* path = getMusicPath(track);

        if (path == nullptr || !FileExists(path)) {
            TraceLog(LOG_WARNING, "[Audio] Missing music file: %s", path ? path : "(null)");
            continue;
        }

        g_music[i] = LoadMusicStream(path);
        g_musicLoaded[i] = true;
    }

    applySFXVolume();
    applyMusicVolume();

    g_audioInitialized = true;
}

void unloadAudio() {
    if (!g_audioInitialized) {
        return;
    }

    stopMusic();

    for (int i = 0; i < SOUND_COUNT; ++i) {
        if (g_sfxLoaded[i]) {
            UnloadSound(g_sfx[i]);
            g_sfxLoaded[i] = false;
        }
    }

    for (int i = 0; i < MUSIC_COUNT; ++i) {
        if (g_musicLoaded[i]) {
            UnloadMusicStream(g_music[i]);
            g_musicLoaded[i] = false;
        }
    }

    g_currentMusic = -1;
    g_audioInitialized = false;

    if (IsAudioDeviceReady()) {
        CloseAudioDevice();
    }
}

void updateAudioStream() {
    if (!g_audioInitialized || !g_musicEnabled || g_currentMusic < 0) {
        return;
    }

    if (!g_musicLoaded[g_currentMusic]) {
        return;
    }

    UpdateMusicStream(g_music[g_currentMusic]);
}

void playSFX(SoundEffect sfx) {
    if (!g_audioInitialized || !g_sfxEnabled || !isValidSFX(sfx)) {
        return;
    }

    const int index = static_cast<int>(sfx);
    if (!g_sfxLoaded[index]) {
        return;
    }

    SetSoundVolume(g_sfx[index], g_sfxVolume);
    PlaySound(g_sfx[index]);
}

void playMusic(MusicTrack track) {
    if (!g_audioInitialized || !isValidTrack(track)) {
        return;
    }

    const int index = static_cast<int>(track);
    if (!g_musicLoaded[index]) {
        return;
    }

    if (g_currentMusic == index) {
        if (g_musicEnabled) {
            SetMusicVolume(g_music[index], g_musicVolume);
            if (!IsMusicStreamPlaying(g_music[index])) {
                PlayMusicStream(g_music[index]);
            }
        }
        return;
    }

    if (g_currentMusic >= 0 && g_musicLoaded[g_currentMusic]) {
        StopMusicStream(g_music[g_currentMusic]);
    }

    g_currentMusic = index;
    SetMusicVolume(g_music[index], g_musicEnabled ? g_musicVolume : 0.0f);

    if (g_musicEnabled) {
        PlayMusicStream(g_music[index]);
    }
}

void stopMusic() {
    if (!g_audioInitialized || g_currentMusic < 0) {
        return;
    }

    if (g_musicLoaded[g_currentMusic]) {
        StopMusicStream(g_music[g_currentMusic]);
    }

    g_currentMusic = -1;
}

void setMusicVolume(float volume) {
    g_musicVolume = clamp01(volume);

    if (!g_audioInitialized) {
        return;
    }

    applyMusicVolume();
}

void setSFXVolume(float volume) {
    g_sfxVolume = clamp01(volume);

    if (!g_audioInitialized) {
        return;
    }

    applySFXVolume();
}

void toggleMusicEnabled() {
    g_musicEnabled = !g_musicEnabled;

    if (!g_audioInitialized) {
        return;
    }

    applyMusicVolume();

    if (g_currentMusic < 0 || !g_musicLoaded[g_currentMusic]) {
        return;
    }

    if (g_musicEnabled) {
        if (!IsMusicStreamPlaying(g_music[g_currentMusic])) {
            PlayMusicStream(g_music[g_currentMusic]);
        }
    } else {
        PauseMusicStream(g_music[g_currentMusic]);
    }
}

void toggleSFXEnabled() {
    g_sfxEnabled = !g_sfxEnabled;

    if (!g_audioInitialized) {
        return;
    }

    applySFXVolume();
}
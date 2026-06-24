#include "sprite_manager.h"
#include "raylib.h"
#include <cstring>

// ============================================================
// Dữ liệu nội bộ
// ============================================================

// Sprite cho 4 nhân vật: ASSASSIN(0), BRUISER(1), VAMPIRE(2), SORCERER(3)
static CharacterSprite characterSprites[4];

// Bảng ánh xạ CharacterType -> tên file sprite (không có đuôi _idle/_attack)
// Lưu ý: ASSASSIN dùng file "imposter"
static const char *SPRITE_BASENAMES[] = {
    "imposter",   // ASSASSIN  = 0
    "bruiser",    // BRUISER   = 1
    "vampire",    // VAMPIRE   = 2
    "sorcerer"    // SORCERER  = 3
};

// Hậu tố cho từng loại animation
static const char *ANIM_SUFFIXES[] = {
    "_idle",      // ANIM_IDLE   = 0
    "_attack"     // ANIM_ATTACK = 1
};

// Số frame mặc định cho từng loại animation
static const int ANIM_FRAME_COUNTS[] = {
    4,  // ANIM_IDLE:   4 frame
    4   // ANIM_ATTACK: 4 frame (dự kiến)
};

// Animation có lặp không?
static const bool ANIM_LOOPS[] = {
    true,   // ANIM_IDLE:   lặp vô hạn
    false   // ANIM_ATTACK: chạy 1 lần
};

// ============================================================
// Helper: load một SpriteAnimation từ file
// ============================================================

static SpriteAnimation loadSpriteAnimation(const char *baseName,
                                            AnimationType animType)
{
    SpriteAnimation anim;
    anim.currentFrame = 0;
    anim.frameTime = DEFAULT_FRAME_TIME;
    anim.timer = 0.0f;
    anim.frameCount = ANIM_FRAME_COUNTS[animType];
    anim.loop = ANIM_LOOPS[animType];
    anim.finished = false;
    anim.loaded = false;

    // Xây dựng đường dẫn: ./assets/images/sprite/<baseName><suffix>.png
    char path[256];
    snprintf(path, sizeof(path), "./assets/images/sprite/%s%s.png",
             baseName, ANIM_SUFFIXES[animType]);

    // Kiểm tra file tồn tại trước khi load (tránh crash khi chưa có attack sprite)
    if (FileExists(path))
    {
        anim.sheet = LoadTexture(path);
        SetTextureFilter(anim.sheet, TEXTURE_FILTER_POINT); // Pixel art → nearest-neighbor
        anim.loaded = true;

        // Tự tính frameCount từ kích thước thực tế của sheet
        // (phòng trường hợp sheet có số frame khác mặc định)
        if (anim.sheet.width > 0 && SPRITE_FRAME_SIZE > 0)
        {
            anim.frameCount = anim.sheet.width / SPRITE_FRAME_SIZE;
        }
    }

    return anim;
}

// ============================================================
// API: Khởi tạo & giải phóng
// ============================================================

void initSpriteManager()
{
    for (int charIdx = 0; charIdx < 4; charIdx++)
    {
        CharacterSprite &cs = characterSprites[charIdx];
        cs.currentAnim = ANIM_IDLE;

        for (int animIdx = 0; animIdx < ANIM_COUNT; animIdx++)
        {
            cs.anims[animIdx] = loadSpriteAnimation(
                SPRITE_BASENAMES[charIdx],
                static_cast<AnimationType>(animIdx));
        }
    }
}

void unloadSpriteManager()
{
    for (int charIdx = 0; charIdx < 4; charIdx++)
    {
        for (int animIdx = 0; animIdx < ANIM_COUNT; animIdx++)
        {
            if (characterSprites[charIdx].anims[animIdx].loaded)
            {
                UnloadTexture(characterSprites[charIdx].anims[animIdx].sheet);
                characterSprites[charIdx].anims[animIdx].loaded = false;
            }
        }
    }
}

CharacterSprite& getCharacterSprite(CharacterType type)
{
    // CharacterType enum: ASSASSIN=0, BRUISER=1, VAMPIRE=2, SORCERER=3
    int idx = static_cast<int>(type);
    if (idx < 0 || idx >= 4) idx = 0; // fallback
    return characterSprites[idx];
}

CharacterSprite getCharacterSpriteCopy(CharacterType type)
{
    // Trả về bản sao (value copy) — texture handle được chia sẻ (không sao vì
    // chỉ unload qua unloadSpriteManager), nhưng timer/frame/finished độc lập.
    int idx = static_cast<int>(type);
    if (idx < 0 || idx >= 4) idx = 0;
    CharacterSprite copy = characterSprites[idx];
    // Reset animation state cho bản sao
    for (int i = 0; i < ANIM_COUNT; i++)
    {
        copy.anims[i].currentFrame = 0;
        copy.anims[i].timer = 0.0f;
        copy.anims[i].finished = false;
    }
    copy.currentAnim = ANIM_IDLE;
    return copy;
}

// ============================================================
// API: Điều khiển animation
// ============================================================

void updateAnimation(SpriteAnimation &anim, float dt)
{
    if (!anim.loaded || anim.finished)
        return;

    anim.timer += dt;

    if (anim.timer >= anim.frameTime)
    {
        anim.timer -= anim.frameTime;
        anim.currentFrame++;

        if (anim.currentFrame >= anim.frameCount)
        {
            if (anim.loop)
            {
                anim.currentFrame = 0;
            }
            else
            {
                anim.currentFrame = anim.frameCount - 1;
                anim.finished = true;
            }
        }
    }
}

void updateCharacterAnimation(CharacterSprite &cs, float dt)
{
    updateAnimation(cs.anims[cs.currentAnim], dt);
}

void setAnimation(CharacterSprite &cs, AnimationType type)
{
    // Chỉ chuyển nếu animation đó đã được load
    if (!cs.anims[type].loaded)
        return;

    // Không reset nếu đang phát cùng animation (trừ khi đã finished)
    if (cs.currentAnim == type && !cs.anims[type].finished)
        return;

    cs.currentAnim = type;
    cs.anims[type].currentFrame = 0;
    cs.anims[type].timer = 0.0f;
    cs.anims[type].finished = false;
}

bool isAnimationDone(const SpriteAnimation &anim)
{
    return anim.finished;
}

// ============================================================
// API: Vẽ sprite
// ============================================================

void drawSprite(const SpriteAnimation &anim,
                float x, float y,
                float scale,
                bool flipH)
{
    if (!anim.loaded)
        return;

    // Source rectangle: cắt frame hiện tại từ spritesheet
    float srcX = (float)(anim.currentFrame * SPRITE_FRAME_SIZE);
    float srcW = (float)SPRITE_FRAME_SIZE;
    float srcH = (float)SPRITE_FRAME_SIZE;

    // Lật ngang bằng cách đảo chiều rộng source
    if (flipH) srcW = -srcW;

    Rectangle src = { srcX, 0.0f, srcW, srcH };

    // Destination rectangle: vị trí và kích thước trên màn hình
    float destW = SPRITE_FRAME_SIZE * scale;
    float destH = SPRITE_FRAME_SIZE * scale;
    Rectangle dest = { x, y, destW, destH };

    DrawTexturePro(anim.sheet, src, dest, {0, 0}, 0.0f, WHITE);
}

void drawCharacterSprite(const CharacterSprite &cs,
                         float x, float y,
                         float scale,
                         bool flipH)
{
    drawSprite(cs.anims[cs.currentAnim], x, y, scale, flipH);
}

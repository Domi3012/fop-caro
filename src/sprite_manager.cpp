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

// ============================================================
// Attack Step Data — Resolution-independent
// ============================================================
// offsetX: tỉ lệ screenW. Khoảng cách X↔O ≈ 0.70 screenW.
//   0.0  = vị trí gốc
//   0.55 = sát bên đối thủ
//   Khi flipH (Player O), dx tự đảo dấu trong view.
// offsetY: tỉ lệ screenH. Âm = lên trên.

// IMPOSTER (Assassin): đứng yên 4 frame, teleport 5 frame cuối
static const AttackStep IMPOSTER_ATTACK[ATTACK_STEP_COUNT] = {
//  frame  offsetX  offsetY  effect  hit
    {0,    0.00f,   0.00f,   -1,     false},  // Bước 1: chuẩn bị
    {1,    0.00f,   0.00f,   -1,     false},  // Bước 2: rung
    {2,    0.00f,   0.00f,   -1,     false},  // Bước 3
    {3,    0.00f,   0.00f,   -1,     false},  // Bước 4
    {4,    0.70f,   0.00f,   -1,     false},  // Bước 5: teleport
    {5,    0.70f,   0.00f,   -1,     true},   // Bước 6: đánh
    {6,    0.70f,   0.00f,   -1,     true},   // Bước 7: đánh
    {7,    0.70f,   0.00f,   -1,     true},   // Bước 8: đánh
    {8,    0.70f,   0.00f,   -1,     true},   // Bước 9: đánh
};

// BRUISER: tụ lực 4 frame, lao tới 3 frame, quay về + khiên vỡ 2 frame
static const AttackStep BRUISER_ATTACK[ATTACK_STEP_COUNT] = {
    {0,    0.00f,   0.00f,   -1,     false},  // Tụ lực
    {1,    0.00f,   0.00f,   -1,     false},  // Tụ lực
    {2,    0.00f,   0.00f,   -1,     false},  // Tụ lực
    {3,    0.00f,   0.00f,   -1,     false},  // Tụ lực
    {4,    0.18f,   0.00f,   -1,     false},  // Lao tới
    {5,    0.36f,   0.00f,   -1,     false},  // Lao tiếp
    {6,    0.70f,   0.00f,   -1,     true},   // Đánh
    {7,    0.00f,   0.00f,    8,     false},  // Quay về + khiên vỡ
    {7,    0.00f,   0.00f,    9,     false},  // Giữ + khiên vỡ tiếp
};

// VAMPIRE: xoè cánh, lao liền mạch 9 frame
static const AttackStep VAMPIRE_ATTACK[ATTACK_STEP_COUNT] = {
    {0,    0.00f,   0.00f,   -1,     false},  // Xoè cánh
    {1,    0.00f,   0.00f,   -1,     false},  // Xoè lớn
    {2,    0.00f,   0.00f,   -1,     false},  // Thu cánh
    {3,    0.15f,  -0.02f,   -1,     false},  // Bắt đầu lao
    {4,    0.35f,  -0.03f,   -1,     false},  // Giữa đường
    {5,    0.70f,   0.00f,   -1,     true},   // Đánh
    {6,    0.70f,   0.00f,   -1,     true},   // Đánh tiếp
    {7,    0.70f,   0.00f,   -1,     true},   // Đánh tiếp
    {8,    0.70f,   0.00f,   -1,     false},  // Kết thúc
};

// SORCERER: tụ lực, bắn phép tại chỗ, effect nổ trên defender
static const AttackStep SORCERER_ATTACK[ATTACK_STEP_COUNT] = {
    {0,    0.00f,   0.00f,   -1,     false},  // Tụ lực
    {1,    0.00f,   0.00f,   -1,     false},  // Tụ lực
    {2,    0.00f,   0.00f,   -1,     false},  // Tụ lực
    {3,    0.00f,   0.00f,   -1,     false},  // Tụ lực
    {4,    0.00f,   0.00f,    5,     false},  // Đánh + nổ bắt đầu
    {0,    0.00f,   0.00f,    6,     true},   // Về idle + nổ
    {0,    0.00f,   0.00f,    7,     true},   // Về idle + nổ
    {0,    0.00f,   0.00f,    8,     true},   // Về idle + nổ
    {0,    0.00f,   0.00f,    9,     false},  // Về idle + nổ tan
};

// Bảng tra cứu theo CharacterType
static const AttackStep* ALL_ATTACK_STEPS[4] = {
    IMPOSTER_ATTACK,   // ASSASSIN  = 0
    BRUISER_ATTACK,    // BRUISER   = 1
    VAMPIRE_ATTACK,    // VAMPIRE   = 2
    SORCERER_ATTACK,   // SORCERER  = 3
};

const AttackStep* getAttackSteps(CharacterType type)
{
    int idx = static_cast<int>(type);
    if (idx < 0 || idx >= 4) idx = 0;
    return ALL_ATTACK_STEPS[idx];
}

void drawEffectFrame(CharacterType attackerType,
                     int effectFrame,
                     float x, float y,
                     float scale, bool flipH)
{
    int idx = static_cast<int>(attackerType);
    if (idx < 0 || idx >= 4) return;

    const SpriteAnimation &attackAnim = characterSprites[idx].anims[ANIM_ATTACK];
    if (!attackAnim.loaded) return;
    if (effectFrame < 0 || effectFrame >= attackAnim.frameCount) return;

    float srcX = (float)(effectFrame * SPRITE_FRAME_SIZE);
    float srcW = (float)SPRITE_FRAME_SIZE;
    float srcH = (float)SPRITE_FRAME_SIZE;
    if (flipH) srcW = -srcW;

    Rectangle src = { srcX, 0.0f, srcW, srcH };
    float destW = SPRITE_FRAME_SIZE * scale;
    float destH = SPRITE_FRAME_SIZE * scale;
    Rectangle dest = { x, y, destW, destH };

    DrawTexturePro(attackAnim.sheet, src, dest, {0, 0}, 0.0f, WHITE);
}


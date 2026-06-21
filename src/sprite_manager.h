#pragma once

#include "model.h"
#include "raylib.h"

// ============================================================
// Sprite Animation System
// ============================================================

// Loại animation — mở rộng thêm khi có sprite mới
enum AnimationType
{
    ANIM_IDLE,
    ANIM_ATTACK,
    ANIM_COUNT   // Tổng số loại animation (dùng làm kích thước mảng)
};

// Một animation cụ thể (VD: idle 4 frame, attack 4 frame)
struct SpriteAnimation
{
    Texture2D sheet;        // Spritesheet (các frame nằm ngang, mỗi frame 64×64)
    int frameCount;         // Số frame trong sheet
    int currentFrame;       // Frame đang hiển thị (0-indexed)
    float frameTime;        // Thời gian mỗi frame (giây)
    float timer;            // Bộ đếm thời gian tích luỹ
    bool loop;              // true = lặp vô hạn (idle), false = chạy 1 lần (attack)
    bool finished;          // true khi animation one-shot đã chạy xong
    bool loaded;            // true nếu texture đã được load thành công
};

// Tập hợp tất cả animation của một nhân vật
struct CharacterSprite
{
    SpriteAnimation anims[ANIM_COUNT]; // Mảng animation theo AnimationType
    AnimationType currentAnim;         // Animation đang phát
};

// ============================================================
// Hằng số
// ============================================================

constexpr int SPRITE_FRAME_SIZE = 64;           // Kích thước gốc mỗi frame (pixel)
constexpr float DEFAULT_FRAME_TIME = 0.15f;     // Thời gian mặc định mỗi frame (giây)

// ============================================================
// API chính
// ============================================================

// Khởi tạo: load tất cả spritesheet cho 4 nhân vật.
// Gọi sau InitWindow(), trước vòng lặp game.
void initSpriteManager();

// Giải phóng tất cả texture.
// Gọi trước CloseWindow().
void unloadSpriteManager();

// Lấy sprite của nhân vật theo CharacterType (tham chiếu chung — dùng cho preview).
CharacterSprite& getCharacterSprite(CharacterType type);

// Lấy bản sao độc lập của sprite nhân vật (dùng cho mỗi player riêng biệt,
// tránh chia sẻ timer/frame khi 2 người chọn cùng nhân vật).
CharacterSprite getCharacterSpriteCopy(CharacterType type);

// ============================================================
// Điều khiển animation
// ============================================================

// Cập nhật timer và chuyển frame. Gọi mỗi frame với dt = GetFrameTime().
void updateAnimation(SpriteAnimation &anim, float dt);

// Cập nhật animation đang phát của CharacterSprite (tiện lợi).
void updateCharacterAnimation(CharacterSprite &cs, float dt);

// Chuyển sang animation khác. Reset frame về 0.
// Nếu animation chưa được load, giữ nguyên animation cũ.
void setAnimation(CharacterSprite &cs, AnimationType type);

// Kiểm tra animation one-shot (attack) đã chạy xong chưa.
bool isAnimationDone(const SpriteAnimation &anim);

// ============================================================
// Vẽ sprite
// ============================================================

// Vẽ frame hiện tại của animation tại vị trí (x, y).
// scale: hệ số phóng to (VD: 3.0f = 192×192 pixel).
// flipH: true = lật ngang (dùng cho nhân vật bên phải nhìn sang trái).
void drawSprite(const SpriteAnimation &anim,
                float x, float y,
                float scale = 1.0f,
                bool flipH = false);

// Vẽ animation đang phát của CharacterSprite (tiện lợi).
void drawCharacterSprite(const CharacterSprite &cs,
                         float x, float y,
                         float scale = 1.0f,
                         bool flipH = false);

// ============================================================
// Attack Animation Data — Per-character, per-step
// ============================================================

// Một bước trong chuỗi attack animation (tổng 9 bước, 200ms/bước = 1800ms)
struct AttackStep
{
    int attackerFrame;      // Frame index trên spritesheet attacker (0-indexed)
    float offsetX;          // Offset X (tỉ lệ screenW, dương = về phía đối thủ)
    float offsetY;          // Offset Y (tỉ lệ screenH, âm = lên trên)
    int effectFrame;        // Frame index cho effect trên defender (-1 = không có)
    bool defenderHit;       // true = defender bị rung+flash ở bước này
};

constexpr int ATTACK_STEP_COUNT = 9;          // Mọi nhân vật đều 9 bước
constexpr float ATTACK_FRAME_TIME = 0.20f;    // 200ms/bước

// Lấy bảng 9 AttackStep cho nhân vật theo CharacterType.
const AttackStep* getAttackSteps(CharacterType type);

// Vẽ một frame effect (khiên vỡ / nổ) từ spritesheet attack của nhân vật.
// effectFrame là index trên spritesheet gốc (VD: bruiser frame 8-9, sorcerer frame 5-9).
void drawEffectFrame(CharacterType attackerType,
                     int effectFrame,
                     float x, float y,
                     float scale, bool flipH);

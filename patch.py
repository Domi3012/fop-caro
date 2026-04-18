import re

with open('/home/domi1712/Desktop/fop-caro/src/view.cpp', 'r', encoding='utf-8') as f:
    text = f.read()

# 1. Update declaration
text = text.replace('void drawParallaxBackground(float extraPanLeft = 0.0f);', 'void drawParallaxBackground(float speedMultiplier = 1.0f);')

# 2. Add drawCharacters helper
char_helper = """
void drawCharacters(float shiftX) {
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();
    
    // Ve placeholder hinh chu nhat
    float charW = screenW * 0.08f;
    float charH = screenH * 0.20f;
    
    float baseGroundY = screenH * 0.85f; // Platform khoang 85% manh hinh
    
    float posX_X = screenW * 0.15f + shiftX - charW/2.0f;
    float posX_O = screenW * 0.85f + shiftX - charW/2.0f;
    
    DrawRectangle(posX_X, baseGroundY - charH, charW, charH, Fade(RED, 0.8f));
    DrawRectangleLinesEx({posX_X, baseGroundY - charH, charW, charH}, 3, WHITE);
    DrawTextEx(font8bit, "X", { posX_X + charW/2.0f - 15, baseGroundY - charH/2.0f - 20 }, 40, 2, WHITE);
    
    DrawRectangle(posX_O, baseGroundY - charH, charW, charH, Fade(BLUE, 0.8f));
    DrawRectangleLinesEx({posX_O, baseGroundY - charH, charW, charH}, 3, WHITE);
    DrawTextEx(font8bit, "O", { posX_O + charW/2.0f - 15, baseGroundY - charH/2.0f - 20 }, 40, 2, WHITE);
}
"""
text = text.replace('// --- PARALLAX BACKGROUND ---', char_helper + '\n// --- PARALLAX BACKGROUND ---')

# 3. Modify drawParallaxBackground body
old_parallax = """void drawParallaxBackground(float extraPanLeft) {
    float dt = GetFrameTime();
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();

    // Rừng hiện thị ở 2/3 phías dưới màn hình
    float forestH  = screenH * (2.0f / 3.0f);  // chiều cao vùng rừng
    float drawY    = (float)screenH - forestH;  // Y bắt đầu (= screenH / 3)

    for (int i = 0; i < FOREST_LAYER_COUNT; i++) {
        ParallaxLayer& layer = forestLayers[i];
        Texture2D& tex = layer.texture;

        // Cập nhật offset cuộn
        layer.scrollX -= layer.speed * dt;
        
        // Them pan left do intro camera (layer nhanh dich nhieu hon, layer cham dich it hon)
        // layer.speed tu 2.5 den 30
        float factor = layer.speed / 30.0f;
        float actualScrollX = layer.scrollX - extraPanLeft * factor;

        // Source rect: ch\u1ec9 l\u1ea5y 2/3 ph\u00edas d\u01b0\u1edbi c\u1ee7a \u1ea3nh g\u1ed1c (b\u1ecf 1/3 tr\u00ean)
        float srcY = tex.height / 3.0f;
        float srcH = tex.height * (2.0f / 3.0f);
        float srcW = (float)tex.width;

        // Dest width: gi\u1eef aspect ratio c\u1ee7a v\u00f9ng crop, fill full chieu cao man hinh
        float scaledW = srcW / srcH * screenH;

        // Wrap
        while (actualScrollX <= -scaledW)
            actualScrollX += scaledW;

        // Vẽ đủ số bản để phủ kín toàn màn hình
        int copies = (int)((float)screenW / scaledW) + 2;
        for (int c = 0; c < copies; c++) {
            Rectangle src  = { 0, srcY, srcW, srcH };
            Rectangle dest = { actualScrollX + c * scaledW, 0, scaledW, (float)screenH };
            DrawTexturePro(tex, src, dest, { 0, 0 }, 0.0f, WHITE);
        }
    }
}"""

new_parallax = """void drawParallaxBackground(float speedMultiplier) {
    float dt = GetFrameTime();
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();

    // Rừng hiện thị ở 2/3 phías dưới màn hình
    float forestH  = screenH * (2.0f / 3.0f);  // chiều cao vùng rừng
    float drawY    = (float)screenH - forestH;  // Y bắt đầu (= screenH / 3)

    for (int i = 0; i < FOREST_LAYER_COUNT; i++) {
        ParallaxLayer& layer = forestLayers[i];
        Texture2D& tex = layer.texture;

        // Cập nhật offset cuộn
        layer.scrollX -= layer.speed * speedMultiplier * dt;

        // Source rect: ch\u1ec9 l\u1ea5y 2/3 ph\u00edas d\u01b0\u1edbi c\u1ee7a \u1ea3nh g\u1ed1c (b\u1ecf 1/3 tr\u00ean)
        float srcY = tex.height / 3.0f;
        float srcH = tex.height * (2.0f / 3.0f);
        float srcW = (float)tex.width;

        // Dest width: gi\u1eef aspect ratio c\u1ee7a v\u00f9ng crop, fill full chieu cao man hinh
        float scaledW = srcW / srcH * screenH;

        // Wrap
        while (layer.scrollX <= -scaledW)
            layer.scrollX += scaledW;

        // Vẽ đủ số bản để phủ kín toàn màn hình
        int copies = (int)((float)screenW / scaledW) + 2;
        for (int c = 0; c < copies; c++) {
            Rectangle src  = { 0, srcY, srcW, srcH };
            Rectangle dest = { layer.scrollX + c * scaledW, 0, scaledW, (float)screenH };
            DrawTexturePro(tex, src, dest, { 0, 0 }, 0.0f, WHITE);
        }
    }
}"""

text = text.replace(old_parallax, new_parallax)

# 4. Update drawGameIntro
old_intro = """void drawGameIntro(const MatchState& match, const UIState& ui) {
    // Parallax ban nay can drawParallaxBackground(ui.introCamX) de tao hieu ung lao toi
    drawParallaxBackground(ui.introCamX);
    
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();
    
    // Ve hai nhan vat o tren platform
    // Ve placeholder hinh chu nhat
    float charW = screenW * 0.08f;
    float charH = screenH * 0.20f;
    
    float baseGroundY = screenH * 0.85f; // Platform khoang 85% manh hinh
    
    // extra shift vi camera di tu phai qua
    float shiftX = ui.introCamX; 
    
    float posX_X = screenW * 0.35f + shiftX - charW/2.0f;
    float posX_O = screenW * 0.65f + shiftX - charW/2.0f;
    
    DrawRectangle(posX_X, baseGroundY - charH, charW, charH, Fade(RED, 0.8f));
    DrawRectangleLinesEx({posX_X, baseGroundY - charH, charW, charH}, 3, WHITE);
    DrawTextEx(font8bit, "X", { posX_X + charW/2.0f - 15, baseGroundY - charH/2.0f - 20 }, 40, 2, WHITE);
    
    DrawRectangle(posX_O, baseGroundY - charH, charW, charH, Fade(BLUE, 0.8f));
    DrawRectangleLinesEx({posX_O, baseGroundY - charH, charW, charH}, 3, WHITE);
    DrawTextEx(font8bit, "O", { posX_O + charW/2.0f - 15, baseGroundY - charH/2.0f - 20 }, 40, 2, WHITE);
}"""

new_intro = """void drawGameIntro(const MatchState& match, const UIState& ui) {
    drawParallaxBackground(15.0f);
    drawCharacters(ui.introCamX);
}"""

text = text.replace(old_intro, new_intro)

# 5. Update drawCaroGame
old_board = """// Nhom ban co
void drawCaroGame(const MatchState& match, const UIState& ui) {
    
    drawParallaxBackground();

    // Vẽ hai bên trước, vẽ bàn cờ ở giữa sau
    drawStatusPanel(match);
    drawBoard(match, ui);
}"""

new_board = """// Nhom ban co
void drawCaroGame(const MatchState& match, const UIState& ui) {
    
    drawParallaxBackground(0.0f); // Lock background
    drawCharacters(0.0f);         // Ve character o trang thai Dung im

    // Vẽ hai bên trước, vẽ bàn cờ ở giữa sau
    drawStatusPanel(match);
    drawBoard(match, ui);
}"""

text = text.replace(old_board, new_board)

# 6. Update drawGameOver
old_over = """void drawGameOver(const MatchState& match, const UIState& ui) {
    
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();

    drawParallaxBackground();"""

new_over = """void drawGameOver(const MatchState& match, const UIState& ui) {
    
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();

    drawParallaxBackground(0.0f); // Lock background
    drawCharacters(0.0f);"""

text = text.replace(old_over, new_over)

# 7. Update other drawParallaxBackground() calls to drawParallaxBackground(1.0f)
text = text.replace('drawParallaxBackground()', 'drawParallaxBackground(1.0f)')

with open('/home/domi1712/Desktop/fop-caro/src/view.cpp', 'w', encoding='utf-8') as f:
    f.write(text)

print("success")

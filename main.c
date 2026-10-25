/* ==========================================================================
 *  DOODLE JUMP GBA - 60FPS Canvas Edition (1:1 Physics)
 *  Zero Sprite Tearing, Zero Cropping, Flawless Dimensions
 * ========================================================================== */

#define REG_DISPCNT     *(volatile unsigned short*)0x04000000
#define REG_VCOUNT      *(volatile unsigned short*)0x04000006
#define REG_KEYINPUT    *(volatile unsigned short*)0x04000130

#define REG_DMA3SAD     *(volatile unsigned int*)0x040000D4
#define REG_DMA3DAD     *(volatile unsigned int*)0x040000D8
#define REG_DMA3CNT     *(volatile unsigned int*)0x040000DC

// --- GBA Hardware Audio ---
#define REG_SOUNDCNT_L  *(volatile unsigned short*)0x04000080
#define REG_SOUNDCNT_H  *(volatile unsigned short*)0x04000082
#define REG_SOUNDCNT_X  *(volatile unsigned short*)0x04000084

#define REG_SOUND1CNT_L *(volatile unsigned short*)0x04000060
#define REG_SOUND1CNT_H *(volatile unsigned short*)0x04000062
#define REG_SOUND1CNT_X *(volatile unsigned short*)0x04000064

#define REG_SOUND2CNT_L *(volatile unsigned short*)0x04000068
#define REG_SOUND2CNT_H *(volatile unsigned short*)0x0400006C

#define REG_SOUND4CNT_L *(volatile unsigned short*)0x04000078
#define REG_SOUND4CNT_H *(volatile unsigned short*)0x0400007C

#define KEY_A           (1 << 0)
#define KEY_B           (1 << 1)
#define KEY_START       (1 << 3)
#define KEY_RIGHT       (1 << 4)
#define KEY_LEFT        (1 << 5)
#define KEY_DOWN        (1 << 7)

#define RGB15(r, g, b)  ((r) | ((g) << 5) | ((b) << 10))

#define SCREEN_W 240
#define SCREEN_H 160

#define screen       ((unsigned char*)0x02000000)
#define bg_template  ((unsigned char*)0x0200A000)
#define PALETTE_MEM  ((volatile unsigned short*)0x05000000)

// 16.16 Fixed Point Math (Chromium 1:1 Physics)
#define FP_SHIFT        16
#define INT_TO_FP(x)    ((x) << FP_SHIFT)
#define FP_TO_INT(x)    ((x) >> FP_SHIFT)

#define SPEED_MAX       340787  // 5.20 px/frame
#define SPEED_ACCEL     26      // 0.0004 px/frame
#define GRAVITY         16384   // 0.25 px/frame^2
#define JUMP_VEL        -275251 // -4.20 px/frame
#define JUMP_DROP_VEL   -137626 // -2.10 px/frame (variable jump cutoff)
#define FAST_FALL       49152   // 3x Gravity drop
#define SPRING_VEL      -430000 // Super bounce

// --- Palette Indices ---
#define C_TRANS      0
#define C_PAPER      1
#define C_GRID       2
#define C_RED1       3
#define C_RED2       4
#define C_TEXT       5

#define C_DOOD_LINE  6
#define C_DOOD_BODY  7
#define C_DOOD_EYE   8
#define C_DOOD_PUPIL 9
#define C_DOOD_HOLE  10
#define C_DOOD_LEG   11

#define C_PLAT_HIGH  12
#define C_PLAT_MAIN  13
#define C_PLAT_SHAD  14
#define C_PLAT_LINE  15

#define C_BLUE_HIGH  16
#define C_BLUE_MAIN  17
#define C_BLUE_SHAD  18

#define C_WOOD_MAIN  19
#define C_WOOD_SHAD  20
#define C_WOOD_CRACK 21

#define C_SPRING_GRY 22
#define C_SPRING_RED 23

static void init_palettes(void) {
    PALETTE_MEM[C_TRANS]      = 0x0000;
    PALETTE_MEM[C_PAPER]      = RGB15(31, 31, 28);
    PALETTE_MEM[C_GRID]       = RGB15(25, 27, 29);
    PALETTE_MEM[C_RED1]       = RGB15(28, 12, 12);
    PALETTE_MEM[C_RED2]       = RGB15(29, 18, 18);
    PALETTE_MEM[C_TEXT]       = RGB15(4, 5, 7);

    // Official Doodler Colors
    PALETTE_MEM[C_DOOD_LINE]  = RGB15(6, 10, 2);
    PALETTE_MEM[C_DOOD_BODY]  = RGB15(23, 29, 6);
    PALETTE_MEM[C_DOOD_EYE]   = RGB15(31, 31, 31);
    PALETTE_MEM[C_DOOD_PUPIL] = RGB15(0, 0, 0);
    PALETTE_MEM[C_DOOD_HOLE]  = RGB15(4, 6, 1);
    PALETTE_MEM[C_DOOD_LEG]   = RGB15(15, 20, 4);

    // Green Platform
    PALETTE_MEM[C_PLAT_HIGH]  = RGB15(25, 31, 12);
    PALETTE_MEM[C_PLAT_MAIN]  = RGB15(14, 27, 6);
    PALETTE_MEM[C_PLAT_SHAD]  = RGB15(7, 18, 3);
    PALETTE_MEM[C_PLAT_LINE]  = RGB15(2, 9, 2);

    // Blue Platform
    PALETTE_MEM[C_BLUE_HIGH]  = RGB15(18, 28, 31);
    PALETTE_MEM[C_BLUE_MAIN]  = RGB15(8, 19, 29);
    PALETTE_MEM[C_BLUE_SHAD]  = RGB15(4, 11, 20);

    // Wood Platform
    PALETTE_MEM[C_WOOD_MAIN]  = RGB15(20, 13, 7);
    PALETTE_MEM[C_WOOD_SHAD]  = RGB15(12, 7, 3);
    PALETTE_MEM[C_WOOD_CRACK] = RGB15(5, 3, 1);

    // Spring
    PALETTE_MEM[C_SPRING_GRY] = RGB15(22, 22, 22);
    PALETTE_MEM[C_SPRING_RED] = RGB15(28, 5, 5);
}

// ==========================================================================
// ACCURATE 24x22 DOODLER SPRITE (NO HARDWARE CROPPING ISSUES)
// ==========================================================================
static const unsigned char doodler_map[22][24] = {
    {0,0,0,0,0,0,0,0,6,6,6,6,6,6,6,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,6,6,7,7,7,7,7,7,7,6,6,0,0,0,0,0,0,0},
    {0,0,0,0,0,6,7,7,7,7,7,7,7,7,7,7,7,6,0,0,0,0,0,0},
    {0,0,0,0,6,7,7,7,7,7,7,7,7,7,7,7,7,7,6,0,0,0,0,0},
    {0,0,0,0,6,7,7,7,8,8,7,7,8,8,7,7,7,7,6,0,0,0,0,0}, // Eyes
    {0,0,0,0,6,7,7,8,9,9,8,8,9,9,8,7,7,7,6,0,0,0,0,0}, // Pupils
    {0,0,0,0,6,7,7,8,9,9,8,8,9,9,8,7,7,7,6,0,0,0,0,0},
    {0,0,0,0,6,7,7,7,8,8,7,7,8,8,7,7,7,7,6,0,0,0,0,0},
    {0,0,0,0,6,7,7,7,7,7,7,7,7,7,7,7,7,7,7,6,6,6,0,0},
    {0,0,0,0,6,7,7,7,7,7,7,7,7,7,7,7,7,7,7,6,10,10,6,0}, // Snout Hole
    {0,0,0,0,6,7,7,7,7,7,7,7,7,7,7,7,7,7,7,6,10,10,6,0},
    {0,0,0,0,6,7,7,7,7,7,7,7,7,7,7,7,7,7,7,6,6,6,0,0},
    {0,0,0,0,6,7,7,7,7,7,7,7,7,7,7,7,7,7,6,0,0,0,0,0},
    {0,0,0,0,0,6,7,7,7,7,7,7,7,7,7,7,7,6,0,0,0,0,0,0},
    {0,0,0,0,0,0,6,6,7,7,7,7,7,7,7,6,6,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,6,6,6,6,6,6,6,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,6,6,0,0,6,6,0,0,6,6,0,0,6,6,0,0,0,0,0,0}, // 4 Legs
    {0,0,0,6,11,11,6,6,11,11,6,6,11,11,6,6,11,11,6,0,0,0,0,0},
    {0,0,0,6,11,11,6,6,11,11,6,6,11,11,6,6,11,11,6,0,0,0,0,0},
    {0,0,0,6,11,11,6,6,11,11,6,6,11,11,6,6,11,11,6,0,0,0,0,0},
    {0,0,0,0,6,6,0,0,6,6,0,0,6,6,0,0,6,6,0,0,0,0,0,0}
};

static void init_background(void) {
    for (int y = 0; y < SCREEN_H; y++) {
        int is_hgrid = ((y & 15) == 0);
        int row = y * SCREEN_W;
        for (int x = 0; x < SCREEN_W; x++) {
            if (x == 26) bg_template[row + x] = C_RED1;
            else if (x == 28) bg_template[row + x] = C_RED2;
            else if (is_hgrid || ((x & 15) == 0)) bg_template[row + x] = C_GRID;
            else bg_template[row + x] = C_PAPER;
        }
    }
}

static void init_sound(void) {
    REG_SOUNDCNT_X = 0x0080;
    REG_SOUNDCNT_L = 0x7777;
    REG_SOUNDCNT_H = 0x0002;
}

static inline void snd_jump(void)   { REG_SOUND1CNT_L = 0x0013; REG_SOUND1CNT_H = 0xB280; REG_SOUND1CNT_X = 0x8530; }
static inline void snd_spring(void) { REG_SOUND1CNT_L = 0x0015; REG_SOUND1CNT_H = 0xF380; REG_SOUND1CNT_X = 0x86B0; }
static inline void snd_crack(void)  { REG_SOUND4CNT_L = 0xB100; REG_SOUND4CNT_H = 0x8035; }
static inline void snd_fall(void)   { REG_SOUND1CNT_L = 0x003D; REG_SOUND1CNT_H = 0xC480; REG_SOUND1CNT_X = 0x8600; }
static inline void snd_shoot(void)  { REG_SOUND2CNT_L = 0xD180; REG_SOUND2CNT_H = 0x8780; }

static unsigned int rng = 0x87654321;
static inline unsigned int rand_next(void) {
    rng = rng * 1664525u + 1013904223u;
    return (rng >> 16);
}

// ==========================================================================
// PROCEDURAL CANVAS DRAWING (Completely avoids 8x8 Hardware tile limits)
// ==========================================================================

static inline void vsync(void) {
    while (REG_VCOUNT >= 160);
    while (REG_VCOUNT < 160);
}

static inline void clear_screen_dma(void) {
    REG_DMA3SAD = (unsigned int)bg_template;
    REG_DMA3DAD = (unsigned int)screen;
    REG_DMA3CNT = 0x84000000 | (SCREEN_W * SCREEN_H / 4);
}

static inline void flip_display(void) {
    vsync();
    REG_DMA3SAD = (unsigned int)screen;
    REG_DMA3DAD = 0x06000000;
    REG_DMA3CNT = 0x84000000 | (SCREEN_W * SCREEN_H / 4);
}

// Draws the exact shape of a Doodle Jump platform (40x10)
static void draw_platform(int px, int py, int type, int crumble_timer) {
    int draw_x = px;
    if (type == 2 && crumble_timer > 0) {
        draw_x += ((crumble_timer & 2) ? 1 : -1); // Shake effect
    }

    unsigned char c_high = C_PLAT_HIGH;
    unsigned char c_main = C_PLAT_MAIN;
    unsigned char c_shad = C_PLAT_SHAD;
    unsigned char c_line = C_PLAT_LINE;

    if (type == 1) { c_high = C_BLUE_HIGH; c_main = C_BLUE_MAIN; c_shad = C_BLUE_SHAD; }
    else if (type == 2) { c_high = C_WOOD_MAIN; c_main = C_WOOD_MAIN; c_shad = C_WOOD_SHAD; c_line = C_WOOD_SHAD; }

    for (int y = 0; y < 10; y++) {
        int sy = py + y;
        if (sy < 0 || sy >= SCREEN_H) continue;
        int row = sy * SCREEN_W;

        for (int x = 0; x < 40; x++) {
            int sx = draw_x + x;
            if (sx < 0 || sx >= SCREEN_W) continue;

            if (type == 2) {
                if (crumble_timer > 0 && (x == 20 + (y % 3) || (crumble_timer < 45 && x == 12 + y))) {
                    screen[row + sx] = C_WOOD_CRACK; continue;
                } else if (crumble_timer < 0 && (x >= 18 && x <= 22)) continue;
            }

            // Rounded pill caps
            if ((y == 0 || y == 9) && (x < 3 || x > 36)) continue;
            if ((y == 1 || y == 8) && (x < 1 || x > 38)) continue;

            unsigned char c;
            if (y == 0 || y == 9 || x == 0 || x == 39 || ((y == 1 || y == 8) && (x < 3 || x > 36))) c = c_line;
            else if (y == 1 || y == 2) c = c_high;
            else if (y < 7) c = c_main;
            else c = c_shad;

            screen[row + sx] = c;
        }
    }
}

static void draw_spring(int px, int py, int extended) {
    int h = extended ? 14 : 7;
    for (int y = 0; y < h; y++) {
        int sy = py - h + y;
        if (sy < 0 || sy >= SCREEN_H) continue;
        int row = sy * SCREEN_W;
        for (int x = 0; x < 12; x++) {
            int sx = px + x;
            if (sx < 0 || sx >= SCREEN_W) continue;
            if (y <= 1 && x >= 2 && x <= 9) screen[row + sx] = C_SPRING_RED;
            else if ((x == 0 || x == 11) || ((y & 1) && x > 0 && x < 11)) screen[row + sx] = C_SPRING_GRY;
        }
    }
}

static void draw_doodler(int px, int py, int facing, int vy) {
    // Dynamic Leg Squish
    int leg_offset = (vy < 0) ? -2 : 0; 

    for (int y = 0; y < 22; y++) {
        int sy = py + y;
        if (sy < 0 || sy >= SCREEN_H) continue;
        int row = sy * SCREEN_W;
        
        int src_y = y;
        if (y >= 17 && leg_offset != 0) src_y = y + leg_offset;

        for (int x = 0; x < 24; x++) {
            int src_x = (facing == 1) ? x : (23 - x);
            unsigned char c = doodler_map[src_y][src_x];
            if (c != 0) {
                int sx = px + x;
                if (sx >= 0 && sx < SCREEN_W) screen[row + sx] = c;
            }
        }
    }
}

static const unsigned char font3x5[10][5] = {
    {0x7, 0x5, 0x5, 0x5, 0x7}, {0x2, 0x6, 0x2, 0x2, 0x7},
    {0x7, 0x1, 0x7, 0x4, 0x7}, {0x7, 0x1, 0x7, 0x1, 0x7},
    {0x5, 0x5, 0x7, 0x1, 0x1}, {0x7, 0x4, 0x7, 0x1, 0x7},
    {0x7, 0x4, 0x7, 0x5, 0x7}, {0x7, 0x1, 0x2, 0x4, 0x4},
    {0x7, 0x5, 0x7, 0x5, 0x7}, {0x7, 0x5, 0x7, 0x1, 0x7}
};

static void draw_score(int x, int y, int value, int scale) {
    char buf[10]; int len = 0;
    if (value == 0) buf[len++] = 0;
    else { while (value > 0 && len < 10) { buf[len++] = value % 10; value /= 10; } }

    for (int i = len - 1; i >= 0; i--) {
        int d = buf[i];
        for (int r = 0; r < 5; r++) {
            unsigned char row_bits = font3x5[d][r];
            for (int c = 0; c < 3; c++) {
                if (row_bits & (1 << (2 - c))) {
                    for (int dy = 0; dy < scale; dy++) {
                        int py = y + r * scale + dy;
                        if (py >= SCREEN_H) continue;
                        int row = py * SCREEN_W;
                        for (int dx = 0; dx < scale; dx++) {
                            int px = x + c * scale + dx;
                            if (px < SCREEN_W) screen[row + px] = C_TEXT;
                        }
                    }
                }
            }
        }
        x += (3 * scale) + (scale > 1 ? scale : 1);
    }
}

// ==========================================================================
// GAME STATE & CONSTANTS
// ==========================================================================

#define NUM_PLATFORMS 7
#define PLATFORM_W    40
#define PLATFORM_H    10
#define PLAYER_W      24
#define PLAYER_H      22
#define MOVE_SPEED    183500

typedef struct {
    int x, y, type, vx, has_spring, crumble_timer, spring_anim;
} Platform;

typedef struct {
    int x, y, vy, facing;
} Player;

#define MAX_PELLETS 4
typedef struct { int x, y, vy, active; } Pellet;

static Platform platforms[NUM_PLATFORMS];
static Player player;
static Pellet pellets[MAX_PELLETS];
static int score = 0, speed_fp = 157286, game_over = 0;

static void spawn_platform(int i, int y_pos) {
    platforms[i].y = y_pos;
    platforms[i].x = rand_next() % (SCREEN_W - PLATFORM_W - 20) + 10;
    platforms[i].crumble_timer = 0;
    platforms[i].has_spring = 0;
    platforms[i].spring_anim = 0;

    int r = rand_next() % 100;
    if (score < 400) {
        platforms[i].type = 0;
        platforms[i].vx = 0;
        if ((rand_next() % 100) < 30) platforms[i].has_spring = 1;
    } else if (score < 1200) {
        if (r < 65) platforms[i].type = 0;
        else { platforms[i].type = 1; platforms[i].vx = (rand_next() & 1) ? 1 : -1; }
        if ((rand_next() % 100) < 22) platforms[i].has_spring = 1;
    } else {
        if (r < 40) platforms[i].type = 0;
        else if (r < 75) { platforms[i].type = 1; platforms[i].vx = (rand_next() & 1) ? 2 : -2; }
        else { platforms[i].type = 2; platforms[i].vx = 0; }
        if (platforms[i].type != 2 && (rand_next() % 100) < 14) platforms[i].has_spring = 1;
    }
}

static void reset_game(void) {
    score = 0;
    speed_fp = 157286;
    game_over = 0;

    player.x = INT_TO_FP(120 - PLAYER_W / 2);
    player.y = INT_TO_FP(90);
    player.vy = JUMP_VEL;
    player.facing = 1;

    for (int i = 0; i < MAX_PELLETS; i++) pellets[i].active = 0;

    platforms[0].x = 120 - (PLATFORM_W / 2);
    platforms[0].y = 135;
    platforms[0].type = 0;
    platforms[0].vx = 0;
    platforms[0].has_spring = 0;
    platforms[0].crumble_timer = 0;

    for (int i = 1; i < NUM_PLATFORMS; i++) spawn_platform(i, platforms[i - 1].y - 25);
}

int main(void) {
    REG_DISPCNT = 0x0404; // Back to Mode 4! Absolute Canvas Drawing
    init_sound();
    init_palettes();
    init_background_template();
    reset_game();

    while (1) {
        unsigned short keys = ~REG_KEYINPUT;

        if (!game_over) {
            // Controls
            if (keys & KEY_LEFT)  { player.x -= MOVE_SPEED; player.facing = -1; }
            if (keys & KEY_RIGHT) { player.x += MOVE_SPEED; player.facing = 1; }

            // Shoot
            static int shoot_cd = 0;
            if (shoot_cd > 0) shoot_cd--;
            if ((keys & KEY_B) && shoot_cd == 0) {
                for (int i = 0; i < MAX_PELLETS; i++) {
                    if (!pellets[i].active) {
                        pellets[i].active = 1;
                        pellets[i].x = FP_TO_INT(player.x) + (player.facing == 1 ? 22 : 2);
                        pellets[i].y = FP_TO_INT(player.y) + 10;
                        pellets[i].vy = -6;
                        snd_shoot();
                        shoot_cd = 15;
                        break;
                    }
                }
            }
            for (int i = 0; i < MAX_PELLETS; i++) {
                if (pellets[i].active) {
                    pellets[i].y += pellets[i].vy;
                    if (pellets[i].y < -8) pellets[i].active = 0;
                }
            }

            // Variable Jump & Fast Fall
            if (player.vy < 0 && !(keys & KEY_A) && !(keys & KEY_UP)) {
                if (player.vy < JUMP_DROP_VEL) player.vy = JUMP_DROP_VEL;
            }
            player.vy += (keys & KEY_DOWN) ? FAST_FALL : GRAVITY;
            player.y += player.vy;

            // Wrap
            if (FP_TO_INT(player.x) < -PLAYER_W) player.x = INT_TO_FP(SCREEN_W);
            else if (FP_TO_INT(player.x) > SCREEN_W) player.x = INT_TO_FP(-PLAYER_W);

            // Platform Logic
            for (int i = 0; i < NUM_PLATFORMS; i++) {
                if (platforms[i].type == 1) {
                    platforms[i].x += platforms[i].vx;
                    if (platforms[i].x <= 10) platforms[i].vx = 1;
                    else if (platforms[i].x >= SCREEN_W - PLATFORM_W - 10) platforms[i].vx = -1;
                } else if (platforms[i].type == 2) {
                    if (platforms[i].crumble_timer > 0) {
                        platforms[i].crumble_timer--;
                        if (platforms[i].crumble_timer == 0) {
                            platforms[i].crumble_timer = -1;
                            snd_crack();
                        }
                    } else if (platforms[i].crumble_timer < 0) platforms[i].y += 4;
                }
                if (platforms[i].spring_anim > 0) platforms[i].spring_anim--;
            }

            int px = FP_TO_INT(player.x);
            int py = FP_TO_INT(player.y);

            // Collisions
            if (player.vy > 0) {
                int feet_y = py + PLAYER_H;
                for (int i = 0; i < NUM_PLATFORMS; i++) {
                    if (platforms[i].crumble_timer < 0) continue;
                    if (feet_y >= platforms[i].y && feet_y <= platforms[i].y + 10 &&
                        px + PLAYER_W - 4 >= platforms[i].x && px + 4 <= platforms[i].x + PLATFORM_W) {

                        if (platforms[i].type == 2) {
                            player.vy = JUMP_VEL;
                            if (platforms[i].crumble_timer == 0) {
                                platforms[i].crumble_timer = 90; snd_crack();
                            } else snd_jump();
                            break;
                        } else {
                            if (platforms[i].has_spring && px + PLAYER_W >= platforms[i].x + 12 && px <= platforms[i].x + 24) {
                                player.vy = SPRING_VEL;
                                platforms[i].spring_anim = 20;
                                snd_spring();
                            } else {
                                player.vy = JUMP_VEL;
                                snd_jump();
                            }
                            break;
                        }
                    }
                }
            }

            // Camera Scroll
            if (py < 65) {
                int scroll_diff = 65 - py;
                player.y = INT_TO_FP(65);
                score += scroll_diff;
                for (int i = 0; i < NUM_PLATFORMS; i++) {
                    platforms[i].y += scroll_diff;
                    if (platforms[i].y > SCREEN_H) spawn_platform(i, platforms[i].y - (NUM_PLATFORMS * (24 + score/350 > 36 ? 36 : 24 + score/350)));
                }
            }

            if (py > SCREEN_H) { game_over = 1; snd_fall(); }
        } else {
            if ((keys & KEY_START) || (keys & KEY_A)) reset_game();
        }

        // ==================================================================
        // RENDER (Mode 4 Flawless Canvas)
        // ==================================================================
        clear_screen_dma();

        for (int i = 0; i < NUM_PLATFORMS; i++) {
            draw_platform(platforms[i].x, platforms[i].y, platforms[i].type, platforms[i].crumble_timer);
            if (platforms[i].has_spring && platforms[i].crumble_timer == 0) {
                draw_spring(platforms[i].x + 14, platforms[i].y, platforms[i].spring_anim > 0);
            }
        }

        for (int i = 0; i < MAX_PELLETS; i++) {
            if (pellets[i].active) {
                int r = pellets[i].y * SCREEN_W;
                int x = pellets[i].x;
                if (x > 0 && x < SCREEN_W - 4 && pellets[i].y > 0) {
                    screen[r + x] = C_DOOD_LINE; screen[r + x + 1] = C_DOOD_LINE;
                    screen[r + SCREEN_W + x] = C_DOOD_LINE; screen[r + SCREEN_W + x + 1] = C_DOOD_LINE;
                }
            }
        }

        if (!game_over) draw_doodler(FP_TO_INT(player.x), FP_TO_INT(player.y), player.facing, player.vy);

        draw_score(32, 6, score, 2);
        if (game_over) draw_score(100, 70, score, 3);

        flip_display();
    }

    return 0;
}

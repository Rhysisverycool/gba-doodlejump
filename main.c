/* ==========================================================================
 *  DOODLE JUMP GBA (Mode 4 - Official Sprites, Sound & 1.5s Crumble Platforms)
 * ========================================================================== */

// --- Display & Input Registers ---
#define REG_DISPCNT     *(volatile unsigned short*)0x04000000
#define REG_VCOUNT      *(volatile unsigned short*)0x04000006
#define REG_KEYINPUT    *(volatile unsigned short*)0x04000130

// --- DMA 3 Registers ---
#define REG_DMA3SAD     *(volatile unsigned int*)0x040000D4
#define REG_DMA3DAD     *(volatile unsigned int*)0x040000D8
#define REG_DMA3CNT     *(volatile unsigned int*)0x040000DC

// --- GBA Hardware Sound Registers ---
#define REG_SOUNDCNT_L  *(volatile unsigned short*)0x04000080
#define REG_SOUNDCNT_H  *(volatile unsigned short*)0x04000082
#define REG_SOUNDCNT_X  *(volatile unsigned short*)0x04000084

#define REG_SOUND1CNT_L *(volatile unsigned short*)0x04000060 // Sweep
#define REG_SOUND1CNT_H *(volatile unsigned short*)0x04000062 // Duty/Envelope
#define REG_SOUND1CNT_X *(volatile unsigned short*)0x04000064 // Frequency/Trigger

#define REG_SOUND4CNT_L *(volatile unsigned short*)0x04000078 // Noise Envelope
#define REG_SOUND4CNT_H *(volatile unsigned short*)0x0400007C // Noise Freq/Trigger

#define KEY_A           (1 << 0)
#define KEY_START       (1 << 3)
#define KEY_RIGHT       (1 << 4)
#define KEY_LEFT        (1 << 5)

#define RGB15(r, g, b)  ((r) | ((g) << 5) | ((b) << 10))

#define SCREEN_W 240
#define SCREEN_H 160

#define screen       ((unsigned char*)0x02000000)
#define bg_template  ((unsigned char*)0x0200A000)
#define PALETTE_MEM  ((volatile unsigned short*)0x05000000)

// --- Palette Indices ---
#define C_TRANS        0
#define C_PAPER        1
#define C_BLUE_GRID    2
#define C_RED_MARGIN   3
#define C_TEXT         4

#define C_DOODLE_LIME  5
#define C_DOODLE_SHADE 6
#define C_DOODLE_LINE  7
#define C_EYE_WHITE    8
#define C_EYE_BLACK    9
#define C_DOODLE_HOLE  10
#define C_DOODLE_FEET  11

#define C_PLAT_HIGH    12
#define C_PLAT_MAIN    13
#define C_PLAT_SHADOW  14
#define C_PLAT_LINE    15

#define C_BLUE_HIGH    16
#define C_BLUE_MAIN    17
#define C_BLUE_SHADOW  18

#define C_BROWN_MAIN   19
#define C_BROWN_DARK   20
#define C_BROWN_CRACK  21

#define C_SPRING_GRAY  22
#define C_SPRING_RED   23

// --- Audio Engine ---
static void init_sound(void) {
    REG_SOUNDCNT_X = 0x0080; // Master Sound Enable
    REG_SOUNDCNT_L = 0x7777; // Max DMG Volume (Left/Right)
    REG_SOUNDCNT_H = 0x0002; // DMG 100% Mix
}

static inline void snd_jump(void) {
    REG_SOUND1CNT_L = 0x0013; // Sweep up
    REG_SOUND1CNT_H = 0xB280; // 50% duty, decay
    REG_SOUND1CNT_X = 0x8520; // Trigger ~680Hz
}

static inline void snd_spring(void) {
    REG_SOUND1CNT_L = 0x0015; // Fast steep sweep
    REG_SOUND1CNT_H = 0xF380; // High volume
    REG_SOUND1CNT_X = 0x86B0; // Trigger ~1400Hz
}

static inline void snd_crack(void) {
    REG_SOUND4CNT_L = 0xB100; // Noise burst
    REG_SOUND4CNT_H = 0x8035; // Trigger crunch
}

static inline void snd_fall(void) {
    REG_SOUND1CNT_L = 0x003D; // Sweep down
    REG_SOUND1CNT_H = 0xC480; // Slow decay
    REG_SOUND1CNT_X = 0x8600; // Trigger drop
}

static void init_palette(void) {
    PALETTE_MEM[C_TRANS]        = 0x0000;
    PALETTE_MEM[C_PAPER]        = RGB15(31, 30, 26); // #FAF5D6
    PALETTE_MEM[C_BLUE_GRID]    = RGB15(23, 27, 29); // #BCD9EA
    PALETTE_MEM[C_RED_MARGIN]   = RGB15(28, 14, 15); // #E47379
    PALETTE_MEM[C_TEXT]         = RGB15(4, 5, 6);

    PALETTE_MEM[C_DOODLE_LIME]  = RGB15(23, 29, 6);  // Official Lime
    PALETTE_MEM[C_DOODLE_SHADE] = RGB15(17, 23, 4);
    PALETTE_MEM[C_DOODLE_LINE]  = RGB15(7, 11, 2);
    PALETTE_MEM[C_EYE_WHITE]    = RGB15(31, 31, 31);
    PALETTE_MEM[C_EYE_BLACK]    = RGB15(0, 0, 0);
    PALETTE_MEM[C_DOODLE_HOLE]  = RGB15(4, 6, 1);    // Dark snout cannon hole
    PALETTE_MEM[C_DOODLE_FEET]  = RGB15(6, 10, 2);

    PALETTE_MEM[C_PLAT_HIGH]    = RGB15(24, 30, 10);
    PALETTE_MEM[C_PLAT_MAIN]    = RGB15(14, 27, 5);
    PALETTE_MEM[C_PLAT_SHADOW]  = RGB15(8, 18, 3);
    PALETTE_MEM[C_PLAT_LINE]    = RGB15(2, 9, 2);

    PALETTE_MEM[C_BLUE_HIGH]    = RGB15(16, 26, 31);
    PALETTE_MEM[C_BLUE_MAIN]    = RGB15(8, 18, 28);
    PALETTE_MEM[C_BLUE_SHADOW]  = RGB15(4, 11, 19);

    PALETTE_MEM[C_BROWN_MAIN]   = RGB15(20, 13, 7);
    PALETTE_MEM[C_BROWN_DARK]   = RGB15(12, 7, 3);
    PALETTE_MEM[C_BROWN_CRACK]  = RGB15(5, 3, 1);

    PALETTE_MEM[C_SPRING_GRAY]  = RGB15(22, 22, 22);
    PALETTE_MEM[C_SPRING_RED]   = RGB15(28, 6, 6);
}

static unsigned int rng_state = 0x87654321;
static inline unsigned int rand_next(void) {
    rng_state = rng_state * 1664525u + 1013904223u;
    return (rng_state >> 16);
}

static void init_background_template(void) {
    for (int y = 0; y < SCREEN_H; y++) {
        int is_hgrid = ((y & 15) == 0);
        int row = y * SCREEN_W;
        for (int x = 0; x < SCREEN_W; x++) {
            if (x == 26) {
                bg_template[row + x] = C_RED_MARGIN;
            } else if (is_hgrid || ((x & 15) == 0)) {
                bg_template[row + x] = C_BLUE_GRID;
            } else {
                bg_template[row + x] = C_PAPER;
            }
        }
    }
}

// --- Official 18x18 Doodler with Snout Hole & 2 Feet ---
static const unsigned char doodler_map[18][18] = {
    {0,0,0,0,7,7,7,7,7,7,0,0,0,0,0,0,0,0},
    {0,0,0,7,5,5,5,5,5,5,7,0,0,0,0,0,0,0},
    {0,0,7,5,5,5,5,5,5,5,5,7,0,0,0,0,0,0},
    {0,0,7,5,5,8,8,5,8,8,5,7,0,0,0,0,0,0}, // Eyes
    {0,0,7,5,8,9,8,8,9,8,5,7,0,0,0,0,0,0}, // Pupils
    {0,0,7,5,8,9,8,8,9,8,5,7,0,0,0,0,0,0},
    {0,0,7,5,5,8,8,5,8,8,5,7,7,7,0,0,0,0},
    {0,0,7,5,5,5,5,5,5,5,5,5,5,10,7,0,0,0}, // Snout starts
    {0,0,7,5,5,5,5,5,5,5,5,5,10,10,10,7,0,0}, // Snout cannon hole!
    {0,0,7,5,5,5,5,5,5,5,5,5,10,10,10,7,0,0},
    {0,0,7,5,5,5,5,5,5,5,5,6,6,6,10,7,0,0},
    {0,0,7,5,5,5,5,5,5,5,5,7,7,7,7,0,0,0},
    {0,0,7,5,5,5,5,5,5,6,7,0,0,0,0,0,0,0},
    {0,0,0,7,5,5,5,5,5,6,7,0,0,0,0,0,0,0},
    {0,0,0,7,6,6,6,6,6,6,7,0,0,0,0,0,0,0},
    {0,0,0,0,7,7,7,7,7,7,0,0,0,0,0,0,0,0},
    {0,0,0,11,11,0,0,0,11,11,0,0,0,0,0,0,0,0}, // Exactly 2 Feet
    {0,0,0,11,11,0,0,0,11,11,0,0,0,0,0,0,0,0}
};

#define PLAT_GREEN  0
#define PLAT_BLUE   1
#define PLAT_BROWN  2

static void draw_platform(int px, int py, int type, int crumble_timer) {
    int draw_x = px;
    if (type == PLAT_BROWN && crumble_timer > 0) {
        // Shake animation while crumbling
        draw_x += ((crumble_timer & 2) ? 1 : -1);
    }

    unsigned char c_high = C_PLAT_HIGH;
    unsigned char c_main = C_PLAT_MAIN;
    unsigned char c_shad = C_PLAT_SHADOW;
    unsigned char c_line = C_PLAT_LINE;

    if (type == PLAT_BLUE) {
        c_high = C_BLUE_HIGH; c_main = C_BLUE_MAIN; c_shad = C_BLUE_SHADOW;
    } else if (type == PLAT_BROWN) {
        c_high = C_BROWN_MAIN; c_main = C_BROWN_MAIN; c_shad = C_BROWN_DARK; c_line = C_BROWN_DARK;
    }

    for (int y = 0; y < 7; y++) {
        int sy = py + y;
        if (sy < 0 || sy >= SCREEN_H) continue;
        int row = sy * SCREEN_W;

        for (int x = 0; x < 34; x++) {
            int sx = draw_x + x;
            if (sx < 0 || sx >= SCREEN_W) continue;

            // Crack lines expand during 1.5s crumble
            if (type == PLAT_BROWN) {
                if (crumble_timer > 0) {
                    if (x == 17 + (y % 3) || (crumble_timer < 45 && (x == 10 + y || x == 24 - y))) {
                        screen[row + sx] = C_BROWN_CRACK;
                        continue;
                    }
                } else if (crumble_timer < 0) {
                    // Split in half while falling
                    if (x >= 15 && x <= 18) continue;
                }
            }

            if ((y == 0 || y == 6) && (x < 2 || x > 31)) continue;
            if ((y == 1 || y == 5) && (x < 1 || x > 32)) continue;

            unsigned char c;
            if (y == 0 || y == 6 || x == 0 || x == 33 || (y == 1 && (x < 2 || x > 31)) || (y == 5 && (x < 2 || x > 31))) {
                c = c_line;
            } else if (y == 1) {
                c = c_high;
            } else if (y < 4) {
                c = c_main;
            } else {
                c = c_shad;
            }

            screen[row + sx] = c;
        }
    }
}

static void draw_spring(int px, int py, int extended) {
    int h = extended ? 8 : 5;
    for (int y = 0; y < h; y++) {
        int sy = py - h + y;
        if (sy < 0 || sy >= SCREEN_H) continue;
        int row = sy * SCREEN_W;

        for (int x = 0; x < 8; x++) {
            int sx = px + x;
            if (sx < 0 || sx >= SCREEN_W) continue;

            if (y == 0) screen[row + sx] = C_SPRING_RED;
            else if ((x == 1 || x == 6) || ((y & 1) && (x > 1 && x < 6))) screen[row + sx] = C_SPRING_GRAY;
        }
    }
}

static void draw_doodler(int px, int py, int facing, int vy) {
    int leg_offset = (vy < 0) ? -1 : 0;

    for (int y = 0; y < 18; y++) {
        int sy = py + y;
        if (sy < 0 || sy >= SCREEN_H) continue;
        int row = sy * SCREEN_W;

        int src_y = y;
        if (y >= 16 && leg_offset != 0) src_y = y + leg_offset;

        for (int x = 0; x < 18; x++) {
            int src_x = (facing == 1) ? x : (17 - x);
            unsigned char c = doodler_map[src_y][src_x];

            if (c != 0) {
                int sx = px + x;
                if (sx >= 0 && sx < SCREEN_W) {
                    screen[row + sx] = c;
                }
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

static void draw_number(int x, int y, int value, int scale, unsigned char color) {
    char buf[10];
    int len = 0;
    if (value == 0) buf[len++] = 0;
    else {
        while (value > 0 && len < 10) {
            buf[len++] = value % 10;
            value /= 10;
        }
    }

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
                            if (px < SCREEN_W) screen[row + px] = color;
                        }
                    }
                }
            }
        }
        x += (3 * scale) + (scale > 1 ? scale : 1);
    }
}

static inline void vsync(void) {
    while (REG_VCOUNT >= 160);
    while (REG_VCOUNT < 160);
}

static inline void flip_display(void) {
    vsync();
    REG_DMA3SAD = (unsigned int)screen;
    REG_DMA3DAD = 0x06000000;
    REG_DMA3CNT = 0x84000000 | (SCREEN_W * SCREEN_H / 4);
}

// --- Gameplay Mechanics ---
#define NUM_PLATFORMS 7
#define PLATFORM_W    34
#define PLATFORM_H    7
#define PLAYER_W      18
#define PLAYER_H      18

#define GRAVITY        14
#define JUMP_NORMAL   -780
#define JUMP_SPRING   -1320
#define MOVE_SPEED     350

typedef struct {
    int x, y;
    int type;          // 0: Green, 1: Blue, 2: Brown
    int vx;
    int has_spring;
    int crumble_timer; // 0 = stable, 90..1 = shaking/standing (1.5s), -1 = shattered/falling
} Platform;

typedef struct {
    int x, y, vy, facing;
} Player;

static Platform platforms[NUM_PLATFORMS];
static Player player;
static int score = 0;
static int game_over = 0;

static void spawn_platform(int i, int y_pos) {
    platforms[i].y = y_pos;
    platforms[i].x = rand_next() % (SCREEN_W - PLATFORM_W - 20) + 10;
    platforms[i].crumble_timer = 0;
    platforms[i].has_spring = 0;

    int r = rand_next() % 100;
    if (score < 400) {
        platforms[i].type = PLAT_GREEN;
        platforms[i].vx = 0;
        if ((rand_next() % 100) < 30) platforms[i].has_spring = 1;
    } else if (score < 1200) {
        if (r < 65) platforms[i].type = PLAT_GREEN;
        else {
            platforms[i].type = PLAT_BLUE;
            platforms[i].vx = (rand_next() & 1) ? 1 : -1;
        }
        if ((rand_next() % 100) < 22) platforms[i].has_spring = 1;
    } else {
        if (r < 40) platforms[i].type = PLAT_GREEN;
        else if (r < 75) {
            platforms[i].type = PLAT_BLUE;
            platforms[i].vx = (rand_next() & 1) ? 2 : -2;
        } else {
            platforms[i].type = PLAT_BROWN; // Crumbling brown platform
            platforms[i].vx = 0;
        }
        if (platforms[i].type != PLAT_BROWN && (rand_next() % 100) < 14) {
            platforms[i].has_spring = 1;
        }
    }
}

static void reset_game(void) {
    score = 0;
    game_over = 0;

    player.x = (120 - PLAYER_W / 2) << 8;
    player.y = (100) << 8;
    player.vy = JUMP_NORMAL;
    player.facing = 1;

    platforms[0].x = 120 - (PLATFORM_W / 2);
    platforms[0].y = 135;
    platforms[0].type = PLAT_GREEN;
    platforms[0].vx = 0;
    platforms[0].has_spring = 0;
    platforms[0].crumble_timer = 0;

    for (int i = 1; i < NUM_PLATFORMS; i++) {
        spawn_platform(i, platforms[i - 1].y - 25);
    }
}

int main(void) {
    REG_DISPCNT = 0x0404; // Mode 4

    init_sound();
    init_palette();
    init_background_template();
    reset_game();

    while (1) {
        unsigned short keys = ~REG_KEYINPUT;

        if (!game_over) {
            if (keys & KEY_LEFT)  { player.x -= MOVE_SPEED; player.facing = -1; }
            if (keys & KEY_RIGHT) { player.x += MOVE_SPEED; player.facing = 1; }

            // Screen Wrap
            if ((player.x >> 8) < -PLAYER_W) player.x = (SCREEN_W) << 8;
            else if ((player.x >> 8) > SCREEN_W) player.x = (-PLAYER_W) << 8;

            // Move blue platforms & handle crumbling brown platforms
            for (int i = 0; i < NUM_PLATFORMS; i++) {
                if (platforms[i].type == PLAT_BLUE) {
                    platforms[i].x += platforms[i].vx;
                    if (platforms[i].x <= 10) platforms[i].vx = 1;
                    else if (platforms[i].x >= SCREEN_W - PLATFORM_W - 10) platforms[i].vx = -1;
                } else if (platforms[i].type == PLAT_BROWN) {
                    if (platforms[i].crumble_timer > 0) {
                        platforms[i].crumble_timer--;
                        if (platforms[i].crumble_timer == 0) {
                            platforms[i].crumble_timer = -1; // 1.5s finished: Shatter!
                            snd_crack();
                        }
                    } else if (platforms[i].crumble_timer < 0) {
                        platforms[i].y += 4; // Drop shattered pieces
                    }
                }
            }

            player.vy += GRAVITY;
            player.y += player.vy;

            int px = player.x >> 8;
            int py = player.y >> 8;

            // Collision check (only when falling)
            if (player.vy > 0) {
                int feet_y = py + PLAYER_H;
                for (int i = 0; i < NUM_PLATFORMS; i++) {
                    if (platforms[i].crumble_timer < 0) continue; // Cannot land on shattered pieces

                    if (feet_y >= platforms[i].y &&
                        feet_y <= platforms[i].y + 7 &&
                        px + PLAYER_W - 4 >= platforms[i].x &&
                        px + 4 <= platforms[i].x + PLATFORM_W) {

                        if (platforms[i].type == PLAT_BROWN) {
                            // Player can bounce on it for 1.5 seconds!
                            player.vy = JUMP_NORMAL;
                            if (platforms[i].crumble_timer == 0) {
                                platforms[i].crumble_timer = 90; // 90 frames = 1.5s
                                snd_crack();
                            } else {
                                snd_jump();
                            }
                            break;
                        } else {
                            if (platforms[i].has_spring &&
                                px + PLAYER_W >= platforms[i].x + 12 &&
                                px <= platforms[i].x + 22) {
                                player.vy = JUMP_SPRING;
                                snd_spring();
                            } else {
                                player.vy = JUMP_NORMAL;
                                snd_jump();
                            }
                            break;
                        }
                    }
                }
            }

            // Camera scroll
            if (py < 65) {
                int scroll_diff = 65 - py;
                player.y = 65 << 8;
                score += scroll_diff;

                for (int i = 0; i < NUM_PLATFORMS; i++) {
                    platforms[i].y += scroll_diff;
                    if (platforms[i].y > SCREEN_H) {
                        int gap = 24 + (score / 350);
                        if (gap > 36) gap = 36;
                        spawn_platform(i, platforms[i].y - (NUM_PLATFORMS * gap));
                    }
                }
            }

            if ((player.y >> 8) > SCREEN_H) {
                game_over = 1;
                snd_fall();
            }
        } else {
            if ((keys & KEY_START) || (keys & KEY_A)) reset_game();
        }

        // ==================================================================
        // RENDER
        // ==================================================================
        REG_DMA3SAD = (unsigned int)bg_template;
        REG_DMA3DAD = (unsigned int)screen;
        REG_DMA3CNT = 0x84000000 | (SCREEN_W * SCREEN_H / 4);

        for (int i = 0; i < NUM_PLATFORMS; i++) {
            draw_platform(platforms[i].x, platforms[i].y, platforms[i].type, platforms[i].crumble_timer);
            if (platforms[i].has_spring && platforms[i].crumble_timer == 0) {
                draw_spring(platforms[i].x + 14, platforms[i].y, (player.vy == JUMP_SPRING));
            }
        }

        if (!game_over) {
            draw_doodler(player.x >> 8, player.y >> 8, player.facing, player.vy);
        }

        draw_number(32, 6, score, 2, C_TEXT);

        if (game_over) {
            draw_number(100, 70, score, 3, C_TEXT);
        }

        flip_display();
    }

    return 0;
}

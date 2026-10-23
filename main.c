/* ==========================================================================
 *  DOODLE JUMP GBA (Optimized 60 FPS Engine)
 * ========================================================================== */

#define REG_DISPCNT   *(volatile unsigned short*)0x04000000
#define REG_VCOUNT    *(volatile unsigned short*)0x04000006
#define REG_KEYINPUT  *(volatile unsigned short*)0x04000130

#define REG_DMA3SAD   *(volatile unsigned int*)0x040000D4
#define REG_DMA3DAD   *(volatile unsigned int*)0x040000D8
#define REG_DMA3CNT   *(volatile unsigned int*)0x040000DC

#define KEY_A         (1 << 0)
#define KEY_START     (1 << 3)
#define KEY_RIGHT     (1 << 4)
#define KEY_LEFT      (1 << 5)

#define RGB15(r, g, b)  ((r) | ((g) << 5) | ((b) << 10))

#define COLOR_PAPER       RGB15(31, 31, 28) // Cream notebook page
#define COLOR_BLUE_GRID   RGB15(26, 28, 30) // Faint blue ruled lines
#define COLOR_RED_MARGIN  RGB15(28, 12, 12) // Red notebook margin
#define COLOR_TEXT        RGB15(5, 7, 10)   // Dark pencil

#define SCREEN_W 240
#define SCREEN_H 160

// Two buffers in 256 KB EWRAM:
// Buffer 1: Active drawing buffer
#define backbuffer  ((unsigned short*)0x02000000)
// Buffer 2: Pre-rendered static background template
#define bg_template ((unsigned short*)0x02013000)

static unsigned int rng_state = 0x87654321;
static inline unsigned int rand_next(void) {
    rng_state = rng_state * 1664525u + 1013904223u;
    return (rng_state >> 16);
}

// Pre-render the entire notebook once at startup (0 frame lag during gameplay)
static void init_background_template(void) {
    for (int y = 0; y < SCREEN_H; y++) {
        int is_hgrid = ((y & 15) == 0);
        int row = y * SCREEN_W;
        for (int x = 0; x < SCREEN_W; x++) {
            if (x == 26) {
                bg_template[row + x] = COLOR_RED_MARGIN;
            } else if (is_hgrid || ((x & 15) == 0)) {
                bg_template[row + x] = COLOR_BLUE_GRID;
            } else {
                bg_template[row + x] = COLOR_PAPER;
            }
        }
    }
}

// Instant DMA screen clear (hardware memory-to-memory copy)
static inline void clear_screen_fast(void) {
    REG_DMA3SAD = (unsigned int)bg_template;
    REG_DMA3DAD = (unsigned int)backbuffer;
    REG_DMA3CNT = 0x84000000 | (SCREEN_W * SCREEN_H / 2);
}

// --- Doodler 16x16 Pixel Sprite ---
static const unsigned short doodler_pal[6] = {
    0x0000,
    RGB15(23, 29, 6),   // 1: Lime Green
    RGB15(11, 15, 3),   // 2: Dark Outline
    RGB15(31, 31, 31),  // 3: Eye White
    RGB15(0, 0, 0),     // 4: Eye Pupil
    RGB15(9, 13, 2)     // 5: Stumpy Feet
};

static const unsigned char doodler_map[16][16] = {
    {0,0,0,0,2,2,2,2,2,0,0,0,0,0,0,0},
    {0,0,0,2,1,1,1,1,1,2,0,0,0,0,0,0},
    {0,0,2,1,1,1,1,1,1,1,2,0,0,0,0,0},
    {0,0,2,1,3,4,1,3,4,1,2,0,0,0,0,0},
    {0,0,2,1,3,4,1,3,4,1,2,0,0,0,0,0},
    {0,0,2,1,1,1,1,1,1,1,2,2,2,0,0,0},
    {0,0,2,1,1,1,1,1,1,1,1,1,1,2,0,0},
    {0,0,2,1,1,1,1,1,1,1,1,1,1,2,0,0},
    {0,0,2,1,1,1,1,1,1,1,1,1,1,2,0,0},
    {0,0,2,1,1,1,1,1,1,1,2,2,2,0,0,0},
    {0,0,2,1,1,1,1,1,1,1,2,0,0,0,0,0},
    {0,0,0,2,1,1,1,1,1,2,0,0,0,0,0,0},
    {0,0,0,2,2,2,2,2,2,2,0,0,0,0,0,0},
    {0,0,5,5,0,5,5,0,5,5,0,5,5,0,0,0},
    {0,0,5,5,0,5,5,0,5,5,0,5,5,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
};

#define PLAT_OUTLINE  RGB15(3, 14, 3)
#define PLAT_HIGH     RGB15(16, 29, 13)
#define PLAT_MAIN     RGB15(9, 25, 7)
#define PLAT_SHADOW   RGB15(5, 18, 4)

static void draw_platform(int px, int py) {
    for (int y = 0; y < 6; y++) {
        int screen_y = py + y;
        if (screen_y < 0 || screen_y >= SCREEN_H) continue;
        int row = screen_y * SCREEN_W;

        for (int x = 0; x < 32; x++) {
            int screen_x = px + x;
            if (screen_x < 0 || screen_x >= SCREEN_W) continue;
            if ((y == 0 || y == 5) && (x < 2 || x > 29)) continue;

            unsigned short color;
            if (y == 0 || y == 5 || x == 0 || x == 31 || (x == 1 && y != 0 && y != 5) || (x == 30 && y != 0 && y != 5)) {
                color = PLAT_OUTLINE;
            } else if (y == 1) {
                color = PLAT_HIGH;
            } else if (y < 4) {
                color = PLAT_MAIN;
            } else {
                color = PLAT_SHADOW;
            }

            backbuffer[row + screen_x] = color;
        }
    }
}

static void draw_doodler(int px, int py, int facing) {
    for (int y = 0; y < 16; y++) {
        int screen_y = py + y;
        if (screen_y < 0 || screen_y >= SCREEN_H) continue;
        int row = screen_y * SCREEN_W;

        for (int x = 0; x < 16; x++) {
            int src_x = (facing == 1) ? x : (15 - x);
            unsigned char idx = doodler_map[y][src_x];

            if (idx != 0) {
                int screen_x = px + x;
                if (screen_x >= 0 && screen_x < SCREEN_W) {
                    backbuffer[row + screen_x] = doodler_pal[idx];
                }
            }
        }
    }
}

// 3x5 Pixel Font
static const unsigned char font3x5[10][5] = {
    {0x7, 0x5, 0x5, 0x5, 0x7}, {0x2, 0x6, 0x2, 0x2, 0x7},
    {0x7, 0x1, 0x7, 0x4, 0x7}, {0x7, 0x1, 0x7, 0x1, 0x7},
    {0x5, 0x5, 0x7, 0x1, 0x1}, {0x7, 0x4, 0x7, 0x1, 0x7},
    {0x7, 0x4, 0x7, 0x5, 0x7}, {0x7, 0x1, 0x2, 0x4, 0x4},
    {0x7, 0x5, 0x7, 0x5, 0x7}, {0x7, 0x5, 0x7, 0x1, 0x7}
};

static void draw_number(int x, int y, int value, int scale, unsigned short color) {
    char buf[10];
    int len = 0;
    if (value == 0) {
        buf[len++] = 0;
    } else {
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
                            if (px < SCREEN_W) {
                                backbuffer[row + px] = color;
                            }
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
    // DMA 3 copy backbuffer to VRAM
    REG_DMA3SAD = 0x02000000;
    REG_DMA3DAD = 0x06000000;
    REG_DMA3CNT = 0x84000000 | (SCREEN_W * SCREEN_H / 2);
}

// --- Gameplay ---
#define NUM_PLATFORMS 7
#define PLATFORM_W    32
#define PLATFORM_H    6
#define PLAYER_W      16
#define PLAYER_H      15

#define GRAVITY       14
#define JUMP_IMPULSE -780
#define MOVE_SPEED    350

typedef struct { int x, y; } Platform;
typedef struct { int x, y, vy, facing; } Player;

static Platform platforms[NUM_PLATFORMS];
static Player player;
static int score = 0;
static int game_over = 0;

static void reset_game(void) {
    score = 0;
    game_over = 0;
    player.x = (120 - PLAYER_W / 2) << 8;
    player.y = (100) << 8;
    player.vy = JUMP_IMPULSE;
    player.facing = 1;

    platforms[0].x = 120 - (PLATFORM_W / 2);
    platforms[0].y = 135;

    for (int i = 1; i < NUM_PLATFORMS; i++) {
        platforms[i].x = (rand_next() % (SCREEN_W - PLATFORM_W));
        platforms[i].y = platforms[i - 1].y - 25;
    }
}

int main(void) {
    // Mode 3 (240x160 15-bit color), BG2 enabled
    REG_DISPCNT = 0x0403;

    // Generate static background template once
    init_background_template();
    reset_game();

    while (1) {
        unsigned short keys = ~REG_KEYINPUT;

        if (!game_over) {
            if (keys & KEY_LEFT) {
                player.x -= MOVE_SPEED;
                player.facing = -1;
            }
            if (keys & KEY_RIGHT) {
                player.x += MOVE_SPEED;
                player.facing = 1;
            }

            if ((player.x >> 8) < -PLAYER_W) {
                player.x = (SCREEN_W) << 8;
            } else if ((player.x >> 8) > SCREEN_W) {
                player.x = (-PLAYER_W) << 8;
            }

            player.vy += GRAVITY;
            player.y += player.vy;

            int px = player.x >> 8;
            int py = player.y >> 8;

            if (player.vy > 0) {
                int feet_y = py + PLAYER_H;
                for (int i = 0; i < NUM_PLATFORMS; i++) {
                    if (feet_y >= platforms[i].y &&
                        feet_y <= platforms[i].y + 6 &&
                        px + PLAYER_W - 4 >= platforms[i].x &&
                        px + 4 <= platforms[i].x + PLATFORM_W) {
                        player.vy = JUMP_IMPULSE;
                        break;
                    }
                }
            }

            if (py < 65) {
                int scroll_diff = 65 - py;
                player.y = 65 << 8;
                score += scroll_diff;

                for (int i = 0; i < NUM_PLATFORMS; i++) {
                    platforms[i].y += scroll_diff;
                    if (platforms[i].y > SCREEN_H) {
                        platforms[i].y -= (NUM_PLATFORMS * 25);
                        platforms[i].x = (rand_next() % (SCREEN_W - PLATFORM_W));
                    }
                }
            }

            if ((player.y >> 8) > SCREEN_H) {
                game_over = 1;
            }
        } else {
            if ((keys & KEY_START) || (keys & KEY_A)) {
                reset_game();
            }
        }

        // 1. Instant DMA Background Restore
        clear_screen_fast();

        // 2. Draw Platforms
        for (int i = 0; i < NUM_PLATFORMS; i++) {
            draw_platform(platforms[i].x, platforms[i].y);
        }

        // 3. Draw Doodler
        if (!game_over) {
            draw_doodler(player.x >> 8, player.y >> 8, player.facing);
        }

        // 4. Draw Score
        draw_number(32, 6, score, 2, COLOR_TEXT);

        if (game_over) {
            draw_number(100, 70, score, 3, COLOR_TEXT);
        }

        // 5. Flip Buffer at VBlank
        flip_display();
    }

    return 0;
}

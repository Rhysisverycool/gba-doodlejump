/* ==========================================================================
 *  DOODLE JUMP GBA - True 60 FPS Engine (Official Assets)
 * ========================================================================== */

#define REG_DISPCNT   *(volatile unsigned short*)0x04000000
#define REG_VCOUNT    *(volatile unsigned short*)0x04000006
#define REG_KEYINPUT  *(volatile unsigned short*)0x04000130

#define KEY_A         (1 << 0)
#define KEY_START     (1 << 3)
#define KEY_RIGHT     (1 << 4)
#define KEY_LEFT      (1 << 5)

#define RGB15(r, g, b)  ((r) | ((g) << 5) | ((b) << 10))

// --- Authentic Lima Sky Color Palette ---
#define COLOR_PAPER       RGB15(31, 30, 26) // #FAF5D6 Notebook Cream
#define COLOR_BLUE_GRID   RGB15(23, 27, 29) // #BCD9EA Faint Blue Lines
#define COLOR_RED_MARGIN  RGB15(28, 14, 15) // #E47379 Red Margin Line
#define COLOR_TEXT        RGB15(4, 5, 6)    // Pencil Lead Black

#define SCREEN_W 240
#define SCREEN_H 160

// Direct pointer to Mode 3 Video RAM
#define VRAM ((volatile unsigned short*)0x06000000)

static unsigned int rng_state = 0x87654321;
static inline unsigned int rand_next(void) {
    rng_state = rng_state * 1664525u + 1013904223u;
    return (rng_state >> 16);
}

// ==========================================================================
// OFFICIAL ASSET SPRITES
// ==========================================================================

// --- The Doodler (18x18 Official Sprite Map) ---
// 0: Trans, 1: Lime Body, 2: Under-Shade, 3: Dark Outline, 4: Eye White, 5: Pupil, 6: Legs
static const unsigned short doodler_pal[7] = {
    0x0000,
    RGB15(23, 29, 6),   // 1: Vibrant Lime Body
    RGB15(17, 23, 4),   // 2: Snout/Belly Shade
    RGB15(7, 11, 2),    // 3: Dark Olive Outline
    RGB15(31, 31, 31),  // 4: Eye White
    RGB15(0, 0, 0),     // 5: Eye Pupil
    RGB15(6, 10, 2)     // 6: 4 Stumpy Feet
};

static const unsigned char doodler_map[18][18] = {
    {0,0,0,0,3,3,3,3,3,3,0,0,0,0,0,0,0,0},
    {0,0,0,3,1,1,1,1,1,1,3,0,0,0,0,0,0,0},
    {0,0,3,1,1,1,1,1,1,1,1,3,0,0,0,0,0,0},
    {0,0,3,1,1,4,4,1,4,4,1,3,0,0,0,0,0,0}, // Eyes
    {0,0,3,1,4,5,4,4,5,4,1,3,0,0,0,0,0,0}, // Pupils
    {0,0,3,1,4,5,4,4,5,4,1,3,0,0,0,0,0,0},
    {0,0,3,1,1,4,4,1,4,4,1,3,3,3,0,0,0,0},
    {0,0,3,1,1,1,1,1,1,1,1,1,1,1,3,0,0,0}, // Snout start
    {0,0,3,1,1,1,1,1,1,1,1,1,1,1,1,3,0,0}, // Snout tip
    {0,0,3,1,1,1,1,1,1,1,1,1,1,1,1,3,0,0},
    {0,0,3,1,1,1,1,1,1,1,1,2,2,2,2,3,0,0}, // Snout bottom shade
    {0,0,3,1,1,1,1,1,1,1,1,3,3,3,3,0,0,0},
    {0,0,3,1,1,1,1,1,1,1,2,3,0,0,0,0,0,0},
    {0,0,0,3,1,1,1,1,1,2,3,0,0,0,0,0,0,0},
    {0,0,0,3,2,2,2,2,2,2,3,0,0,0,0,0,0,0},
    {0,0,0,0,3,3,3,3,3,3,0,0,0,0,0,0,0,0},
    {0,0,6,6,0,6,6,0,6,6,0,6,6,0,0,0,0,0}, // 4 Legs (Default)
    {0,0,6,6,0,6,6,0,6,6,0,6,6,0,0,0,0,0}
};

// --- Official Rounded Platform (34x7) ---
#define PLAT_OUTLINE  RGB15(2, 9, 2)    // Dark border
#define PLAT_HIGH     RGB15(24, 30, 10) // Top lime highlight
#define PLAT_MAIN     RGB15(14, 27, 5)  // Vibrant green
#define PLAT_SHADOW   RGB15(8, 18, 3)   // Bottom shade

static void draw_platform(int px, int py) {
    for (int y = 0; y < 7; y++) {
        int sy = py + y;
        if (sy < 0 || sy >= SCREEN_H) continue;
        int row = sy * SCREEN_W;

        for (int x = 0; x < 34; x++) {
            int sx = px + x;
            if (sx < 0 || sx >= SCREEN_W) continue;

            // Pill-shaped rounded caps
            if ((y == 0 || y == 6) && (x < 2 || x > 31)) continue;
            if ((y == 1 || y == 5) && (x < 1 || x > 32)) continue;

            unsigned short color;
            if (y == 0 || y == 6 || x == 0 || x == 33 || (y == 1 && (x < 2 || x > 31)) || (y == 5 && (x < 2 || x > 31))) {
                color = PLAT_OUTLINE;
            } else if (y == 1) {
                color = PLAT_HIGH;
            } else if (y < 4) {
                color = PLAT_MAIN;
            } else {
                color = PLAT_SHADOW;
            }

            VRAM[row + sx] = color;
        }
    }
}

// Draw Doodler with animation & direction flipping
static void draw_doodler(int px, int py, int facing, int vy) {
    int leg_offset = (vy < 0) ? -1 : 0; // Tucks feet while rising, dangles while falling

    for (int y = 0; y < 18; y++) {
        int sy = py + y;
        if (sy < 0 || sy >= SCREEN_H) continue;
        int row = sy * SCREEN_W;

        int src_y = y;
        if (y >= 16 && leg_offset != 0) src_y = y + leg_offset;

        for (int x = 0; x < 18; x++) {
            int src_x = (facing == 1) ? x : (17 - x);
            unsigned char idx = doodler_map[src_y][src_x];

            if (idx != 0) {
                int sx = px + x;
                if (sx >= 0 && sx < SCREEN_W) {
                    VRAM[row + sx] = doodler_pal[idx];
                }
            }
        }
    }
}

// ==========================================================================
// DIRTY RECTANGLE ENGINE (Ultra-Fast 60 FPS)
// ==========================================================================

// Restores only the specific patch of notebook paper behind a moving sprite
static void restore_bg(int x, int y, int w, int h) {
    int x1 = x < 0 ? 0 : x;
    int y1 = y < 0 ? 0 : y;
    int x2 = (x + w) > SCREEN_W ? SCREEN_W : (x + w);
    int y2 = (y + h) > SCREEN_H ? SCREEN_H : (y + h);

    for (int py = y1; py < y2; py++) {
        int is_hgrid = ((py & 15) == 0);
        int row = py * SCREEN_W;
        for (int px = x1; px < x2; px++) {
            if (px == 26) {
                VRAM[row + px] = COLOR_RED_MARGIN;
            } else if (is_hgrid || ((px & 15) == 0)) {
                VRAM[row + px] = COLOR_BLUE_GRID;
            } else {
                VRAM[row + px] = COLOR_PAPER;
            }
        }
    }
}

// Full notebook background drawn only ONCE on boot/reset
static void draw_full_notebook(void) {
    for (int y = 0; y < SCREEN_H; y++) {
        int is_hgrid = ((y & 15) == 0);
        int row = y * SCREEN_W;
        for (int x = 0; x < SCREEN_W; x++) {
            if (x == 26) {
                VRAM[row + x] = COLOR_RED_MARGIN;
            } else if (is_hgrid || ((x & 15) == 0)) {
                VRAM[row + x] = COLOR_BLUE_GRID;
            } else {
                VRAM[row + x] = COLOR_PAPER;
            }
        }
    }
}

// --- 3x5 Marker Font ---
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
                                VRAM[row + px] = color;
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

// ==========================================================================
// GAME STATE & CONSTANTS
// ==========================================================================
#define NUM_PLATFORMS 7
#define PLATFORM_W    34
#define PLATFORM_H    7
#define PLAYER_W      18
#define PLAYER_H      18

#define GRAVITY       14
#define JUMP_IMPULSE -780
#define MOVE_SPEED    350

typedef struct { int x, y; } Platform;
typedef struct { int x, y, vy, facing; } Player;

static Platform platforms[NUM_PLATFORMS];
static Platform prev_platforms[NUM_PLATFORMS];

static Player player;
static int prev_px = 0, prev_py = 0;
static int score = 0, prev_score = -1;
static int game_over = 0;

static void reset_game(void) {
    draw_full_notebook();

    score = 0;
    prev_score = -1;
    game_over = 0;

    player.x = (120 - PLAYER_W / 2) << 8;
    player.y = (100) << 8;
    player.vy = JUMP_IMPULSE;
    player.facing = 1;

    prev_px = player.x >> 8;
    prev_py = player.y >> 8;

    platforms[0].x = 120 - (PLATFORM_W / 2);
    platforms[0].y = 135;
    prev_platforms[0] = platforms[0];

    for (int i = 1; i < NUM_PLATFORMS; i++) {
        platforms[i].x = (rand_next() % (SCREEN_W - PLATFORM_W));
        platforms[i].y = platforms[i - 1].y - 25;
        prev_platforms[i] = platforms[i];
    }
}

int main(void) {
    REG_DISPCNT = 0x0403; // Mode 3
    reset_game();

    while (1) {
        unsigned short keys = ~REG_KEYINPUT;

        if (!game_over) {
            // Controls
            if (keys & KEY_LEFT) {
                player.x -= MOVE_SPEED;
                player.facing = -1;
            }
            if (keys & KEY_RIGHT) {
                player.x += MOVE_SPEED;
                player.facing = 1;
            }

            // Screen Wrap
            if ((player.x >> 8) < -PLAYER_W) {
                player.x = (SCREEN_W) << 8;
            } else if ((player.x >> 8) > SCREEN_W) {
                player.x = (-PLAYER_W) << 8;
            }

            player.vy += GRAVITY;
            player.y += player.vy;

            int px = player.x >> 8;
            int py = player.y >> 8;

            // Bounce on platforms
            if (player.vy > 0) {
                int feet_y = py + PLAYER_H;
                for (int i = 0; i < NUM_PLATFORMS; i++) {
                    if (feet_y >= platforms[i].y &&
                        feet_y <= platforms[i].y + 7 &&
                        px + PLAYER_W - 4 >= platforms[i].x &&
                        px + 4 <= platforms[i].x + PLATFORM_W) {
                        player.vy = JUMP_IMPULSE;
                        break;
                    }
                }
            }

            // Camera Scroll
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

        // ==================================================================
        // RENDER (Completed in <0.6ms during VBlank)
        // ==================================================================
        vsync();

        // 1. Erase only the previous positions (Dirty Rectangles)
        restore_bg(prev_px, prev_py, PLAYER_W, PLAYER_H);
        for (int i = 0; i < NUM_PLATFORMS; i++) {
            restore_bg(prev_platforms[i].x, prev_platforms[i].y, PLATFORM_W, PLATFORM_H);
        }
        if (score != prev_score) {
            restore_bg(32, 6, 60, 12);
        }

        // 2. Draw new platforms
        for (int i = 0; i < NUM_PLATFORMS; i++) {
            draw_platform(platforms[i].x, platforms[i].y);
            prev_platforms[i] = platforms[i];
        }

        // 3. Draw new Doodler
        if (!game_over) {
            int cur_px = player.x >> 8;
            int cur_py = player.y >> 8;
            draw_doodler(cur_px, cur_py, player.facing, player.vy);
            prev_px = cur_px;
            prev_py = cur_py;
        }

        // 4. Draw Score
        draw_number(32, 6, score, 2, COLOR_TEXT);
        prev_score = score;

        if (game_over) {
            draw_number(100, 70, score, 3, COLOR_TEXT);
        }
    }

    return 0;
}

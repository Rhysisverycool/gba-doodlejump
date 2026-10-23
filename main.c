/* ==========================================================================
 *  DOODLE JUMP GBA (Mode 3, EWRAM Double Buffered)
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

#define COLOR_BG        RGB15(30, 30, 28)
#define COLOR_GRID      RGB15(27, 27, 25)
#define COLOR_DOODLE    RGB15(10, 26, 8)
#define COLOR_SNOUT     RGB15(6, 20, 6)   
#define COLOR_EYE       RGB15(0, 0, 0)
#define COLOR_PLATFORM  RGB15(4, 25, 4)
#define COLOR_PLAT_EDGE RGB15(2, 16, 2)
#define COLOR_TEXT      RGB15(6, 10, 16)

#define SCREEN_W 240
#define SCREEN_H 160

// Direct pointer to 256 KB EWRAM (prevents memory overflow)
#define backbuffer ((unsigned short*)0x02000000)

static unsigned int rng_state = 0x12345678;
static unsigned int rand_next(void) {
    rng_state = rng_state * 1664525u + 1013904223u;
    return (rng_state >> 16);
}

static inline void set_pixel(int x, int y, unsigned short color) {
    if (x >= 0 && x < SCREEN_W && y >= 0 && y < SCREEN_H) {
        backbuffer[y * SCREEN_W + x] = color;
    }
}

static void draw_rect(int x, int y, int w, int h, unsigned short color) {
    int x1 = x < 0 ? 0 : x;
    int y1 = y < 0 ? 0 : y;
    int x2 = (x + w) > SCREEN_W ? SCREEN_W : (x + w);
    int y2 = (y + h) > SCREEN_H ? SCREEN_H : (y + h);

    for (int py = y1; py < y2; py++) {
        for (int px = x1; px < x2; px++) {
            backbuffer[py * SCREEN_W + px] = color;
        }
    }
}

static void clear_screen(void) {
    for (int y = 0; y < SCREEN_H; y++) {
        int is_hgrid = (y % 16 == 0);
        for (int x = 0; x < SCREEN_W; x++) {
            if (is_hgrid || (x % 16 == 0)) {
                backbuffer[y * SCREEN_W + x] = COLOR_GRID;
            } else {
                backbuffer[y * SCREEN_W + x] = COLOR_BG;
            }
        }
    }
}

static const unsigned char font3x5[10][5] = {
    {0x7, 0x5, 0x5, 0x5, 0x7},
    {0x2, 0x6, 0x2, 0x2, 0x7},
    {0x7, 0x1, 0x7, 0x4, 0x7},
    {0x7, 0x1, 0x7, 0x1, 0x7},
    {0x5, 0x5, 0x7, 0x1, 0x1},
    {0x7, 0x4, 0x7, 0x1, 0x7},
    {0x7, 0x4, 0x7, 0x5, 0x7},
    {0x7, 0x1, 0x2, 0x4, 0x4},
    {0x7, 0x5, 0x7, 0x5, 0x7},
    {0x7, 0x5, 0x7, 0x1, 0x7}
};

static void draw_digit(int x, int y, int digit, int scale, unsigned short color) {
    if (digit < 0 || digit > 9) return;
    for (int r = 0; r < 5; r++) {
        unsigned char row = font3x5[digit][r];
        for (int c = 0; c < 3; c++) {
            if (row & (1 << (2 - c))) {
                draw_rect(x + c * scale, y + r * scale, scale, scale, color);
            }
        }
    }
}

static void draw_number(int x, int y, int value, int scale, unsigned short color) {
    char buf[10];
    int len = 0;
    if (value == 0) {
        draw_digit(x, y, 0, scale, color);
        return;
    }
    while (value > 0 && len < 10) {
        buf[len++] = value % 10;
        value /= 10;
    }
    for (int i = len - 1; i >= 0; i--) {
        draw_digit(x, y, buf[i], scale, color);
        x += (3 * scale) + scale;
    }
}

static void vsync(void) {
    while (REG_VCOUNT >= 160);
    while (REG_VCOUNT < 160);
}

static void flip_display(void) {
    vsync();
    REG_DMA3SAD = 0x02000000;
    REG_DMA3DAD = 0x06000000;
    REG_DMA3CNT = 0x84000000 | (SCREEN_W * SCREEN_H / 2);
}

#define NUM_PLATFORMS 7
#define PLATFORM_W    30
#define PLATFORM_H    5
#define PLAYER_W      10
#define PLAYER_H      12

#define GRAVITY       14
#define JUMP_IMPULSE -780
#define MOVE_SPEED    350

typedef struct {
    int x, y;
} Platform;

typedef struct {
    int x, y, vy, facing;
} Player;

static Platform platforms[NUM_PLATFORMS];
static Player player;
static int score = 0;
static int game_over = 0;

static void reset_game(void) {
    score = 0;
    game_over = 0;
    player.x = (120 - PLAYER_W / 2) << 8;
    player.y = (110) << 8;
    player.vy = JUMP_IMPULSE;
    player.facing = 1;

    platforms[0].x = 120 - (PLATFORM_W / 2);
    platforms[0].y = 135;

    for (int i = 1; i < NUM_PLATFORMS; i++) {
        platforms[i].x = (rand_next() % (SCREEN_W - PLATFORM_W));
        platforms[i].y = platforms[i - 1].y - 24;
    }
}

int main(void) {
    REG_DISPCNT = 0x0403;
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
                        px + PLAYER_W >= platforms[i].x &&
                        px <= platforms[i].x + PLATFORM_W) {
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
                        platforms[i].y -= (NUM_PLATFORMS * 24);
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

        clear_screen();

        for (int i = 0; i < NUM_PLATFORMS; i++) {
            draw_rect(platforms[i].x, platforms[i].y, PLATFORM_W, PLATFORM_H, COLOR_PLATFORM);
            draw_rect(platforms[i].x, platforms[i].y + PLATFORM_H - 1, PLATFORM_W, 1, COLOR_PLAT_EDGE);
        }

        if (!game_over) {
            int px = player.x >> 8;
            int py = player.y >> 8;

            draw_rect(px + 1, py, PLAYER_W - 2, PLAYER_H, COLOR_DOODLE);

            if (player.facing == 1) {
                draw_rect(px + PLAYER_W - 2, py + 4, 3, 3, COLOR_SNOUT);
                set_pixel(px + 6, py + 2, COLOR_EYE);
            } else {
                draw_rect(px - 1, py + 4, 3, 3, COLOR_SNOUT);
                set_pixel(px + 3, py + 2, COLOR_EYE);
            }

            set_pixel(px + 2, py + PLAYER_H, COLOR_DOODLE);
            set_pixel(px + 7, py + PLAYER_H, COLOR_DOODLE);
        }

        draw_number(8, 6, score, 2, COLOR_TEXT);

        if (game_over) {
            draw_number(108, 70, score, 3, COLOR_TEXT);
        }

        flip_display();
    }

    return 0;
}

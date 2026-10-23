/* ==========================================================================
 *  GEOMETRY DASH: STEREO MADNESS GBA
 *  Mode 4 (240x160 8-bit Paletted) - Locked 60 FPS
 * ========================================================================== */

#define REG_DISPCNT     *(volatile unsigned short*)0x04000000
#define REG_VCOUNT      *(volatile unsigned short*)0x04000006
#define REG_KEYINPUT    *(volatile unsigned short*)0x04000130

#define REG_DMA3SAD     *(volatile unsigned int*)0x040000D4
#define REG_DMA3DAD     *(volatile unsigned int*)0x040000D8
#define REG_DMA3CNT     *(volatile unsigned int*)0x040000DC

// --- GBA Hardware Audio Registers ---
#define REG_SOUNDCNT_L  *(volatile unsigned short*)0x04000080
#define REG_SOUNDCNT_H  *(volatile unsigned short*)0x04000082
#define REG_SOUNDCNT_X  *(volatile unsigned short*)0x04000084

#define REG_SOUND1CNT_L *(volatile unsigned short*)0x04000060
#define REG_SOUND1CNT_H *(volatile unsigned short*)0x04000062
#define REG_SOUND1CNT_X *(volatile unsigned short*)0x04000064

#define REG_SOUND4CNT_L *(volatile unsigned short*)0x04000078
#define REG_SOUND4CNT_H *(volatile unsigned short*)0x0400007C

#define KEY_A           (1 << 0)
#define KEY_UP          (1 << 6)
#define KEY_START       (1 << 3)

#define RGB15(r, g, b)  ((r) | ((g) << 5) | ((b) << 10))

#define SCREEN_W 240
#define SCREEN_H 160
#define FLOOR_Y  126

#define screen       ((unsigned char*)0x02000000)
#define PALETTE_MEM  ((volatile unsigned short*)0x05000000)

// 16.16 Fixed Point Math
#define FP_SHIFT        16
#define INT_TO_FP(x)    ((x) << FP_SHIFT)
#define FP_TO_INT(x)    ((x) >> FP_SHIFT)

// --- GD Cube Physics ---
#define CUBE_SIZE       14
#define HORIZ_SPEED     144179 // 2.20 px/frame (16.16 FP)
#define GD_GRAVITY      18350  // 0.28 px/frame^2
#define GD_JUMP_IMP     -340787// -5.20 px/frame (clears 3 blocks high)
#define GD_PAD_IMP      -425984// -6.50 px/frame (yellow jump pad)

// --- Palette Indices ---
#define C_TRANS         0
#define C_BG            1
#define C_BG_GRID       2
#define C_FLOOR_LINE    3
#define C_FLOOR_DARK    4
#define C_FLOOR_LIGHT   5
#define C_CUBE_YELLOW   6
#define C_CUBE_GREEN    7
#define C_CUBE_EYES     8
#define C_BLOCK_MAIN    9
#define C_BLOCK_LINE    10
#define C_SPIKE_DARK    11
#define C_SPIKE_LINE    12
#define C_PAD_YELLOW    13
#define C_WHITE         14
#define C_PROG_GREEN    15

static unsigned int bg_fill_word = 0;

static void init_palette(void) {
    PALETTE_MEM[C_TRANS]       = 0x0000;
    PALETTE_MEM[C_BG]          = RGB15(2, 2, 8);    // Deep Stereo Madness Indigo
    PALETTE_MEM[C_BG_GRID]     = RGB15(4, 5, 14);   // Faint Background Grid
    PALETTE_MEM[C_FLOOR_LINE]  = RGB15(0, 31, 31);  // Glowing Cyan Floor Border
    PALETTE_MEM[C_FLOOR_DARK]  = RGB15(0, 6, 8);    // Checkered Floor Dark
    PALETTE_MEM[C_FLOOR_LIGHT] = RGB15(0, 14, 18);  // Checkered Floor Light
    PALETTE_MEM[C_CUBE_YELLOW] = RGB15(31, 31, 0);  // Iconic Yellow Cube
    PALETTE_MEM[C_CUBE_GREEN]  = RGB15(0, 26, 8);   // Green Inner Square
    PALETTE_MEM[C_CUBE_EYES]   = RGB15(31, 31, 31); // White Eyes
    PALETTE_MEM[C_BLOCK_MAIN]  = RGB15(3, 5, 18);   // Dark Blue Blocks
    PALETTE_MEM[C_BLOCK_LINE]  = RGB15(8, 16, 31);  // Glowing Blue Block Outline
    PALETTE_MEM[C_SPIKE_DARK]  = RGB15(3, 4, 6);    // Dark Spike Body
    PALETTE_MEM[C_SPIKE_LINE]  = RGB15(31, 31, 31); // Pure White Spike Border
    PALETTE_MEM[C_PAD_YELLOW]  = RGB15(31, 26, 0);  // Jump Pad Yellow
    PALETTE_MEM[C_WHITE]       = RGB15(31, 31, 31);
    PALETTE_MEM[C_PROG_GREEN]  = RGB15(0, 31, 6);   // Bright Green Progress Fill

    bg_fill_word = (C_BG << 24) | (C_BG << 16) | (C_BG << 8) | C_BG;
}

static void init_sound(void) {
    REG_SOUNDCNT_X = 0x0080;
    REG_SOUNDCNT_L = 0x7777;
    REG_SOUNDCNT_H = 0x0002;
}

static inline void snd_jump(void) {
    REG_SOUND1CNT_L = 0x0000;
    REG_SOUND1CNT_H = 0xB180;
    REG_SOUND1CNT_X = 0x8690;
}

static inline void snd_pad(void) {
    REG_SOUND1CNT_L = 0x0013;
    REG_SOUND1CNT_H = 0xF280;
    REG_SOUND1CNT_X = 0x8720;
}

static inline void snd_crash(void) {
    REG_SOUND4CNT_L = 0xF200;
    REG_SOUND4CNT_H = 0x8032;
}

static unsigned int rng = 0x98765432;
static inline unsigned int rand_next(void) {
    rng = rng * 1664525u + 1013904223u;
    return (rng >> 16);
}

// ==========================================================================
// STEREO MADNESS LEVEL MAP
// ==========================================================================
#define T_NONE  0
#define T_SPIKE 1
#define T_BLOCK 2
#define T_PAD   3

typedef struct {
    int gx;   // Grid X (each grid block is 16 pixels wide)
    int gy;   // Grid Y (0 = on floor, 1 = 1 block high, etc.)
    int type; // T_SPIKE, T_BLOCK, T_PAD
} LevelObject;

// Exact obstacle course for the opening section of Stereo Madness
static const LevelObject level_data[] = {
    // 1. Warm-up single spikes
    { 14, 0, T_SPIKE },
    { 22, 0, T_SPIKE },

    // 2. Double spike
    { 29, 0, T_SPIKE },
    { 30, 0, T_SPIKE },

    // 3. First step-up block platform
    { 36, 0, T_BLOCK },
    { 37, 0, T_BLOCK },
    { 38, 0, T_BLOCK },

    // 4. Spike with elevated landing
    { 44, 0, T_SPIKE },
    { 45, 1, T_BLOCK },
    { 46, 1, T_BLOCK },

    // 5. Ascending / Descending staircase
    { 52, 0, T_BLOCK },
    { 53, 1, T_BLOCK },
    { 54, 2, T_BLOCK },
    { 55, 2, T_BLOCK },
    { 56, 2, T_BLOCK },
    { 57, 1, T_BLOCK },
    { 58, 0, T_BLOCK },

    // 6. Spikes on floor with elevated bridge
    { 64, 0, T_SPIKE },
    { 65, 0, T_SPIKE },
    { 70, 1, T_BLOCK },
    { 71, 1, T_BLOCK },
    { 72, 1, T_BLOCK },
    { 77, 0, T_SPIKE },
    { 78, 0, T_SPIKE },

    // 7. Yellow Jump Pad launch
    { 84, 0, T_PAD },
    { 88, 2, T_BLOCK },
    { 89, 2, T_BLOCK },
    { 90, 2, T_BLOCK },
    { 89, 0, T_SPIKE },

    // 8. The notorious TRIPLE SPIKE
    { 100, 0, T_SPIKE },
    { 101, 0, T_SPIKE },
    { 102, 0, T_SPIKE },

    // 9. Multi-tier platforms
    { 109, 0, T_BLOCK },
    { 110, 1, T_BLOCK },
    { 111, 2, T_BLOCK },
    { 112, 3, T_BLOCK },
    { 117, 2, T_BLOCK },
    { 118, 2, T_BLOCK },
    { 123, 1, T_BLOCK },
    { 124, 1, T_BLOCK },

    // 10. Floor spikes beneath gap
    { 129, 0, T_SPIKE },
    { 130, 0, T_SPIKE },

    // 11. Final high run into ship portal zone
    { 135, 1, T_BLOCK },
    { 136, 1, T_BLOCK },
    { 137, 1, T_BLOCK },
    { 138, 1, T_BLOCK },
    { 143, 0, T_PAD },
    { 147, 2, T_BLOCK },
    { 148, 2, T_BLOCK },
    { 149, 2, T_BLOCK },
    { 155, 0, T_SPIKE },
    { 156, 0, T_SPIKE },
    { 162, 0, T_BLOCK }
};

#define LEVEL_COUNT (sizeof(level_data) / sizeof(LevelObject))
#define LEVEL_END_PX (175 * 16)

// ==========================================================================
// RENDER ENGINE
// ==========================================================================

static inline void vsync(void) {
    while (REG_VCOUNT >= 160);
    while (REG_VCOUNT < 160);
}

static inline void clear_screen_dma(void) {
    REG_DMA3SAD = (unsigned int)&bg_fill_word;
    REG_DMA3DAD = (unsigned int)screen;
    REG_DMA3CNT = 0x85000000 | (SCREEN_W * SCREEN_H / 4);
}

static inline void flip_display(void) {
    vsync();
    REG_DMA3SAD = (unsigned int)screen;
    REG_DMA3DAD = 0x06000000;
    REG_DMA3CNT = 0x84000000 | (SCREEN_W * SCREEN_H / 4);
}

static const unsigned char font3x5[10][5] = {
    {0x7, 0x5, 0x5, 0x5, 0x7}, {0x2, 0x6, 0x2, 0x2, 0x7},
    {0x7, 0x1, 0x7, 0x4, 0x7}, {0x7, 0x1, 0x7, 0x1, 0x7},
    {0x5, 0x5, 0x7, 0x1, 0x1}, {0x7, 0x4, 0x7, 0x1, 0x7},
    {0x7, 0x4, 0x7, 0x5, 0x7}, {0x7, 0x1, 0x2, 0x4, 0x4},
    {0x7, 0x5, 0x7, 0x5, 0x7}, {0x7, 0x5, 0x7, 0x1, 0x7}
};

static void draw_num(int x, int y, int val) {
    char d[4]; int len = 0;
    if (val == 0) d[len++] = 0;
    else { while (val > 0 && len < 4) { d[len++] = val % 10; val /= 10; } }
    for (int i = len - 1; i >= 0; i--) {
        int v = d[i];
        for (int r = 0; r < 5; r++) {
            unsigned char b = font3x5[v][r];
            for (int c = 0; c < 3; c++) {
                if (b & (1 << (2 - c))) screen[(y + r) * SCREEN_W + (x + c)] = C_WHITE;
            }
        }
        x += 4;
    }
}

// Draw GD Triangular Spike
static void draw_spike(int sx, int sy) {
    for (int y = 0; y < 16; y++) {
        int py = sy + y;
        if (py < 0 || py >= SCREEN_H) continue;
        int row = py * SCREEN_W;
        int half_w = (y * 7) / 15; // Pointed triangle taper
        for (int x = 8 - half_w; x <= 8 + half_w; x++) {
            int px = sx + x;
            if (px < 0 || px >= SCREEN_W) continue;
            unsigned char col = (x == 8 - half_w || x == 8 + half_w || y == 15) ? C_SPIKE_LINE : C_SPIKE_DARK;
            screen[row + px] = col;
        }
    }
}

// Draw GD Solid Block
static void draw_block(int sx, int sy) {
    for (int y = 0; y < 16; y++) {
        int py = sy + y;
        if (py < 0 || py >= SCREEN_H) continue;
        int row = py * SCREEN_W;
        for (int x = 0; x < 16; x++) {
            int px = sx + x;
            if (px < 0 || px >= SCREEN_W) continue;
            unsigned char col;
            if (y == 0 || y == 15 || x == 0 || x == 15) col = C_BLOCK_LINE;
            else if (y == 2 || y == 13 || x == 2 || x == 13) col = C_BLOCK_LINE;
            else col = C_BLOCK_MAIN;
            screen[row + px] = col;
        }
    }
}

// Draw Yellow Jump Pad
static void draw_pad(int sx, int sy) {
    int py = sy + 12;
    if (py < 0 || py >= SCREEN_H) return;
    int row = py * SCREEN_W;
    for (int y = 0; y < 4; y++) {
        for (int x = 1; x < 15; x++) {
            int px = sx + x;
            if (px >= 0 && px < SCREEN_W) screen[(py + y) * SCREEN_W + px] = C_PAD_YELLOW;
        }
    }
}

// Draw Player Cube with 8-state rotation
static void draw_cube(int px, int py, int rot_step) {
    int center_x = px + (CUBE_SIZE / 2);
    int center_y = py + (CUBE_SIZE / 2);

    for (int y = 0; y < CUBE_SIZE; y++) {
        int sy = py + y;
        if (sy < 0 || sy >= SCREEN_H) continue;
        int row = sy * SCREEN_W;

        for (int x = 0; x < CUBE_SIZE; x++) {
            int sx = px + x;
            if (sx < 0 || sx >= SCREEN_W) continue;

            // Base Cube Coloring
            unsigned char col = C_CUBE_YELLOW;
            if (x >= 2 && x <= 11 && y >= 2 && y <= 11) {
                col = C_CUBE_GREEN; // Inner green square
            }

            // Eyes that face forward based on rotation angle
            int ex1 = 4, ey1 = 4, ex2 = 9, ey2 = 4;
            if (rot_step == 1 || rot_step == 2) { ex1 = 9; ey1 = 4; ex2 = 9; ey2 = 9; }
            else if (rot_step == 3 || rot_step == 4) { ex1 = 4; ey1 = 9; ex2 = 9; ey2 = 9; }
            else if (rot_step == 5 || rot_step == 6) { ex1 = 4; ey1 = 4; ex2 = 4; ey2 = 9; }

            if ((x == ex1 && y == ey1) || (x == ex2 && y == ey2)) {
                col = C_CUBE_EYES;
            }

            screen[row + sx] = col;
        }
    }
}

// ==========================================================================
// PARTICLES & GAME STATE
// ==========================================================================
#define MAX_PARTS 16
typedef struct { int x, y, vx, vy, active; } Particle;
static Particle particles[MAX_PARTS];

typedef struct {
    int x; // 16.16 FP
    int y; // 16.16 FP
    int vy;// 16.16 FP
    int on_ground;
    int dead;
    int rot_step;
} Player;

static Player player;
static int camera_x = 0;
static int attempt_num = 1;
static int death_freeze = 0;

static void trigger_death(void) {
    player.dead = 1;
    death_freeze = 30; // 0.5s death scatter animation
    snd_crash();

    int px = FP_TO_INT(player.x) - camera_x;
    int py = FP_TO_INT(player.y);

    for (int i = 0; i < MAX_PARTS; i++) {
        particles[i].active = 1;
        particles[i].x = px + (CUBE_SIZE / 2);
        particles[i].y = py + (CUBE_SIZE / 2);
        particles[i].vx = ((rand_next() % 7) - 3);
        particles[i].vy = ((rand_next() % 7) - 4);
    }
}

static void reset_attempt(void) {
    player.x = INT_TO_FP(20);
    player.y = INT_TO_FP(FLOOR_Y - CUBE_SIZE);
    player.vy = 0;
    player.on_ground = 1;
    player.dead = 0;
    player.rot_step = 0;
    camera_x = 0;
    for (int i = 0; i < MAX_PARTS; i++) particles[i].active = 0;
}

// ==========================================================================
// MAIN LOOP
// ==========================================================================

int main(void) {
    REG_DISPCNT = 0x0404; // Mode 4

    init_palette();
    init_sound();
    reset_attempt();

    while (1) {
        unsigned short keys = ~REG_KEYINPUT;

        if (!player.dead) {
            // Jump (Press or Hold A / UP)
            if ((keys & KEY_A) || (keys & KEY_UP)) {
                if (player.on_ground) {
                    player.vy = GD_JUMP_IMP;
                    player.on_ground = 0;
                    snd_jump();
                }
            }

            // Constant Horizontal Speed
            player.x += HORIZ_SPEED;

            // Gravity & Vertical Motion
            player.vy += GD_GRAVITY;
            player.y += player.vy;

            // Mid-air Rotation
            if (!player.on_ground) {
                player.rot_step = ((FP_TO_INT(player.x) / 7) % 8);
            } else {
                player.rot_step = 0; // Snaps flat when on surface
            }

            int px = FP_TO_INT(player.x);
            int py = FP_TO_INT(player.y);

            // Camera smoothly locks to player
            camera_x = px - 40;
            if (camera_x < 0) camera_x = 0;

            // Floor Collision
            if (py >= FLOOR_Y - CUBE_SIZE) {
                player.y = INT_TO_FP(FLOOR_Y - CUBE_SIZE);
                player.vy = 0;
                player.on_ground = 1;
                py = FLOOR_Y - CUBE_SIZE;
            } else {
                player.on_ground = 0;
            }

            // Obstacle & Block Collision Checks
            for (int i = 0; i < LEVEL_COUNT; i++) {
                int ox = level_data[i].gx * 16;
                int oy = FLOOR_Y - (level_data[i].gy + 1) * 16;

                // Only test objects near the player
                if (ox < px - 32 || ox > px + 32) continue;

                if (level_data[i].type == T_SPIKE) {
                    // Forgiving inner triangle hitbox
                    if (px + CUBE_SIZE - 3 > ox + 4 && px + 3 < ox + 12 &&
                        py + CUBE_SIZE > oy + 4 && py < oy + 16) {
                        trigger_death();
                        break;
                    }
                } else if (level_data[i].type == T_BLOCK) {
                    // Top Landing Check
                    if (px + CUBE_SIZE - 2 > ox && px + 2 < ox + 16) {
                        // Landing on top
                        if (py + CUBE_SIZE >= oy && py + CUBE_SIZE <= oy + 8 && player.vy >= 0) {
                            player.y = INT_TO_FP(oy - CUBE_SIZE);
                            player.vy = 0;
                            player.on_ground = 1;
                            py = oy - CUBE_SIZE;
                        }
                        // Crash into the side or bottom of block
                        else if (py + CUBE_SIZE > oy + 8 && py < oy + 16) {
                            trigger_death();
                            break;
                        }
                    }
                } else if (level_data[i].type == T_PAD) {
                    // Yellow Jump Pad Trigger
                    if (px + CUBE_SIZE > ox + 2 && px < ox + 14 &&
                        py + CUBE_SIZE >= oy + 10 && py + CUBE_SIZE <= oy + 18) {
                        player.vy = GD_PAD_IMP;
                        player.on_ground = 0;
                        snd_pad();
                    }
                }
            }

            // Loop / Complete Level Check
            if (px >= LEVEL_END_PX) {
                reset_attempt();
            }

        } else {
            // Death freeze & particle update
            if (death_freeze > 0) {
                death_freeze--;
                for (int i = 0; i < MAX_PARTS; i++) {
                    particles[i].x += particles[i].vx;
                    particles[i].y += particles[i].vy;
                }
            } else {
                attempt_num++;
                reset_attempt();
            }
        }

        // ==================================================================
        // RENDER
        // ==================================================================
        clear_screen_dma();

        // 1. Draw Background Grid (Stereo Madness Wallpaper)
        int bg_scroll = camera_x / 3;
        for (int y = 0; y < FLOOR_Y; y += 32) {
            for (int x = 0; x < SCREEN_W; x++) screen[y * SCREEN_W + x] = C_BG_GRID;
        }
        for (int x = 0; x < SCREEN_W; x++) {
            if ((x + bg_scroll) % 32 == 0) {
                for (int y = 0; y < FLOOR_Y; y++) screen[y * SCREEN_W + x] = C_BG_GRID;
            }
        }

        // 2. Draw Checkered Ground & Glowing Border
        for (int x = 0; x < SCREEN_W; x++) screen[FLOOR_Y * SCREEN_W + x] = C_FLOOR_LINE;

        for (int y = FLOOR_Y + 1; y < SCREEN_H; y++) {
            int row = y * SCREEN_W;
            int check_y = (y / 8) & 1;
            for (int x = 0; x < SCREEN_W; x++) {
                int check_x = ((x + camera_x) / 8) & 1;
                screen[row + x] = (check_x ^ check_y) ? C_FLOOR_DARK : C_FLOOR_LIGHT;
            }
        }

        // 3. Draw Level Objects
        for (int i = 0; i < LEVEL_COUNT; i++) {
            int sx = (level_data[i].gx * 16) - camera_x;
            int sy = FLOOR_Y - (level_data[i].gy + 1) * 16;

            if (sx < -16 || sx > SCREEN_W) continue;

            if (level_data[i].type == T_SPIKE) draw_spike(sx, sy);
            else if (level_data[i].type == T_BLOCK) draw_block(sx, sy);
            else if (level_data[i].type == T_PAD) draw_pad(sx, sy);
        }

        // 4. Draw Player Cube
        if (!player.dead) {
            int draw_px = FP_TO_INT(player.x) - camera_x;
            int draw_py = FP_TO_INT(player.y);
            draw_cube(draw_px, draw_py, player.rot_step);
        } else {
            // Draw scatter explosion particles
            for (int i = 0; i < MAX_PARTS; i++) {
                if (particles[i].active) {
                    int ppx = particles[i].x;
                    int ppy = particles[i].y;
                    if (ppx >= 0 && ppx < SCREEN_W - 2 && ppy >= 0 && ppy < SCREEN_H - 2) {
                        screen[ppy * SCREEN_W + ppx] = C_CUBE_YELLOW;
                        screen[ppy * SCREEN_W + ppx + 1] = C_CUBE_YELLOW;
                        screen[(ppy + 1) * SCREEN_W + ppx] = C_CUBE_YELLOW;
                    }
                }
            }
        }

        // 5. Draw Progress Bar (0% to 100%)
        int bar_w = 120;
        int bar_x = (SCREEN_W - bar_w) / 2;
        int bar_y = 6;
        for (int bx = 0; bx < bar_w; bx++) {
            screen[bar_y * SCREEN_W + (bar_x + bx)] = C_WHITE;
            screen[(bar_y + 4) * SCREEN_W + (bar_x + bx)] = C_WHITE;
        }
        int progress_px = (FP_TO_INT(player.x) * bar_w) / LEVEL_END_PX;
        if (progress_px > bar_w) progress_px = bar_w;
        for (int by = 1; by < 4; by++) {
            for (int bx = 0; bx < progress_px; bx++) {
                screen[(bar_y + by) * SCREEN_W + (bar_x + bx)] = C_PROG_GREEN;
            }
        }

        // 6. Draw Attempt Counter (Top Left)
        draw_num(6, 6, attempt_num);

        flip_display();
    }

    return 0;
}

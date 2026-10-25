/* ==========================================================================
 *  CHROME DINO GBA - Full Color Edition
 *  Mode 4 (240x160 8-bit paletted) - Locked 60 FPS
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

#define REG_SOUND2CNT_L *(volatile unsigned short*)0x04000068
#define REG_SOUND2CNT_H *(volatile unsigned short*)0x0400006C

#define REG_SOUND4CNT_L *(volatile unsigned short*)0x04000078
#define REG_SOUND4CNT_H *(volatile unsigned short*)0x0400007C

// --- Controls ---
#define KEY_A           (1 << 0)
#define KEY_B           (1 << 1)
#define KEY_START       (1 << 3)
#define KEY_UP          (1 << 6)
#define KEY_DOWN        (1 << 7)

#define RGB15(r, g, b)  ((r) | ((g) << 5) | ((b) << 10))

#define SCREEN_W 240
#define SCREEN_H 160
#define GROUND_Y 128

#define screen       ((unsigned char*)0x02000000)
#define PALETTE_MEM  ((volatile unsigned short*)0x05000000)

// 16.16 Fixed Point Math
#define FP_SHIFT        16
#define INT_TO_FP(x)    ((x) << FP_SHIFT)
#define FP_TO_INT(x)    ((x) >> FP_SHIFT)

// ==========================================================================
// CHROMIUM 1:1 CONSTANTS
// ==========================================================================
#define SPEED_INITIAL       157286  // 2.40 px/frame
#define SPEED_MAX           340787  // 5.20 px/frame
#define SPEED_ACCEL         26      // 0.0004 px/frame (~2 min to max speed)

#define GRAVITY             16384   // 0.25 px/frame^2
#define JUMP_INITIAL_VEL    -275251 // -4.20 px/frame
#define JUMP_DROP_VEL       -137626 // -2.10 px/frame (variable jump cutoff)
#define FAST_FALL_GRAV      49152   // 3x Gravity

// Palette Indices
#define C_TRANS      0
#define C_BG         1
#define C_CLOUD      2
#define C_CELESTIAL  3
#define C_GROUND     4
#define C_PEBBLE     5
#define C_DINO       6
#define C_DINO_EYE   7
#define C_CACTUS     8
#define C_PTERO      9
#define C_TEXT       10
#define C_DEAD_EYE   11

static int is_night = 0;
static unsigned int fill_color = 0;

static void set_theme(int night) {
    is_night = night;
    PALETTE_MEM[C_TRANS] = 0x0000;
    if (night) {
        PALETTE_MEM[C_BG]        = RGB15(2, 3, 7);      // Midnight Navy Sky
        PALETTE_MEM[C_CLOUD]     = RGB15(6, 8, 14);     // Deep Night Cloud
        PALETTE_MEM[C_CELESTIAL] = RGB15(31, 31, 22);   // Pale Moon Glow
        PALETTE_MEM[C_GROUND]    = RGB15(9, 10, 16);    // Cool Twilight Sand
        PALETTE_MEM[C_PEBBLE]    = RGB15(6, 7, 12);     // Night Dune Rock
        PALETTE_MEM[C_DINO]      = RGB15(8, 28, 12);    // Neon Phosphor T-Rex
        PALETTE_MEM[C_DINO_EYE]  = RGB15(31, 31, 12);   // Glowing Eye
        PALETTE_MEM[C_CACTUS]    = RGB15(5, 16, 9);     // Dark Jade Cactus
        PALETTE_MEM[C_PTERO]     = RGB15(22, 9, 25);    // Phantom Violet
        PALETTE_MEM[C_TEXT]      = RGB15(28, 29, 31);   // Starlight White Text
        PALETTE_MEM[C_DEAD_EYE]  = RGB15(31, 3, 3);     // Red Crash Eye
    } else {
        PALETTE_MEM[C_BG]        = RGB15(16, 24, 30);   // Pastel Desert Sky Blue
        PALETTE_MEM[C_CLOUD]     = RGB15(31, 31, 31);   // Pure White Clouds
        PALETTE_MEM[C_CELESTIAL] = RGB15(31, 26, 4);    // Golden Sun
        PALETTE_MEM[C_GROUND]    = RGB15(25, 19, 10);   // Golden Desert Sand
        PALETTE_MEM[C_PEBBLE]    = RGB15(18, 13, 6);    // Sandstone Pebble
        PALETTE_MEM[C_DINO]      = RGB15(4, 21, 6);     // Emerald T-Rex Green
        PALETTE_MEM[C_DINO_EYE]  = RGB15(31, 31, 31);   // White Eye
        PALETTE_MEM[C_CACTUS]    = RGB15(3, 17, 5);     // Saguaro Green
        PALETTE_MEM[C_PTERO]     = RGB15(25, 8, 4);     // Terracotta Pterosaur
        PALETTE_MEM[C_TEXT]      = RGB15(3, 5, 8);      // Dark Charcoal Slate
        PALETTE_MEM[C_DEAD_EYE]  = RGB15(31, 3, 3);     // Red Crash Eye
    }
    fill_color = (C_BG << 24) | (C_BG << 16) | (C_BG << 8) | C_BG;
}

static void init_sound(void) {
    REG_SOUNDCNT_X = 0x0080;
    REG_SOUNDCNT_L = 0x7777;
    REG_SOUNDCNT_H = 0x0002;
}

static inline void snd_jump(void)  { REG_SOUND1CNT_L = 0x0000; REG_SOUND1CNT_H = 0xA180; REG_SOUND1CNT_X = 0x8680; }
static inline void snd_point(void) { REG_SOUND2CNT_L = 0xF180; REG_SOUND2CNT_H = 0x8740; }
static inline void snd_death(void) { REG_SOUND4CNT_L = 0xC200; REG_SOUND4CNT_H = 0x8052; }

static unsigned int rng = 0x12345678;
static inline unsigned int rand_next(void) {
    rng = rng * 1664525u + 1013904223u;
    return (rng >> 16);
}

// ==========================================================================
// SPRITE ASSETS
// ==========================================================================

// Sun (8x8 round)
static const unsigned char sun_map[8][8] = {
    {0,0,1,1,1,1,0,0},
    {0,1,1,1,1,1,1,0},
    {1,1,1,1,1,1,1,1},
    {1,1,1,1,1,1,1,1},
    {1,1,1,1,1,1,1,1},
    {1,1,1,1,1,1,1,1},
    {0,1,1,1,1,1,1,0},
    {0,0,1,1,1,1,0,0}
};

// Crescent Moon (8x8)
static const unsigned char moon_map[8][8] = {
    {0,0,0,1,1,0,0,0},
    {0,0,1,1,0,0,0,0},
    {0,1,1,0,0,0,0,0},
    {0,1,1,0,0,0,0,0},
    {0,1,1,0,0,0,0,0},
    {0,0,1,1,0,0,0,0},
    {0,0,0,1,1,0,0,0},
    {0,0,0,0,0,0,0,0}
};

static const unsigned char dino_stand[20][18] = {
    {0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,0,0}, {0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1},
    {0,0,0,0,0,0,0,0,1,1,1,1,0,1,1,1,1,1}, {0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1},
    {0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1}, {0,0,0,0,0,0,0,0,1,1,1,1,1,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,0,0}, {0,0,0,0,0,0,0,1,1,1,1,1,0,0,0,0,0,0},
    {1,0,0,0,0,0,1,1,1,1,1,1,0,0,0,0,0,0}, {1,0,0,0,0,1,1,1,1,1,1,1,0,1,0,0,0,0},
    {1,1,0,0,1,1,1,1,1,1,1,1,1,1,0,0,0,0}, {1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0},
    {0,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0}, {0,0,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0},
    {0,0,0,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0}, {0,0,0,0,1,1,1,1,1,1,0,0,0,0,0,0,0,0},
    {0,0,0,0,1,1,0,0,1,1,0,0,0,0,0,0,0,0}, {0,0,0,0,1,0,0,0,0,1,0,0,0,0,0,0,0,0},
    {0,0,0,0,1,1,0,0,0,1,1,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
};

static const unsigned char dino_duck[13][24] = {
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1}, {0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,0,1,1,1,1,1,1}, {0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1},
    {1,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0}, {1,1,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0},
    {0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0}, {0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0},
    {0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0}, {0,0,0,0,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,1,1,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0}, {0,0,0,0,0,1,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,1,1,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0}
};

static const unsigned char cactus_small[16][9] = {
    {0,0,0,1,1,1,0,0,0}, {0,0,0,1,1,1,0,0,0}, {0,1,0,1,1,1,0,0,0}, {0,1,0,1,1,1,0,1,0},
    {0,1,0,1,1,1,0,1,0}, {0,1,1,1,1,1,1,1,0}, {0,0,1,1,1,1,1,0,0}, {0,0,0,1,1,1,0,0,0},
    {0,0,0,1,1,1,0,0,0}, {0,0,0,1,1,1,0,0,0}, {0,0,0,1,1,1,0,0,0}, {0,0,0,1,1,1,0,0,0},
    {0,0,0,1,1,1,0,0,0}, {0,0,0,1,1,1,0,0,0}, {0,0,0,1,1,1,0,0,0}, {0,0,0,1,1,1,0,0,0}
};

static const unsigned char cactus_large[22][13] = {
    {0,0,0,0,0,1,1,1,0,0,0,0,0}, {0,0,0,0,0,1,1,1,0,0,0,0,0}, {0,0,1,0,0,1,1,1,0,0,0,0,0},
    {0,0,1,0,0,1,1,1,0,0,1,0,0}, {0,0,1,0,0,1,1,1,0,0,1,0,0}, {0,0,1,0,0,1,1,1,0,0,1,0,0},
    {0,0,1,1,1,1,1,1,1,1,1,0,0}, {0,0,0,1,1,1,1,1,1,1,0,0,0}, {0,0,0,0,0,1,1,1,0,0,0,0,0},
    {0,0,0,0,0,1,1,1,0,0,0,0,0}, {0,0,0,0,0,1,1,1,0,0,0,0,0}, {0,0,0,0,0,1,1,1,0,0,0,0,0},
    {0,0,0,0,0,1,1,1,0,0,0,0,0}, {0,0,0,0,0,1,1,1,0,0,0,0,0}, {0,0,0,0,0,1,1,1,0,0,0,0,0},
    {0,0,0,0,0,1,1,1,0,0,0,0,0}, {0,0,0,0,0,1,1,1,0,0,0,0,0}, {0,0,0,0,0,1,1,1,0,0,0,0,0},
    {0,0,0,0,0,1,1,1,0,0,0,0,0}, {0,0,0,0,0,1,1,1,0,0,0,0,0}, {0,0,0,0,0,1,1,1,0,0,0,0,0},
    {0,0,0,0,0,1,1,1,0,0,0,0,0}
};

static const unsigned char ptero_up[14][18] = {
    {0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,1,1,1,1,0,0,0,0,0,0,0,0},
    {1,1,0,0,0,1,1,1,1,1,1,1,1,1,1,1,0,0}, {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {0,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0}, {0,0,0,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0},
    {0,0,0,0,0,1,1,1,1,0,0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
};

static const unsigned char ptero_down[14][18] = {
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {1,1,0,0,0,0,0,0,1,1,1,1,1,1,1,1,0,0}, {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {0,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0}, {0,0,0,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0},
    {0,0,0,0,0,1,1,1,1,1,1,0,0,0,0,0,0,0}, {0,0,0,0,0,0,1,1,1,1,1,1,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,1,1,1,1,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
};

static const unsigned char cloud_map[6][16] = {
    {0,0,0,0,0,1,1,1,1,0,0,0,0,0,0,0}, {0,0,1,1,1,1,1,1,1,1,1,1,0,0,0,0},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0}, {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0}, {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
};

static const unsigned char font3x5[12][5] = {
    {0x7, 0x5, 0x5, 0x5, 0x7}, {0x2, 0x6, 0x2, 0x2, 0x7},
    {0x7, 0x1, 0x7, 0x4, 0x7}, {0x7, 0x1, 0x7, 0x1, 0x7},
    {0x5, 0x5, 0x7, 0x1, 0x1}, {0x7, 0x4, 0x7, 0x1, 0x7},
    {0x7, 0x4, 0x7, 0x5, 0x7}, {0x7, 0x1, 0x2, 0x4, 0x4},
    {0x7, 0x5, 0x7, 0x5, 0x7}, {0x7, 0x5, 0x7, 0x1, 0x7},
    {0x5, 0x5, 0x7, 0x5, 0x5}, // H
    {0x7, 0x2, 0x2, 0x2, 0x7}  // I
};

// ==========================================================================
// RENDER HELPERS
// ==========================================================================

static inline void vsync(void) {
    while (REG_VCOUNT >= 160);
    while (REG_VCOUNT < 160);
}

static inline void clear_screen_dma(void) {
    REG_DMA3SAD = (unsigned int)&fill_color;
    REG_DMA3DAD = (unsigned int)screen;
    REG_DMA3CNT = 0x85000000 | (SCREEN_W * SCREEN_H / 4);
}

static inline void flip_display(void) {
    vsync();
    REG_DMA3SAD = (unsigned int)screen;
    REG_DMA3DAD = 0x06000000;
    REG_DMA3CNT = 0x84000000 | (SCREEN_W * SCREEN_H / 4);
}

static void draw_glyph(int x, int y, int idx, unsigned char color) {
    for (int r = 0; r < 5; r++) {
        int sy = y + r;
        if (sy >= SCREEN_H) continue;
        int row = sy * SCREEN_W;
        unsigned char row_bits = font3x5[idx][r];
        for (int c = 0; c < 3; c++) {
            if (row_bits & (1 << (2 - c))) {
                int sx = x + c;
                if (sx < SCREEN_W) screen[row + sx] = color;
            }
        }
    }
}

static void draw_score(int x, int y, int score_val) {
    char digits[5];
    int temp = score_val;
    for (int i = 4; i >= 0; i--) {
        digits[i] = temp % 10;
        temp /= 10;
    }
    for (int i = 0; i < 5; i++) {
        draw_glyph(x + i * 4, y, digits[i], C_TEXT);
    }
}

// ==========================================================================
// GAME STATE
// ==========================================================================

#define OBS_CACTUS_SM  0
#define OBS_CACTUS_LG  1
#define OBS_PTERO      2

#define MAX_OBS 3
typedef struct {
    int active;
    int x; // 16.16 FP
    int y;
    int type;
    int w, h;
    int ptero_frame;
} Obstacle;

static Obstacle obstacles[MAX_OBS];

typedef struct {
    int x;
    int y;  // 16.16 FP
    int vy; // 16.16 FP
    int is_jumping;
    int is_ducking;
    int dead;
    int anim_tick;
} Dino;

static Dino dino;
static int distance_ran_fp = 0;
static int dist_since_last_spawn_fp = 0;
static int next_spawn_gap = 350;
static int score = 0;
static int high_score = 0;
static int speed_fp = SPEED_INITIAL;
static int ground_scroll = 0;
static int milestone_blink = 0;
static int game_over = 0;

#define MAX_CLOUDS 2
static int cloud_x[MAX_CLOUDS] = {120, 230};
static int cloud_y[MAX_CLOUDS] = {30, 50};

static void reset_game(void) {
    set_theme(0);

    dino.x = 24;
    dino.y = INT_TO_FP(GROUND_Y - 20);
    dino.vy = 0;
    dino.is_jumping = 0;
    dino.is_ducking = 0;
    dino.dead = 0;
    dino.anim_tick = 0;

    distance_ran_fp = 0;
    dist_since_last_spawn_fp = 0;
    next_spawn_gap = 350;

    score = 0;
    speed_fp = SPEED_INITIAL;
    ground_scroll = 0;
    milestone_blink = 0;
    game_over = 0;

    for (int i = 0; i < MAX_OBS; i++) obstacles[i].active = 0;
}

static void spawn_obstacle(void) {
    if (FP_TO_INT(dist_since_last_spawn_fp) < next_spawn_gap) {
        return;
    }

    int idx = -1;
    for (int i = 0; i < MAX_OBS; i++) {
        if (!obstacles[i].active) { idx = i; break; }
    }
    if (idx == -1) return;

    Obstacle* ob = &obstacles[idx];
    ob->active = 1;
    ob->x = INT_TO_FP(SCREEN_W);
    ob->ptero_frame = 0;

    int r = rand_next() % 100;
    if (score > 350 && r < 35) {
        ob->type = OBS_PTERO;
        ob->w = 18; ob->h = 14;
        int alt = rand_next() % 3;
        if (alt == 0)      ob->y = GROUND_Y - 14;
        else if (alt == 1) ob->y = GROUND_Y - 28;
        else               ob->y = GROUND_Y - 45;
    } else if (r < 75) {
        ob->type = OBS_CACTUS_SM;
        ob->w = 9; ob->h = 16;
        ob->y = GROUND_Y - 16;
    } else {
        ob->type = OBS_CACTUS_LG;
        ob->w = 13; ob->h = 22;
        ob->y = GROUND_Y - 22;
    }

    dist_since_last_spawn_fp = 0;
    next_spawn_gap = 220 + (rand_next() % 140);
}

static inline int check_collision(Obstacle* ob) {
    int ox = FP_TO_INT(ob->x);
    int oy = ob->y;
    int ow = ob->w;
    int oh = ob->h;

    int dx = dino.x + 3;
    int dy = FP_TO_INT(dino.y) + 2;
    int dw = dino.is_ducking ? 20 : 12;
    int dh = dino.is_ducking ? 10 : 16;

    return (dx < ox + ow - 2 && dx + dw > ox + 2 &&
            dy < oy + oh - 2 && dy + dh > oy + 2);
}

// ==========================================================================
// MAIN LOOP
// ==========================================================================

int main(void) {
    REG_DISPCNT = 0x0404;

    init_sound();
    reset_game();

    while (1) {
        unsigned short keys = ~REG_KEYINPUT;

        if (!game_over) {
            int night_phase = (score % 700) >= 500;
            if (night_phase != is_night) set_theme(night_phase);

            if ((keys & KEY_A) || (keys & KEY_UP)) {
                if (!dino.is_jumping && !dino.is_ducking) {
                    dino.vy = JUMP_INITIAL_VEL;
                    dino.is_jumping = 1;
                    snd_jump();
                }
            } else {
                if (dino.is_jumping && dino.vy < JUMP_DROP_VEL) {
                    dino.vy = JUMP_DROP_VEL;
                }
            }

            int grav = GRAVITY;
            if (keys & KEY_DOWN) {
                if (dino.is_jumping) {
                    grav = FAST_FALL_GRAV;
                } else {
                    dino.is_ducking = 1;
                }
            } else {
                dino.is_ducking = 0;
            }

            dino.vy += grav;
            dino.y += dino.vy;

            int floor_y = INT_TO_FP(dino.is_ducking ? (GROUND_Y - 13) : (GROUND_Y - 20));
            if (dino.y >= floor_y) {
                dino.y = floor_y;
                dino.vy = 0;
                dino.is_jumping = 0;
            }

            for (int i = 0; i < MAX_OBS; i++) {
                if (obstacles[i].active) {
                    obstacles[i].x -= speed_fp;
                    if (FP_TO_INT(obstacles[i].x) < -24) {
                        obstacles[i].active = 0;
                    }

                    if (obstacles[i].type == OBS_PTERO && (dino.anim_tick % 12 == 0)) {
                        obstacles[i].ptero_frame ^= 1;
                    }

                    if (check_collision(&obstacles[i])) {
                        dino.dead = 1;
                        game_over = 1;
                        snd_death();
                        if (score > high_score) high_score = score;
                    }
                }
            }

            dist_since_last_spawn_fp += speed_fp;
            spawn_obstacle();

            ground_scroll += FP_TO_INT(speed_fp);
            for (int i = 0; i < MAX_CLOUDS; i++) {
                cloud_x[i] -= 1;
                if (cloud_x[i] < -20) {
                    cloud_x[i] = SCREEN_W + (rand_next() % 40);
                    cloud_y[i] = 20 + (rand_next() % 40);
                }
            }

            if (speed_fp < SPEED_MAX) {
                speed_fp += SPEED_ACCEL;
            }

            distance_ran_fp += speed_fp;
            int prev_score = score;
            score = FP_TO_INT(distance_ran_fp) / 16;

            if (score / 100 > prev_score / 100) {
                snd_point();
                milestone_blink = 60;
            }
            if (milestone_blink > 0) milestone_blink--;

            dino.anim_tick++;

        } else {
            if ((keys & KEY_A) || (keys & KEY_START)) reset_game();
        }

        // ==================================================================
        // RENDER
        // ==================================================================
        clear_screen_dma();

        // 1. Draw Sun or Moon
        const unsigned char (*celestial)[8] = is_night ? moon_map : sun_map;
        for (int y = 0; y < 8; y++) {
            int row = (16 + y) * SCREEN_W;
            for (int x = 0; x < 8; x++) {
                if (celestial[y][x]) {
                    screen[row + (40 + x)] = C_CELESTIAL;
                }
            }
        }

        // 2. Draw Clouds
        for (int i = 0; i < MAX_CLOUDS; i++) {
            for (int y = 0; y < 6; y++) {
                int sy = cloud_y[i] + y;
                if (sy >= SCREEN_H) continue;
                int row = sy * SCREEN_W;
                for (int x = 0; x < 16; x++) {
                    if (cloud_map[y][x]) {
                        int sx = cloud_x[i] + x;
                        if (sx >= 0 && sx < SCREEN_W) screen[row + sx] = C_CLOUD;
                    }
                }
            }
        }

        // 3. Draw Desert Ground Line & Pebbles
        unsigned int g_line = (C_GROUND << 24) | (C_GROUND << 16) | (C_GROUND << 8) | C_GROUND;
        unsigned int* g_ptr = (unsigned int*)(screen + (GROUND_Y * SCREEN_W));
        for (int i = 0; i < (SCREEN_W / 4); i++) g_ptr[i] = g_line;

        int pebble_shift = ground_scroll % 40;
        int p_row = (GROUND_Y + 2) * SCREEN_W;
        for (int p = 0; p < 6; p++) {
            int px = p * 42 - pebble_shift;
            if (px >= 0 && px < SCREEN_W - 2) {
                screen[p_row + px] = C_PEBBLE;
                screen[p_row + px + 1] = C_PEBBLE;
            }
        }

        // 4. Draw Obstacles (Green Cacti / Rust Pterodactyls)
        for (int i = 0; i < MAX_OBS; i++) {
            if (!obstacles[i].active) continue;
            int ox = FP_TO_INT(obstacles[i].x);
            int oy = obstacles[i].y;

            if (obstacles[i].type == OBS_CACTUS_SM) {
                for (int y = 0; y < 16; y++) {
                    int sy = oy + y;
                    if (sy < 0 || sy >= SCREEN_H) continue;
                    int row = sy * SCREEN_W;
                    for (int x = 0; x < 9; x++) {
                        if (cactus_small[y][x]) {
                            int sx = ox + x;
                            if (sx >= 0 && sx < SCREEN_W) screen[row + sx] = C_CACTUS;
                        }
                    }
                }
            } else if (obstacles[i].type == OBS_CACTUS_LG) {
                for (int y = 0; y < 22; y++) {
                    int sy = oy + y;
                    if (sy < 0 || sy >= SCREEN_H) continue;
                    int row = sy * SCREEN_W;
                    for (int x = 0; x < 13; x++) {
                        if (cactus_large[y][x]) {
                            int sx = ox + x;
                            if (sx >= 0 && sx < SCREEN_W) screen[row + sx] = C_CACTUS;
                        }
                    }
                }
            } else if (obstacles[i].type == OBS_PTERO) {
                const unsigned char (*map)[18] = obstacles[i].ptero_frame ? ptero_up : ptero_down;
                for (int y = 0; y < 14; y++) {
                    int sy = oy + y;
                    if (sy < 0 || sy >= SCREEN_H) continue;
                    int row = sy * SCREEN_W;
                    for (int x = 0; x < 18; x++) {
                        if (map[y][x]) {
                            int sx = ox + x;
                            if (sx >= 0 && sx < SCREEN_W) screen[row + sx] = C_PTERO;
                        }
                    }
                }
            }
        }

        // 5. Draw Emerald T-Rex
        int dx = dino.x;
        int dy = FP_TO_INT(dino.y);
        int step = (dino.anim_tick / 6) & 1;

        if (dino.is_ducking && !dino.is_jumping) {
            for (int y = 0; y < 13; y++) {
                int sy = dy + y;
                if (sy < 0 || sy >= SCREEN_H) continue;
                int row = sy * SCREEN_W;
                for (int x = 0; x < 24; x++) {
                    if (dino_duck[y][x]) {
                        if (y >= 10 && ((step && x >= 10) || (!step && x < 10))) continue;
                        int sx = dx + x;
                        if (sx >= 0 && sx < SCREEN_W) screen[row + sx] = C_DINO;
                    } else if (y == 2 && x == 17) {
                        int sx = dx + x;
                        if (sx >= 0 && sx < SCREEN_W) {
                            screen[row + sx] = dino.dead ? C_DEAD_EYE : C_DINO_EYE;
                        }
                    }
                }
            }
        } else {
            for (int y = 0; y < 20; y++) {
                int sy = dy + y;
                if (sy < 0 || sy >= SCREEN_H) continue;
                int row = sy * SCREEN_W;
                for (int x = 0; x < 18; x++) {
                    if (dino_stand[y][x]) {
                        if (!dino.is_jumping && !dino.dead && y >= 17) {
                            if (step && x >= 8) continue;
                            if (!step && x < 8) continue;
                        }
                        int sx = dx + x;
                        if (sx >= 0 && sx < SCREEN_W) screen[row + sx] = C_DINO;
                    } else if (y == 2 && x == 12) {
                        int sx = dx + x;
                        if (sx >= 0 && sx < SCREEN_W) {
                            screen[row + sx] = dino.dead ? C_DEAD_EYE : C_DINO_EYE;
                        }
                    }
                }
            }
        }

        // 6. Draw Scoreboard
        draw_glyph(160, 8, 10, C_TEXT);
        draw_glyph(164, 8, 11, C_TEXT);
        draw_score(172, 8, high_score);

        if (milestone_blink == 0 || (milestone_blink & 8)) {
            draw_score(204, 8, score);
        }

        if (game_over) {
            draw_score(110, 60, score);
        }

        flip_display();
    }

    return 0;
}

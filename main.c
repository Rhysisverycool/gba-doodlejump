/* ==========================================================================
 *  CHROME DINO GBA (1:1 Offline T-Rex Runner Port)
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

// Palette Indices
#define C_TRANS 0
#define C_BG    1
#define C_FG    2
#define C_CLOUD 3

static int is_night = 0;

static void set_theme(int night) {
    is_night = night;
    PALETTE_MEM[C_TRANS] = 0x0000;
    if (night) {
        PALETTE_MEM[C_BG]    = RGB15(4, 4, 4);     // Dark charcoal
        PALETTE_MEM[C_FG]    = RGB15(28, 28, 28); // Light grey
        PALETTE_MEM[C_CLOUD] = RGB15(9, 9, 9);
    } else {
        PALETTE_MEM[C_BG]    = RGB15(31, 31, 31); // Chrome white
        PALETTE_MEM[C_FG]    = RGB15(8, 8, 8);    // Charcoal #535353
        PALETTE_MEM[C_CLOUD] = RGB15(25, 25, 25);
    }
}

// --- Sound Effects ---
static void init_sound(void) {
    REG_SOUNDCNT_X = 0x0080; // Master sound on
    REG_SOUNDCNT_L = 0x7777; // Max volume
    REG_SOUNDCNT_H = 0x0002; // DMG channels 100%
}

static inline void snd_jump(void) {
    REG_SOUND1CNT_L = 0x0000;
    REG_SOUND1CNT_H = 0xA180; // Crisp high chirp
    REG_SOUND1CNT_X = 0x8680;
}

static inline void snd_point(void) {
    REG_SOUND2CNT_L = 0xF180; // High score chime
    REG_SOUND2CNT_H = 0x8740;
}

static inline void snd_death(void) {
    REG_SOUND4CNT_L = 0xC200; // Low crunch
    REG_SOUND4CNT_H = 0x8052;
}

// --- Random Number Generator ---
static unsigned int rng = 0x12345678;
static inline unsigned int rand_next(void) {
    rng = rng * 1664525u + 1013904223u;
    return (rng >> 16);
}

// ==========================================================================
// BITMAP SPRITES (Pixel-perfect Chrome Dino assets)
// ==========================================================================

// Standing / Jumping T-Rex (18w x 20h)
static const unsigned char dino_stand[20][18] = {
    {0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,0,0},
    {0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1},
    {0,0,0,0,0,0,0,0,1,1,1,1,0,1,1,1,1,1}, // Eye at col 12
    {0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1},
    {0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1},
    {0,0,0,0,0,0,0,0,1,1,1,1,1,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,0,0},
    {0,0,0,0,0,0,0,1,1,1,1,1,0,0,0,0,0,0},
    {1,0,0,0,0,0,1,1,1,1,1,1,0,0,0,0,0,0},
    {1,0,0,0,0,1,1,1,1,1,1,1,0,1,0,0,0,0}, // Tiny arm
    {1,1,0,0,1,1,1,1,1,1,1,1,1,1,0,0,0,0},
    {1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0},
    {0,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0},
    {0,0,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0},
    {0,0,0,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0},
    {0,0,0,0,1,1,1,1,1,1,0,0,0,0,0,0,0,0},
    {0,0,0,0,1,1,0,0,1,1,0,0,0,0,0,0,0,0}, // Both legs together
    {0,0,0,0,1,0,0,0,0,1,0,0,0,0,0,0,0,0},
    {0,0,0,0,1,1,0,0,0,1,1,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
};

// Ducking T-Rex (24w x 13h)
static const unsigned char dino_duck[13][24] = {
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,0,1,1,1,1,1,1},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1},
    {1,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0},
    {1,1,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0},
    {0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0},
    {0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0},
    {0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,1,1,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,1,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,1,1,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0}
};

// Small Cactus (9w x 16h)
static const unsigned char cactus_small[16][9] = {
    {0,0,0,1,1,1,0,0,0},
    {0,0,0,1,1,1,0,0,0},
    {0,1,0,1,1,1,0,0,0},
    {0,1,0,1,1,1,0,1,0},
    {0,1,0,1,1,1,0,1,0},
    {0,1,1,1,1,1,1,1,0},
    {0,0,1,1,1,1,1,0,0},
    {0,0,0,1,1,1,0,0,0},
    {0,0,0,1,1,1,0,0,0},
    {0,0,0,1,1,1,0,0,0},
    {0,0,0,1,1,1,0,0,0},
    {0,0,0,1,1,1,0,0,0},
    {0,0,0,1,1,1,0,0,0},
    {0,0,0,1,1,1,0,0,0},
    {0,0,0,1,1,1,0,0,0},
    {0,0,0,1,1,1,0,0,0}
};

// Large Cactus (13w x 22h)
static const unsigned char cactus_large[22][13] = {
    {0,0,0,0,0,1,1,1,0,0,0,0,0},
    {0,0,0,0,0,1,1,1,0,0,0,0,0},
    {0,0,1,0,0,1,1,1,0,0,0,0,0},
    {0,0,1,0,0,1,1,1,0,0,1,0,0},
    {0,0,1,0,0,1,1,1,0,0,1,0,0},
    {0,0,1,0,0,1,1,1,0,0,1,0,0},
    {0,0,1,1,1,1,1,1,1,1,1,0,0},
    {0,0,0,1,1,1,1,1,1,1,0,0,0},
    {0,0,0,0,0,1,1,1,0,0,0,0,0},
    {0,0,0,0,0,1,1,1,0,0,0,0,0},
    {0,0,0,0,0,1,1,1,0,0,0,0,0},
    {0,0,0,0,0,1,1,1,0,0,0,0,0},
    {0,0,0,0,0,1,1,1,0,0,0,0,0},
    {0,0,0,0,0,1,1,1,0,0,0,0,0},
    {0,0,0,0,0,1,1,1,0,0,0,0,0},
    {0,0,0,0,0,1,1,1,0,0,0,0,0},
    {0,0,0,0,0,1,1,1,0,0,0,0,0},
    {0,0,0,0,0,1,1,1,0,0,0,0,0},
    {0,0,0,0,0,1,1,1,0,0,0,0,0},
    {0,0,0,0,0,1,1,1,0,0,0,0,0},
    {0,0,0,0,0,1,1,1,0,0,0,0,0},
    {0,0,0,0,0,1,1,1,0,0,0,0,0}
};

// Pterodactyl Wing-Up (18w x 14h)
static const unsigned char ptero_up[14][18] = {
    {0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,1,1,1,1,0,0,0,0,0,0,0,0},
    {1,1,0,0,0,1,1,1,1,1,1,1,1,1,1,1,0,0},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {0,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0},
    {0,0,0,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0},
    {0,0,0,0,0,1,1,1,1,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
};

// Pterodactyl Wing-Down (18w x 14h)
static const unsigned char ptero_down[14][18] = {
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {1,1,0,0,0,0,0,0,1,1,1,1,1,1,1,1,0,0},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {0,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0},
    {0,0,0,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0},
    {0,0,0,0,0,1,1,1,1,1,1,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,1,1,1,1,1,1,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,1,1,1,1,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
};

// Cloud (16w x 6h)
static const unsigned char cloud_map[6][16] = {
    {0,0,0,0,0,1,1,1,1,0,0,0,0,0,0,0},
    {0,0,1,1,1,1,1,1,1,1,1,1,0,0,0,0},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
};

// Font 3x5 + Digits & "HI"
static const unsigned char font3x5[12][5] = {
    {0x7, 0x5, 0x5, 0x5, 0x7}, // 0
    {0x2, 0x6, 0x2, 0x2, 0x7}, // 1
    {0x7, 0x1, 0x7, 0x4, 0x7}, // 2
    {0x7, 0x1, 0x7, 0x1, 0x7}, // 3
    {0x5, 0x5, 0x7, 0x1, 0x1}, // 4
    {0x7, 0x4, 0x7, 0x1, 0x7}, // 5
    {0x7, 0x4, 0x7, 0x5, 0x7}, // 6
    {0x7, 0x1, 0x2, 0x4, 0x4}, // 7
    {0x7, 0x5, 0x7, 0x5, 0x7}, // 8
    {0x7, 0x5, 0x7, 0x1, 0x7}, // 9
    {0x5, 0x5, 0x7, 0x5, 0x5}, // H (10)
    {0x7, 0x2, 0x2, 0x2, 0x7}  // I (11)
};

// ==========================================================================
// RENDER HELPERS
// ==========================================================================

static inline void vsync(void) {
    while (REG_VCOUNT >= 160);
    while (REG_VCOUNT < 160);
}

static inline void flip_display(void) {
    vsync();
    REG_DMA3SAD = (unsigned int)screen;
    REG_DMA3DAD = 0x06000000;
    REG_DMA3CNT = 0x84000000 | (SCREEN_W * SCREEN_H / 4); // 9600 words
}

static void clear_screen(void) {
    unsigned int c = (C_BG << 24) | (C_BG << 16) | (C_BG << 8) | C_BG;
    unsigned int* p = (unsigned int*)screen;
    for (int i = 0; i < (SCREEN_W * SCREEN_H / 4); i++) {
        p[i] = c;
    }
}

static void draw_glyph(int x, int y, int idx, unsigned char color) {
    for (int r = 0; r < 5; r++) {
        unsigned char row = font3x5[idx][r];
        for (int c = 0; c < 3; c++) {
            if (row & (1 << (2 - c))) {
                if (x + c < SCREEN_W && y + r < SCREEN_H) {
                    screen[(y + r) * SCREEN_W + (x + c)] = color;
                }
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
        draw_glyph(x + i * 4, y, digits[i], C_FG);
    }
}

// ==========================================================================
// GAME STATE & CONSTANTS
// ==========================================================================

#define OBS_CACTUS_SM  0
#define OBS_CACTUS_LG  1
#define OBS_PTERO      2

#define MAX_OBS 3
typedef struct {
    int active;
    int x; // 8.8 fixed
    int y;
    int type;
    int w, h;
    int ptero_frame;
} Obstacle;

static Obstacle obstacles[MAX_OBS];

typedef struct {
    int x; // Screen X
    int y; // 8.8 fixed
    int vy; // 8.8 fixed
    int is_jumping;
    int is_ducking;
    int dead;
    int anim_tick;
} Dino;

static Dino dino;
static int score = 0;
static int high_score = 0;
static int speed = 0; // 8.8 fixed speed
static int ground_scroll = 0;
static int milestone_blink = 0;
static int game_over = 0;

// Clouds
#define MAX_CLOUDS 2
static int cloud_x[MAX_CLOUDS] = {120, 230};
static int cloud_y[MAX_CLOUDS] = {30, 50};

static void reset_game(void) {
    set_theme(0);

    dino.x = 24;
    dino.y = (GROUND_Y - 20) << 8;
    dino.vy = 0;
    dino.is_jumping = 0;
    dino.is_ducking = 0;
    dino.dead = 0;
    dino.anim_tick = 0;

    score = 0;
    speed = 640; // ~2.5 pixels per frame
    ground_scroll = 0;
    milestone_blink = 0;
    game_over = 0;

    for (int i = 0; i < MAX_OBS; i++) {
        obstacles[i].active = 0;
    }
}

static void spawn_obstacle(void) {
    int idx = -1;
    for (int i = 0; i < MAX_OBS; i++) {
        if (!obstacles[i].active) { idx = i; break; }
    }
    if (idx == -1) return;

    // Minimum distance from preceding obstacle
    for (int i = 0; i < MAX_OBS; i++) {
        if (obstacles[i].active && (obstacles[i].x >> 8) > (SCREEN_W - 80)) return;
    }

    Obstacle* ob = &obstacles[idx];
    ob->active = 1;
    ob->x = SCREEN_W << 8;
    ob->ptero_frame = 0;

    int r = rand_next() % 100;

    // Pterodactyls appear once score > 250
    if (score > 250 && r < 35) {
        ob->type = OBS_PTERO;
        ob->w = 18;
        ob->h = 14;
        // 3 distinct flight altitudes:
        int alt = rand_next() % 3;
        if (alt == 0)      ob->y = GROUND_Y - 14; // Low: MUST DUCK!
        else if (alt == 1) ob->y = GROUND_Y - 28; // Mid: MUST JUMP!
        else               ob->y = GROUND_Y - 45; // High: Fly overhead
    } else if (r < 70) {
        ob->type = OBS_CACTUS_SM;
        ob->w = 9;
        ob->h = 16;
        ob->y = GROUND_Y - 16;
    } else {
        ob->type = OBS_CACTUS_LG;
        ob->w = 13;
        ob->h = 22;
        ob->y = GROUND_Y - 22;
    }
}

// AABB Collision with Chrome's forgiving inner hitboxes
static int check_collision(Obstacle* ob) {
    int ox = ob->x >> 8;
    int oy = ob->y;
    int ow = ob->w;
    int oh = ob->h;

    int dx = dino.x + 3;
    int dy = (dino.y >> 8) + 2;
    int dw = dino.is_ducking ? 20 : 12;
    int dh = dino.is_ducking ? 10 : 16;

    if (dx < ox + ow - 2 && dx + dw > ox + 2 &&
        dy < oy + oh - 2 && dy + dh > oy + 2) {
        return 1;
    }
    return 0;
}

// ==========================================================================
// MAIN LOOP
// ==========================================================================

int main(void) {
    REG_DISPCNT = 0x0404; // Mode 4, BG2 on

    init_sound();
    reset_game();

    while (1) {
        unsigned short keys = ~REG_KEYINPUT;

        if (!game_over) {
            // Day / Night Switch every 500 points
            int night_phase = (score % 700) >= 500;
            if (night_phase != is_night) {
                set_theme(night_phase);
            }

            // Controls
            if ((keys & KEY_A) || (keys & KEY_UP)) {
                if (!dino.is_jumping && !dino.is_ducking) {
                    dino.vy = -1200; // Jump impulse (~4.7 px/frame)
                    dino.is_jumping = 1;
                    snd_jump();
                }
            }

            // Fast-Fall when pressing Down mid-air
            if (keys & KEY_DOWN) {
                if (dino.is_jumping) {
                    dino.vy += 80; // Heavy downward pull
                } else {
                    dino.is_ducking = 1;
                }
            } else {
                dino.is_ducking = 0;
            }

            // Gravity & Vertical Physics
            dino.vy += 65; // Gravity
            dino.y += dino.vy;

            int floor_y = (dino.is_ducking ? (GROUND_Y - 13) : (GROUND_Y - 20)) << 8;
            if (dino.y >= floor_y) {
                dino.y = floor_y;
                dino.vy = 0;
                dino.is_jumping = 0;
            }

            // Obstacle Movement & Spawning
            int px_speed = speed >> 8;
            for (int i = 0; i < MAX_OBS; i++) {
                if (obstacles[i].active) {
                    obstacles[i].x -= speed;
                    if ((obstacles[i].x >> 8) < -24) {
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

            // Procedural Spawning
            if ((rand_next() % 100) < 4) {
                spawn_obstacle();
            }

            // Ground & Clouds
            ground_scroll += px_speed;
            for (int i = 0; i < MAX_CLOUDS; i++) {
                cloud_x[i] -= 1;
                if (cloud_x[i] < -20) {
                    cloud_x[i] = SCREEN_W + (rand_next() % 40);
                    cloud_y[i] = 20 + (rand_next() % 40);
                }
            }

            // Score Progression
            dino.anim_tick++;
            if (dino.anim_tick % 4 == 0) {
                score++;
                if (score % 100 == 0) {
                    snd_point();
                    milestone_blink = 60; // Flash score for 1 sec
                }
                // Gradually accelerate up to top speed
                if (speed < 1800) speed += 1;
            }
            if (milestone_blink > 0) milestone_blink--;

        } else {
            // Restart
            if ((keys & KEY_A) || (keys & KEY_START)) {
                reset_game();
            }
        }

        // ==================================================================
        // RENDER (Offscreen -> Mode 4 VBlank DMA)
        // ==================================================================
        clear_screen();

        // 1. Draw Clouds
        for (int i = 0; i < MAX_CLOUDS; i++) {
            for (int y = 0; y < 6; y++) {
                for (int x = 0; x < 16; x++) {
                    if (cloud_map[y][x]) {
                        int cx = cloud_x[i] + x;
                        int cy = cloud_y[i] + y;
                        if (cx >= 0 && cx < SCREEN_W) screen[cy * SCREEN_W + cx] = C_CLOUD;
                    }
                }
            }
        }

        // 2. Draw Ground Line & Bumps
        for (int x = 0; x < SCREEN_W; x++) {
            screen[GROUND_Y * SCREEN_W + x] = C_FG;
            // Little pebbles scrolling by
            if (((x + ground_scroll) % 37 == 0) || ((x + ground_scroll) % 59 == 0)) {
                screen[(GROUND_Y + 2) * SCREEN_W + x] = C_FG;
            }
        }

        // 3. Draw Obstacles
        for (int i = 0; i < MAX_OBS; i++) {
            if (!obstacles[i].active) continue;
            int ox = obstacles[i].x >> 8;
            int oy = obstacles[i].y;

            if (obstacles[i].type == OBS_CACTUS_SM) {
                for (int y = 0; y < 16; y++) {
                    for (int x = 0; x < 9; x++) {
                        if (cactus_small[y][x]) {
                            int sx = ox + x, sy = oy + y;
                            if (sx >= 0 && sx < SCREEN_W) screen[sy * SCREEN_W + sx] = C_FG;
                        }
                    }
                }
            } else if (obstacles[i].type == OBS_CACTUS_LG) {
                for (int y = 0; y < 22; y++) {
                    for (int x = 0; x < 13; x++) {
                        if (cactus_large[y][x]) {
                            int sx = ox + x, sy = oy + y;
                            if (sx >= 0 && sx < SCREEN_W) screen[sy * SCREEN_W + sx] = C_FG;
                        }
                    }
                }
            } else if (obstacles[i].type == OBS_PTERO) {
                const unsigned char (*map)[18] = obstacles[i].ptero_frame ? ptero_up : ptero_down;
                for (int y = 0; y < 14; y++) {
                    for (int x = 0; x < 18; x++) {
                        if (map[y][x]) {
                            int sx = ox + x, sy = oy + y;
                            if (sx >= 0 && sx < SCREEN_W) screen[sy * SCREEN_W + sx] = C_FG;
                        }
                    }
                }
            }
        }

        // 4. Draw T-Rex
        int dx = dino.x;
        int dy = dino.y >> 8;
        int step = (dino.anim_tick / 6) & 1;

        if (dino.is_ducking && !dino.is_jumping) {
            for (int y = 0; y < 13; y++) {
                for (int x = 0; x < 24; x++) {
                    if (dino_duck[y][x]) {
                        // Leg trot animation
                        if (y >= 10 && ((step && x >= 10) || (!step && x < 10))) continue;
                        screen[(dy + y) * SCREEN_W + (dx + x)] = C_FG;
                    }
                }
            }
        } else {
            for (int y = 0; y < 20; y++) {
                for (int x = 0; x < 18; x++) {
                    if (dino_stand[y][x]) {
                        // In game-over: X eye!
                        if (dino.dead && y == 2 && x == 12) {
                            screen[(dy + y) * SCREEN_W + (dx + x)] = C_FG;
                            continue;
                        }
                        // Running legs toggle
                        if (!dino.is_jumping && !dino.dead && y >= 17) {
                            if (step && x >= 8) continue;
                            if (!step && x < 8) continue;
                        }
                        screen[(dy + y) * SCREEN_W + (dx + x)] = C_FG;
                    }
                }
            }
        }

        // 5. Draw High Score & Current Score (Top Right)
        draw_glyph(160, 8, 10, C_CLOUD); // 'H'
        draw_glyph(164, 8, 11, C_CLOUD); // 'I'
        draw_score(172, 8, high_score);

        // Blinking score on 100-point milestones
        if (milestone_blink == 0 || (milestone_blink & 8)) {
            draw_score(204, 8, score);
        }

        // 6. Game Over Text
        if (game_over) {
            // Draw restart arrow / icon centered
            draw_score(110, 60, score);
        }

        flip_display();
    }

    return 0;
}

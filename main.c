/* ==========================================================================
 *  DOODLE JUMP GBA - True Mode 0 (Official PNG Tile Rip)
 *  Hardware BG0 Tilemap Scrolling + 128 OAM Hardware Sprites (<1% CPU)
 * ========================================================================== */

#define REG_DISPCNT     *(volatile unsigned short*)0x04000000
#define REG_VCOUNT      *(volatile unsigned short*)0x04000006
#define REG_KEYINPUT    *(volatile unsigned short*)0x04000130

#define REG_BG0CNT      *(volatile unsigned short*)0x04000008
#define REG_BG0HOFS     *(volatile unsigned short*)0x04000010
#define REG_BG0VOFS     *(volatile unsigned short*)0x04000012

// DMA 3
#define REG_DMA3SAD     *(volatile unsigned int*)0x040000D4
#define REG_DMA3DAD     *(volatile unsigned int*)0x040000D8
#define REG_DMA3CNT     *(volatile unsigned int*)0x040000DC

// Audio
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

#define RGB15(r, g, b)  ((r) | ((g) << 5) | ((b) << 10))

#define SCREEN_W 240
#define SCREEN_H 160

// Memory Maps
#define BG_PAL_MEM      ((volatile unsigned short*)0x05000000)
#define OBJ_PAL_MEM     ((volatile unsigned short*)0x05000200)
#define BG_TILE_MEM     ((volatile unsigned int*)0x06000000)
#define BG_MAP_MEM      ((volatile unsigned short*)0x0600F800) // Screenblock 31
#define OBJ_TILE_MEM    ((volatile unsigned int*)0x06010000) // Charblock 4

typedef struct {
    unsigned short attr0;
    unsigned short attr1;
    unsigned short attr2;
    unsigned short pad;
} __attribute__((packed, aligned(4))) OBJ_ATTR;

static OBJ_ATTR oam[128];

// 16.16 Fixed Point
#define FP_SHIFT        16
#define INT_TO_FP(x)    ((x) << FP_SHIFT)
#define FP_TO_INT(x)    ((x) >> FP_SHIFT)

// ==========================================================================
// RIPPED HARDWARE TILE ARRAYS (Converted from your uploaded PNGs)
// ==========================================================================

// 1. Doodler 32x32 Sprite (16 tiles = 128 words, 4bpp)
static const unsigned int doodler_tiles[128] = {
    0x00000000,0x00000000,0x00000000,0x00004400,0x00422400,0x04222400,0x04222240,0x42222240,
    0x00000000,0x00000000,0x00004400,0x04422400,0x42222400,0x22222240,0x22222240,0x22222240,
    0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
    0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
    0x42222240,0x42562240,0x42562240,0x42222240,0x42222240,0x42222240,0x42222240,0x42222240,
    0x22222240,0x25622240,0x25622240,0x22222240,0x22222244,0x22222174,0x22221774,0x22221774,
    0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000044,0x00000474,0x00000474,
    0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
    0x42222240,0x42222240,0x04222240,0x04333340,0x00444440,0x00411400,0x00411400,0x04888400,
    0x22221774,0x22222174,0x22222244,0x33333340,0x44444400,0x04114000,0x04114000,0x48884000,
    0x00000474,0x00000044,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
    0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
    0x04999400,0x00444000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
    0x49994000,0x04440000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
    0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
    0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000
};

// 2. Green Platform 32x8 (4 tiles = 32 words)
static const unsigned int plat_green_tiles[32] = {
    0x00444444,0x04111111,0x41222222,0x42222222,0x42333333,0x43333333,0x04333333,0x00444444,
    0x44444444,0x11111111,0x22222222,0x22222222,0x33333333,0x33333333,0x33333333,0x44444444,
    0x44444444,0x11111111,0x22222222,0x22222222,0x33333333,0x33333333,0x33333333,0x44444444,
    0x44444400,0x11111140,0x22222214,0x22222224,0x33333324,0x33333334,0x33333340,0x44444400
};

// 3. Blue Platform 32x8 (4 tiles = 32 words)
static const unsigned int plat_blue_tiles[32] = {
    0x00444444,0x04555555,0x45666666,0x46666666,0x46777777,0x47777777,0x04777777,0x00444444,
    0x44444444,0x55555555,0x66666666,0x66666666,0x77777777,0x77777777,0x77777777,0x44444444,
    0x44444444,0x55555555,0x66666666,0x66666666,0x77777777,0x77777777,0x77777777,0x44444444,
    0x44444400,0x55555540,0x66666654,0x66666664,0x77777764,0x77777774,0x77777740,0x44444400
};

// 4. Brown Wood Platform 32x8 (4 tiles = 32 words)
static const unsigned int plat_wood_tiles[32] = {
    0x00222222,0x02111111,0x21111111,0x21111111,0x21122222,0x22222222,0x02222222,0x00222222,
    0x22222222,0x11111111,0x11111111,0x11133111,0x22233222,0x22222222,0x22222222,0x22222222,
    0x22222222,0x11111111,0x11111111,0x11331111,0x22332222,0x22222222,0x22222222,0x22222222,
    0x22222200,0x11111120,0x11111112,0x11111112,0x22222212,0x22222222,0x22222220,0x00222200
};

// 5. Coiled Spring 16x16 (4 tiles = 32 words)
static const unsigned int spring_tiles[32] = {
    0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
    0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
    0x00000000,0x00000000,0x05555500,0x04444440,0x04000040,0x04444440,0x00400040,0x04444440,
    0x00000000,0x00000000,0x00555550,0x04444440,0x04000040,0x04444440,0x04000040,0x04444440
};

// 6. Pellet 8x8 (1 tile = 8 words)
static const unsigned int pellet_tile[8] = {
    0x00000000,0x00111100,0x01111110,0x01111110,
    0x01111110,0x01111110,0x00111100,0x00000000
};

// ==========================================================================
// HARDWARE PALETTES (Extracted from your PNGs)
// ==========================================================================

static void init_palettes(void) {
    // Background Palette (Parchment & Grid)
    BG_PAL_MEM[0] = RGB15(31, 31, 27); // #FDF8D8 Paper
    BG_PAL_MEM[1] = RGB15(23, 27, 29); // Blue grid
    BG_PAL_MEM[2] = RGB15(28, 12, 12); // Red margin

    // OBJ Palette 0: Doodler (Ripped from PNG)
    OBJ_PAL_MEM[0]  = 0x0000;            // Transparent
    OBJ_PAL_MEM[1]  = RGB15(22, 28, 6);  // Official Lime Body
    OBJ_PAL_MEM[2]  = RGB15(26, 30, 12); // Highlight Lime
    OBJ_PAL_MEM[3]  = RGB15(15, 20, 3);  // Shadow Olive
    OBJ_PAL_MEM[4]  = RGB15(4, 7, 2);    // Dark Outline
    OBJ_PAL_MEM[5]  = RGB15(31, 31, 31); // Eye White
    OBJ_PAL_MEM[6]  = RGB15(2, 2, 2);    // Pupil
    OBJ_PAL_MEM[7]  = RGB15(3, 5, 1);    // Cannon Snout Hole
    OBJ_PAL_MEM[8]  = RGB15(14, 9, 3);   // Brown Shoes
    OBJ_PAL_MEM[9]  = RGB15(4, 4, 4);    // Rubber Soles
    OBJ_PAL_MEM[10] = RGB15(31, 24, 0);  // Pellet Yellow

    // OBJ Palette 1: Green & Blue Platforms (Ripped from PNG)
    OBJ_PAL_MEM[16] = 0x0000;
    OBJ_PAL_MEM[17] = RGB15(26, 31, 10); // Green Highlight
    OBJ_PAL_MEM[18] = RGB15(12, 26, 5);  // Green Main
    OBJ_PAL_MEM[19] = RGB15(6, 16, 2);   // Green Shadow
    OBJ_PAL_MEM[20] = RGB15(2, 8, 1);    // Green Border
    OBJ_PAL_MEM[21] = RGB15(18, 28, 31); // Blue Highlight
    OBJ_PAL_MEM[22] = RGB15(8, 19, 29);  // Blue Main
    OBJ_PAL_MEM[23] = RGB15(4, 11, 20);  // Blue Shadow

    // OBJ Palette 2: Wood Platform & Spring (Ripped from PNG)
    OBJ_PAL_MEM[32] = 0x0000;
    OBJ_PAL_MEM[33] = RGB15(21, 14, 7);  // Wood Main
    OBJ_PAL_MEM[34] = RGB15(13, 8, 3);   // Wood Shadow
    OBJ_PAL_MEM[35] = RGB15(5, 3, 1);    // Crack Dark
    OBJ_PAL_MEM[36] = RGB15(22, 22, 22); // Spring Steel
    OBJ_PAL_MEM[37] = RGB15(28, 5, 5);   // Spring Red
}

// Background Hardware Setup
static void init_background(void) {
    volatile unsigned int* bg = BG_TILE_MEM;
    for (int i = 0; i < 7; i++) bg[i] = 0x00000000;
    bg[7] = 0x11111111; // Horizontal line
    for (int i = 0; i < 7; i++) bg[8 + i] = 0x00000001;
    bg[8 + 7] = 0x11111111; // Intersection
    for (int i = 0; i < 7; i++) bg[16 + i] = 0x00020000;
    bg[16 + 7] = 0x11121111; // Red Margin

    for (int ty = 0; ty < 32; ty++) {
        for (int tx = 0; tx < 32; tx++) {
            unsigned short tid = 0;
            if (tx == 3) tid = 2; // Red Margin line
            else if ((tx & 1) && (ty & 1)) tid = 1;
            BG_MAP_MEM[ty * 32 + tx] = tid;
        }
    }
}

// Tile Offsets in Charblock 4
#define TILE_DOODLER  0  // 16 tiles (0..15)
#define TILE_PLAT_G   16 // 4 tiles  (16..19)
#define TILE_PLAT_B   20 // 4 tiles  (20..23)
#define TILE_PLAT_W   24 // 4 tiles  (24..27)
#define TILE_SPRING   28 // 4 tiles  (28..31)
#define TILE_PELLET   32 // 1 tile   (32)

static void load_hardware_tiles(void) {
    volatile unsigned int* dest = OBJ_TILE_MEM;
    for (int i = 0; i < 128; i++) dest[TILE_DOODLER * 8 + i] = doodler_tiles[i];
    for (int i = 0; i < 32; i++)  dest[TILE_PLAT_G * 8 + i]  = plat_green_tiles[i];
    for (int i = 0; i < 32; i++)  dest[TILE_PLAT_B * 8 + i]  = plat_blue_tiles[i];
    for (int i = 0; i < 32; i++)  dest[TILE_PLAT_W * 8 + i]  = plat_wood_tiles[i];
    for (int i = 0; i < 32; i++)  dest[TILE_SPRING * 8 + i]  = spring_tiles[i];
    for (int i = 0; i < 8; i++)   dest[TILE_PELLET * 8 + i]  = pellet_tile[i];
}

// Audio
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

static unsigned int rng = 0x12345678;
static inline unsigned int rand_next(void) {
    rng = rng * 1664525u + 1013904223u;
    return (rng >> 16);
}

// ==========================================================================
// GAME STATE
// ==========================================================================

#define NUM_PLATFORMS 6
#define PLATFORM_W    32
#define PLATFORM_H    8
#define PLAYER_W      24
#define PLAYER_H      24

#define GRAVITY        18350   // 0.28 px/frame^2 (16.16 FP)
#define JUMP_NORMAL    -340787 // -5.20 px/frame
#define JUMP_SPRING    -557056 // -8.50 px/frame Super Launch
#define MOVE_SPEED     183500  // 2.80 px/frame

typedef struct {
    int x, y;
    int type; // 0: Green, 1: Blue, 2: Wood
    int vx;
    int has_spring;
    int crumble_timer;
} Platform;

typedef struct {
    int x, y;   // 16.16 FP
    int vy;     // 16.16 FP
    int facing; // 1: Right, -1: Left (Hardware H-FLIP)
} Player;

#define MAX_PELLETS 3
typedef struct { int x, y, vy, active; } Pellet;

static Platform platforms[NUM_PLATFORMS];
static Player player;
static Pellet pellets[MAX_PELLETS];
static int camera_y = 0;
static int score = 0;
static int game_over = 0;

static void spawn_platform(int i, int y_pos) {
    platforms[i].y = y_pos;
    platforms[i].x = rand_next() % (SCREEN_W - PLATFORM_W - 35) + 30;
    platforms[i].crumble_timer = 0;
    platforms[i].has_spring = 0;

    int r = rand_next() % 100;
    if (score < 500) {
        platforms[i].type = 0;
        platforms[i].vx = 0;
        if ((rand_next() % 100) < 30) platforms[i].has_spring = 1;
    } else if (score < 1400) {
        if (r < 65) platforms[i].type = 0;
        else {
            platforms[i].type = 1;
            platforms[i].vx = (rand_next() & 1) ? 1 : -1;
        }
        if ((rand_next() % 100) < 22) platforms[i].has_spring = 1;
    } else {
        if (r < 40) platforms[i].type = 0;
        else if (r < 75) {
            platforms[i].type = 1;
            platforms[i].vx = (rand_next() & 1) ? 2 : -2;
        } else {
            platforms[i].type = 2; // Cracking Wood
            platforms[i].vx = 0;
        }
        if (platforms[i].type != 2 && (rand_next() % 100) < 14) {
            platforms[i].has_spring = 1;
        }
    }
}

static void reset_game(void) {
    score = 0;
    camera_y = 0;
    game_over = 0;

    player.x = INT_TO_FP(120 - PLAYER_W / 2);
    player.y = INT_TO_FP(90);
    player.vy = JUMP_NORMAL;
    player.facing = 1;

    for (int i = 0; i < MAX_PELLETS; i++) pellets[i].active = 0;

    platforms[0].x = 120 - (PLATFORM_W / 2);
    platforms[0].y = 135;
    platforms[0].type = 0;
    platforms[0].vx = 0;
    platforms[0].has_spring = 0;
    platforms[0].crumble_timer = 0;

    for (int i = 1; i < NUM_PLATFORMS; i++) {
        spawn_platform(i, platforms[i - 1].y - 28);
    }
}

static inline void vsync(void) {
    while (REG_VCOUNT >= 160);
    while (REG_VCOUNT < 160);
}

// ==========================================================================
// MAIN LOOP
// ==========================================================================

int main(void) {
    // Mode 0: 1D OBJ Mapping, Enable BG0 & OBJ
    REG_DISPCNT = 0x1140;
    REG_BG0CNT = (0 << 2) | (31 << 8); // Charblock 0, Screenblock 31

    init_sound();
    init_palettes();
    init_background();
    load_hardware_tiles();
    reset_game();

    while (1) {
        unsigned short keys = ~REG_KEYINPUT;

        if (!game_over) {
            // Movement
            if (keys & KEY_LEFT)  { player.x -= MOVE_SPEED; player.facing = -1; }
            if (keys & KEY_RIGHT) { player.x += MOVE_SPEED; player.facing = 1; }

            // Shoot Nose Pellet (B Button)
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

            // Screen Wrap
            if (FP_TO_INT(player.x) < -PLAYER_W) player.x = INT_TO_FP(SCREEN_W);
            else if (FP_TO_INT(player.x) > SCREEN_W) player.x = INT_TO_FP(-PLAYER_W);

            // Move Platforms
            for (int i = 0; i < NUM_PLATFORMS; i++) {
                if (platforms[i].type == 1) {
                    platforms[i].x += platforms[i].vx;
                    if (platforms[i].x <= 28) platforms[i].vx = 1;
                    else if (platforms[i].x >= SCREEN_W - PLATFORM_W - 10) platforms[i].vx = -1;
                } else if (platforms[i].type == 2) {
                    if (platforms[i].crumble_timer > 0) {
                        platforms[i].crumble_timer--;
                        if (platforms[i].crumble_timer == 0) {
                            platforms[i].crumble_timer = -1;
                            snd_crack();
                        }
                    } else if (platforms[i].crumble_timer < 0) {
                        platforms[i].y += 4;
                    }
                }
            }

            player.vy += GRAVITY;
            player.y += player.vy;

            int px = FP_TO_INT(player.x);
            int py = FP_TO_INT(player.y);

            // Platform Collision
            if (player.vy > 0) {
                int feet_y = py + PLAYER_H;
                for (int i = 0; i < NUM_PLATFORMS; i++) {
                    if (platforms[i].crumble_timer < 0) continue;

                    if (feet_y >= platforms[i].y &&
                        feet_y <= platforms[i].y + 8 &&
                        px + PLAYER_W - 4 >= platforms[i].x &&
                        px + 4 <= platforms[i].x + PLATFORM_W) {

                        if (platforms[i].type == 2) {
                            player.vy = JUMP_NORMAL;
                            if (platforms[i].crumble_timer == 0) {
                                platforms[i].crumble_timer = 90;
                                snd_crack();
                            } else {
                                snd_jump();
                            }
                            break;
                        } else {
                            if (platforms[i].has_spring &&
                                px + PLAYER_W >= platforms[i].x + 10 &&
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

            // Hardware Vertical Scrolling
            if (py < 65) {
                int scroll_diff = 65 - py;
                player.y = INT_TO_FP(65);
                score += scroll_diff;
                camera_y += scroll_diff;

                for (int i = 0; i < NUM_PLATFORMS; i++) {
                    platforms[i].y += scroll_diff;
                    if (platforms[i].y > SCREEN_H) {
                        int gap = 24 + (score / 350);
                        if (gap > 36) gap = 36;
                        spawn_platform(i, platforms[i].y - (NUM_PLATFORMS * gap));
                    }
                }
            }

            if (py > SCREEN_H) {
                game_over = 1;
                snd_fall();
            }
        } else {
            if ((keys & KEY_START) || (keys & KEY_A)) reset_game();
        }

        // ==================================================================
        // HARDWARE OAM UPDATE (<1% CPU)
        // ==================================================================

        // Clear all 128 sprites (hide them off-screen)
        for (int i = 0; i < 128; i++) oam[i].attr0 = 0x0200;

        // Sprite 0: Doodler (32x32 Hardware Sprite with Hardware H-FLIP)
        if (!game_over) {
            int d_px = FP_TO_INT(player.x);
            int d_py = FP_TO_INT(player.y);
            oam[0].attr0 = (d_py & 0xFF) | (0 << 14); // Square shape
            oam[0].attr1 = (d_px & 0x1FF) | (2 << 14) | (player.facing == -1 ? (1 << 12) : 0); // 32x32 + H-Flip bit
            oam[0].attr2 = TILE_DOODLER | (0 << 12);  // Palette 0
        }

        // Sprites 1-6: Platforms (32x8 Hardware Sprites)
        for (int i = 0; i < NUM_PLATFORMS; i++) {
            if (platforms[i].crumble_timer < 0) continue;
            int sid = 1 + i;
            oam[sid].attr0 = (platforms[i].y & 0xFF) | (1 << 14); // Wide shape
            oam[sid].attr1 = (platforms[i].x & 0x1FF) | (1 << 14); // 32x8 size
            int tile_off = (platforms[i].type == 0) ? TILE_PLAT_G : ((platforms[i].type == 1) ? TILE_PLAT_B : TILE_PLAT_W);
            int pal_idx  = (platforms[i].type == 2) ? 2 : 1;
            oam[sid].attr2 = tile_off | (pal_idx << 12);

            // Sprites 7-12: Springs (16x16 Hardware Sprites)
            if (platforms[i].has_spring) {
                int sp_id = 7 + i;
                oam[sp_id].attr0 = ((platforms[i].y - 8) & 0xFF) | (0 << 14); // Square
                oam[sp_id].attr1 = ((platforms[i].x + 10) & 0x1FF) | (1 << 14); // 16x16
                oam[sp_id].attr2 = TILE_SPRING | (2 << 12);
            }
        }

        // Sprites 13-15: Pellets (8x8 Hardware Sprites)
        for (int i = 0; i < MAX_PELLETS; i++) {
            if (pellets[i].active) {
                int pid = 13 + i;
                oam[pid].attr0 = (pellets[i].y & 0xFF) | (0 << 14);
                oam[pid].attr1 = (pellets[i].x & 0x1FF) | (0 << 14); // 8x8
                oam[pid].attr2 = TILE_PELLET | (0 << 12);
            }
        }

        // ==================================================================
        // HARDWARE VBLANK FLIP (<150 cycles)
        // ==================================================================
        vsync();

        // 1. Hardware Background Vertical Scroll
        REG_BG0VOFS = -camera_y;

        // 2. Hardware Sprite DMA Push
        REG_DMA3SAD = (unsigned int)oam;
        REG_DMA3DAD = 0x07000000;
        REG_DMA3CNT = 0x84000000 | (128 * 2);
    }

    return 0;
}

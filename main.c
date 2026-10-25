/* ==========================================================================
 *  DOODLE JUMP GBA - True Mode 0 Hardware Engine
 *  Hardware Tilemaps (BG0) + 128 OAM Hardware Sprites (OBJ)
 * ========================================================================== */

// --- Display Registers ---
#define REG_DISPCNT     *(volatile unsigned short*)0x04000000
#define REG_VCOUNT      *(volatile unsigned short*)0x04000006
#define REG_KEYINPUT    *(volatile unsigned short*)0x04000130

#define REG_BG0CNT      *(volatile unsigned short*)0x04000008
#define REG_BG0HOFS     *(volatile unsigned short*)0x04000010
#define REG_BG0VOFS     *(volatile unsigned short*)0x04000012

// --- DMA 3 Registers ---
#define REG_DMA3SAD     *(volatile unsigned int*)0x040000D4
#define REG_DMA3DAD     *(volatile unsigned int*)0x040000D8
#define REG_DMA3CNT     *(volatile unsigned int*)0x040000DC

// --- Audio Registers ---
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

// --- Mode 0 Memory Layout ---
#define BG_PALETTE_MEM   ((volatile unsigned short*)0x05000000)
#define OBJ_PALETTE_MEM  ((volatile unsigned short*)0x05000200)

// Charblock 0: Background Tile Definitions (8x8 px, 4bpp = 32 bytes per tile)
#define BG_TILE_MEM      ((volatile unsigned int*)0x06000000)
// Screenblock 31: Background 32x32 Tilemap
#define BG_MAP_MEM       ((volatile unsigned short*)0x0600F800)

// Charblock 4: Sprite/OBJ Tile Definitions (4bpp, 1D Mapping)
#define OBJ_TILE_MEM     ((volatile unsigned int*)0x06010000)

// OAM (Object Attribute Memory): 128 Hardware Sprites
#define OAM_MEM          ((volatile unsigned short*)0x07000000)

// 16.16 Fixed Point Math
#define FP_SHIFT        16
#define INT_TO_FP(x)    ((x) << FP_SHIFT)
#define FP_TO_INT(x)    ((x) >> FP_SHIFT)

// --- OAM Attribute Structure (8 bytes per hardware sprite) ---
typedef struct {
    unsigned short attr0;
    unsigned short attr1;
    unsigned short attr2;
    unsigned short pad;
} __attribute__((packed, aligned(4))) OBJ_ATTR;

static OBJ_ATTR oam_buffer[128];

// ==========================================================================
// PALETTES & HARDWARE ASSET DATA
// ==========================================================================

static void init_palettes(void) {
    // 1. Background Palette (Parchment & Grid)
    BG_PALETTE_MEM[0] = RGB15(31, 31, 27); // #FDF8D8 Parchment Paper
    BG_PALETTE_MEM[1] = RGB15(23, 27, 29); // Faint Cyan Grid
    BG_PALETTE_MEM[2] = RGB15(28, 12, 12); // Red Margin Line
    BG_PALETTE_MEM[3] = RGB15(19, 19, 16); // Binder Punch Hole Ring

    // 2. Sprite Palette 0: The Doodler
    OBJ_PALETTE_MEM[0]  = 0x0000;            // Transparent
    OBJ_PALETTE_MEM[1]  = RGB15(22, 28, 6);  // Official Lime Body
    OBJ_PALETTE_MEM[2]  = RGB15(26, 30, 12); // Highlight Lime
    OBJ_PALETTE_MEM[3]  = RGB15(15, 20, 3);  // Shadow Olive
    OBJ_PALETTE_MEM[4]  = RGB15(5, 9, 2);    // Dark Contour
    OBJ_PALETTE_MEM[5]  = RGB15(31, 31, 31); // Eye White
    OBJ_PALETTE_MEM[6]  = RGB15(2, 2, 2);    // Pupil
    OBJ_PALETTE_MEM[7]  = RGB15(3, 5, 1);    // Cannon Snout Hole
    OBJ_PALETTE_MEM[8]  = RGB15(14, 9, 3);   // Brown Shoes
    OBJ_PALETTE_MEM[9]  = RGB15(4, 4, 4);    // Rubber Soles
    OBJ_PALETTE_MEM[10] = RGB15(31, 24, 0);  // Pellet Yellow

    // 3. Sprite Palette 1: Green & Blue Platforms
    OBJ_PALETTE_MEM[16] = 0x0000;
    OBJ_PALETTE_MEM[17] = RGB15(26, 31, 10); // Green Highlight
    OBJ_PALETTE_MEM[18] = RGB15(12, 26, 5);  // Green Main
    OBJ_PALETTE_MEM[19] = RGB15(6, 16, 2);   // Green Shadow
    OBJ_PALETTE_MEM[20] = RGB15(2, 8, 1);    // Green Border
    OBJ_PALETTE_MEM[21] = RGB15(18, 28, 31); // Blue Highlight
    OBJ_PALETTE_MEM[22] = RGB15(8, 19, 29);  // Blue Main
    OBJ_PALETTE_MEM[23] = RGB15(4, 11, 20);  // Blue Shadow

    // 4. Sprite Palette 2: Wood Platform & Spring
    OBJ_PALETTE_MEM[32] = 0x0000;
    OBJ_PALETTE_MEM[33] = RGB15(21, 14, 7);  // Wood Main
    OBJ_PALETTE_MEM[34] = RGB15(13, 8, 3);   // Wood Dark
    OBJ_PALETTE_MEM[35] = RGB15(5, 3, 1);    // Crack Dark
    OBJ_PALETTE_MEM[36] = RGB15(22, 22, 22); // Steel Spring Gray
    OBJ_PALETTE_MEM[37] = RGB15(28, 5, 5);   // Red Spring Cap
}

// ==========================================================================
// BACKGROUND HARDWARE TILES & TILEMAP
// ==========================================================================

static void init_background_hardware(void) {
    // 8x8 pixel tile in 4bpp = 32 bytes (8 words of 32 bits, 2 pixels per byte)
    // Tile 0: Standard grid dot tile
    volatile unsigned int* tile_ptr = BG_TILE_MEM;
    for (int i = 0; i < 7; i++) tile_ptr[i] = 0x00000000;
    tile_ptr[7] = 0x11111111; // Horizontal blue line on bottom pixel row

    // Tile 1: Grid intersection tile
    for (int i = 0; i < 7; i++) tile_ptr[8 + i] = 0x00000001; // Vertical blue line on right
    tile_ptr[8 + 7] = 0x11111111;

    // Tile 2: Red Margin Tile
    for (int i = 0; i < 7; i++) tile_ptr[16 + i] = 0x00020000; // Red line at pixel 2
    tile_ptr[16 + 7] = 0x11121111;

    // Generate 32x32 Screenblock Map (Screenblock 31)
    for (int ty = 0; ty < 32; ty++) {
        for (int tx = 0; tx < 32; tx++) {
            unsigned short tile_id = 0;
            if (tx == 3) {
                tile_id = 2; // Red Margin Column
            } else if ((tx & 1) && (ty & 1)) {
                tile_id = 1; // Grid intersection
            }
            BG_MAP_MEM[ty * 32 + tx] = tile_id;
        }
    }
}

// ==========================================================================
// HARDWARE SPRITE TILES (4bpp 1D Tile Mapping)
// ==========================================================================

#define TILE_DOODLER_BASE 0  // 32x32 sprite = 16 tiles (0..15)
#define TILE_PLAT_GREEN   16 // 32x8 sprite  = 4 tiles (16..19)
#define TILE_PLAT_BLUE    20 // 32x8 sprite  = 4 tiles (20..23)
#define TILE_PLAT_WOOD    24 // 32x8 sprite  = 4 tiles (24..27)
#define TILE_SPRING       28 // 16x16 sprite = 4 tiles (28..31)
#define TILE_PELLET       32 // 8x8 sprite   = 1 tile  (32)

static void init_sprite_hardware_tiles(void) {
    // 1. Build 32x8 Platform Hardware Tiles (Tiles 16-19)
    // Cap Tile (Tile 16), Mid Tiles (17, 18), End Cap Tile (19)
    volatile unsigned int* p_tiles = OBJ_TILE_MEM + (TILE_PLAT_GREEN * 8);

    // Green Platform Tile Data
    for (int t = 0; t < 4; t++) {
        for (int row = 0; row < 8; row++) {
            unsigned int word = 0;
            for (int px = 0; px < 8; px++) {
                int global_x = t * 8 + px;
                unsigned char c = 0;
                if ((row == 0 || row == 7) && (global_x < 2 || global_x > 29)) c = 0;
                else if (row == 0 || row == 7 || global_x == 0 || global_x == 31) c = 4; // Border
                else if (row == 1) c = 1; // Highlight
                else if (row < 5) c = 2;  // Main
                else c = 3;               // Shadow
                word |= ((c & 0xF) << (px * 4));
            }
            p_tiles[t * 8 + row] = word;
        }
    }

    // 2. Build 16x16 Spring Hardware Tiles (Tiles 28-31)
    volatile unsigned int* sp_tiles = OBJ_TILE_MEM + (TILE_SPRING * 8);
    for (int t = 0; t < 4; t++) {
        int tx = (t & 1) * 8;
        int ty = (t >> 1) * 8;
        for (int r = 0; r < 8; r++) {
            unsigned int word = 0;
            for (int px = 0; px < 8; px++) {
                int x = tx + px;
                int y = ty + r;
                unsigned char c = 0;
                if (y >= 10 && y <= 11 && x >= 3 && x <= 12) c = 5; // Red cap
                else if (y >= 12 && (x == 4 || x == 11 || (y == 13 && x >= 5 && x <= 10))) c = 4; // Coil
                word |= ((c & 0xF) << (px * 4));
            }
            sp_tiles[t * 8 + r] = word;
        }
    }

    // 3. Nose Pellet (Tile 32: 8x8 round pellet)
    volatile unsigned int* pel_tile = OBJ_TILE_MEM + (TILE_PELLET * 8);
    for (int r = 0; r < 8; r++) {
        unsigned int word = 0;
        for (int px = 0; px < 8; px++) {
            unsigned char c = 0;
            int dx = px - 4, dy = r - 4;
            if (dx * dx + dy * dy <= 4) c = 10;
            word |= ((c & 0xF) << (px * 4));
        }
        pel_tile[r] = word;
    }
}

// ==========================================================================
// AUDIO
// ==========================================================================

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
// GAMEPLAY STATE & CONSTANTS
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
    int type;          // 0: Green, 1: Blue, 2: Wood
    int vx;
    int has_spring;
    int spring_anim;
    int crumble_timer;
} Platform;

typedef struct {
    int x, y;          // 16.16 FP
    int vy;            // 16.16 FP
    int facing;        // 1: Right, -1: Left (Hardware H-FLIP)
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
    platforms[i].spring_anim = 0;

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
    // Mode 0: 4 Tilemap Layers, 1D OBJ Mapping, Enable BG0 & OBJ
    REG_DISPCNT = 0x1140;

    // BG0: Priority 0, Charblock 0 (Tiles), Screenblock 31 (Map), 32x32 Tiles
    REG_BG0CNT = (0 << 2) | (31 << 8);

    init_sound();
    init_palettes();
    init_background_hardware();
    init_sprite_hardware_tiles();
    reset_game();

    while (1) {
        unsigned short keys = ~REG_KEYINPUT;

        if (!game_over) {
            if (keys & KEY_LEFT)  { player.x -= MOVE_SPEED; player.facing = -1; }
            if (keys & KEY_RIGHT) { player.x += MOVE_SPEED; player.facing = 1; }

            // Shoot Nose Pellet (B Button)
            static int shoot_cooldown = 0;
            if (shoot_cooldown > 0) shoot_cooldown--;
            if ((keys & KEY_B) && shoot_cooldown == 0) {
                for (int i = 0; i < MAX_PELLETS; i++) {
                    if (!pellets[i].active) {
                        pellets[i].active = 1;
                        pellets[i].x = FP_TO_INT(player.x) + (player.facing == 1 ? 22 : 2);
                        pellets[i].y = FP_TO_INT(player.y) + 10;
                        pellets[i].vy = -6;
                        snd_shoot();
                        shoot_cooldown = 15;
                        break;
                    }
                }
            }

            for (int i = 0; i < MAX_PELLETS; i++) {
                if (pellets[i].active) {
                    pellets[i].y += pellets[i].vy;
                    if (pellets[i].y < -4) pellets[i].active = 0;
                }
            }

            // Screen Wrap
            if (FP_TO_INT(player.x) < -PLAYER_W) player.x = INT_TO_FP(SCREEN_W);
            else if (FP_TO_INT(player.x) > SCREEN_W) player.x = INT_TO_FP(-PLAYER_W);

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
                if (platforms[i].spring_anim > 0) platforms[i].spring_anim--;
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
                                platforms[i].spring_anim = 20;
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

            // Camera Vertical Scroll (Hardware BG0)
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
        // HARDWARE OAM SPRITE BUFFER UPDATE (<1% CPU)
        // ==================================================================

        // Hide all 128 hardware sprites by default (Attr0 bit 8-9 = 2)
        for (int i = 0; i < 128; i++) {
            oam_buffer[i].attr0 = 0x0200;
        }

        // Sprite 0: The Doodler (32x32 Hardware Sprite)
        if (!game_over) {
            int d_px = FP_TO_INT(player.x);
            int d_py = FP_TO_INT(player.y);
            // Attr0: Y pos, Shape 0 (Square)
            oam_buffer[0].attr0 = (d_py & 0xFF) | (0 << 14);
            // Attr1: X pos, Size 2 (32x32), Hardware H-FLIP on bit 12!
            oam_buffer[0].attr1 = (d_px & 0x1FF) | (2 << 14) | (player.facing == -1 ? (1 << 12) : 0);
            // Attr2: Starting Tile 0, Palette Bank 0
            oam_buffer[0].attr2 = TILE_DOODLER_BASE | (0 << 12);
        }

        // Sprites 1-6: Platforms (32x8 Wide Hardware Sprites)
        for (int i = 0; i < NUM_PLATFORMS; i++) {
            if (platforms[i].crumble_timer < 0) continue;
            int spr_idx = 1 + i;
            // Attr0: Y pos, Shape 1 (Wide)
            oam_buffer[spr_idx].attr0 = (platforms[i].y & 0xFF) | (1 << 14);
            // Attr1: X pos, Size 1 (32x8)
            oam_buffer[spr_idx].attr1 = (platforms[i].x & 0x1FF) | (1 << 14);
            // Attr2: Tile offset, Palette Bank 1 (Green/Blue) or 2 (Wood)
            int pal_bank = (platforms[i].type == 2) ? 2 : 1;
            oam_buffer[spr_idx].attr2 = TILE_PLAT_GREEN | (pal_bank << 12);

            // Sprites 7-12: Springs (16x16 Hardware Sprites)
            if (platforms[i].has_spring) {
                int sp_idx = 7 + i;
                oam_buffer[sp_idx].attr0 = ((platforms[i].y - 8) & 0xFF) | (0 << 14);
                oam_buffer[sp_idx].attr1 = ((platforms[i].x + 12) & 0x1FF) | (1 << 14); // 16x16
                oam_buffer[sp_idx].attr2 = TILE_SPRING | (2 << 12);
            }
        }

        // Sprites 13-15: Pellets (8x8 Hardware Sprites)
        for (int i = 0; i < MAX_PELLETS; i++) {
            if (pellets[i].active) {
                int p_idx = 13 + i;
                oam_buffer[p_idx].attr0 = (pellets[i].y & 0xFF) | (0 << 14);
                oam_buffer[p_idx].attr1 = (pellets[i].x & 0x1FF) | (0 << 14); // 8x8
                oam_buffer[p_idx].attr2 = TILE_PELLET | (0 << 12);
            }
        }

        // ==================================================================
        // HARDWARE VBLANK PUSH (Instant DMA & 1-Cycle Hardware Scroll)
        // ==================================================================
        vsync();

        // 1. Hardware Background Vertical Scroll (Cost: 1 clock cycle)
        REG_BG0VOFS = -camera_y;

        // 2. Hardware Sprite DMA Push (Cost: ~150 cycles)
        REG_DMA3SAD = (unsigned int)oam_buffer;
        REG_DMA3DAD = 0x07000000;
        REG_DMA3CNT = 0x84000000 | (128 * 2); // 128 entries (256 16-bit words)
    }

    return 0;
}

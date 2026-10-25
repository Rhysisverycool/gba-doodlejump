/* ==========================================================================
 *  CHROME DINO DELUXE (GBA)
 *  - 256-Color Palette with Dynamic Day / Sunset / Night Cycles
 *  - 3-Layer Parallax Desert Scrolling (Mesas, Dunes, Textured Strata)
 *  - 4-Frame Fluid Run Cycle, Jump/Fall/Duck Poses, 3-Frame Flapping Ptero
 *  - Dust Particle Engine & Screen Shake Impact
 * ========================================================================== */

#define REG_DISPCNT     *(volatile unsigned short*)0x04000000
#define REG_VCOUNT      *(volatile unsigned short*)0x04000006
#define REG_KEYINPUT    *(volatile unsigned short*)0x04000130

#define REG_DMA3SAD     *(volatile unsigned int*)0x040000D4
#define REG_DMA3DAD     *(volatile unsigned int*)0x040000D8
#define REG_DMA3CNT     *(volatile unsigned int*)0x040000DC

// Hardware PSG Sound Registers
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

// Button Input
#define KEY_A           (1 << 0)
#define KEY_B           (1 << 1)
#define KEY_START       (1 << 3)
#define KEY_UP          (1 << 6)
#define KEY_DOWN        (1 << 7)

#define SCREEN_W        240
#define SCREEN_H        160
#define GROUND_Y        126

#define PALETTE_MEM     ((volatile unsigned short*)0x05000000)
#define VRAM            ((volatile unsigned short*)0x06000000)
#define RGB15(r, g, b)  ((r) | ((g) << 5) | ((b) << 10))

// 16.16 Fixed Point Math
#define FP_SHIFT        16
#define INT_TO_FP(x)    ((x) << FP_SHIFT)
#define FP_TO_INT(x)    ((x) >> FP_SHIFT)

#define SPEED_INITIAL   157286  // 2.40 px/frame
#define SPEED_MAX       360000  // 5.50 px/frame
#define SPEED_ACCEL     24      // Gradual acceleration
#define GRAVITY         16384   // 0.25 px/frame^2
#define JUMP_INITIAL_VEL -275251// Full jump velocity
#define JUMP_DROP_VEL   -137626 // Short-hop cutoff
#define FAST_FALL_GRAV  49152   // 3x Gravity drop

// EWRAM Back-Buffer (aligned to 32 bits for fast DMA3 copy)
static unsigned char back_buffer[SCREEN_W * SCREEN_H] __attribute__((aligned(4)));

// ==========================================================================
// COLOR PALETTE SYSTEM (256 Colors)
// ==========================================================================
#define PAL_TRANS       0
#define PAL_SKY_BASE    1   // 1..8: Sky gradient
#define PAL_SUN         10
#define PAL_MOON        11
#define PAL_STAR        12
#define PAL_CLOUD_HI    13
#define PAL_CLOUD_LO    14
#define PAL_MESA_DARK   16  // Distant mountains
#define PAL_MESA_LIGHT  17
#define PAL_DUNE_DARK   18  // Midground dunes
#define PAL_DUNE_LIGHT  19
#define PAL_SAND_TOP    20  // Foreground ground
#define PAL_SAND_SUB    21
#define PAL_SAND_DARK   22
#define PAL_PEBBLE      23

// Dino Palette
#define PAL_DINO_OUTLINE 30
#define PAL_DINO_DARK    31
#define PAL_DINO_MAIN    32
#define PAL_DINO_LIGHT   33
#define PAL_DINO_BELLY   34
#define PAL_DINO_WHITE   35
#define PAL_DINO_EYE     36
#define PAL_DINO_RED     37

// Cactus & Obstacles
#define PAL_CAC_OUTLINE  40
#define PAL_CAC_DARK     41
#define PAL_CAC_MAIN     42
#define PAL_CAC_LIGHT    43
#define PAL_CAC_FLOWER   44

// Pterodactyl
#define PAL_PTERO_OUTLINE 50
#define PAL_PTERO_BODY    51
#define PAL_PTERO_WING    52
#define PAL_PTERO_BEAK    53

// Particles & HUD
#define PAL_DUST_LIGHT   60
#define PAL_DUST_DARK    61
#define PAL_TEXT_WHITE   62
#define PAL_TEXT_GOLD    63
#define PAL_TEXT_SHADOW  64

enum Theme { THEME_DAY, THEME_SUNSET, THEME_NIGHT };
static enum Theme current_theme = THEME_DAY;

static void load_palette(enum Theme theme) {
    current_theme = theme;
    PALETTE_MEM[PAL_TRANS] = 0x0000;

    // Entity Colors (Identical across themes for contrast)
    PALETTE_MEM[PAL_DINO_OUTLINE] = RGB15(3, 7, 5);
    PALETTE_MEM[PAL_DINO_DARK]    = RGB15(6, 15, 8);
    PALETTE_MEM[PAL_DINO_MAIN]    = RGB15(11, 23, 11); // Classic retro reptile green
    PALETTE_MEM[PAL_DINO_LIGHT]   = RGB15(17, 28, 14);
    PALETTE_MEM[PAL_DINO_BELLY]   = RGB15(27, 29, 21); // Pale cream
    PALETTE_MEM[PAL_DINO_WHITE]   = RGB15(31, 31, 31);
    PALETTE_MEM[PAL_DINO_EYE]     = RGB15(2, 2, 2);
    PALETTE_MEM[PAL_DINO_RED]     = RGB15(28, 6, 6);

    PALETTE_MEM[PAL_CAC_OUTLINE]  = RGB15(3, 10, 4);
    PALETTE_MEM[PAL_CAC_DARK]     = RGB15(6, 17, 7);
    PALETTE_MEM[PAL_CAC_MAIN]     = RGB15(12, 24, 10);
    PALETTE_MEM[PAL_CAC_LIGHT]    = RGB15(18, 29, 14);
    PALETTE_MEM[PAL_CAC_FLOWER]   = RGB15(31, 8, 16);

    PALETTE_MEM[PAL_PTERO_OUTLINE]= RGB15(8, 3, 3);
    PALETTE_MEM[PAL_PTERO_BODY]   = RGB15(24, 12, 8);
    PALETTE_MEM[PAL_PTERO_WING]   = RGB15(29, 7, 6);  // Crimson leathery wings
    PALETTE_MEM[PAL_PTERO_BEAK]   = RGB15(31, 22, 6);

    PALETTE_MEM[PAL_DUST_LIGHT]   = RGB15(26, 25, 21);
    PALETTE_MEM[PAL_DUST_DARK]    = RGB15(20, 18, 14);
    PALETTE_MEM[PAL_TEXT_WHITE]   = RGB15(31, 31, 31);
    PALETTE_MEM[PAL_TEXT_GOLD]    = RGB15(31, 26, 6);
    PALETTE_MEM[PAL_TEXT_SHADOW]  = RGB15(4, 4, 4);

    if (theme == THEME_DAY) {
        // Bright Desert Day Sky & Terrain
        for (int i = 0; i < 8; i++) {
            int r = 16 + i * 2;
            int g = 22 + i;
            int b = 30;
            PALETTE_MEM[PAL_SKY_BASE + i] = RGB15(r, g, b);
        }
        PALETTE_MEM[PAL_SUN]        = RGB15(31, 30, 16);
        PALETTE_MEM[PAL_CLOUD_HI]   = RGB15(31, 31, 31);
        PALETTE_MEM[PAL_CLOUD_LO]   = RGB15(23, 25, 27);

        PALETTE_MEM[PAL_MESA_DARK]  = RGB15(16, 18, 24);
        PALETTE_MEM[PAL_MESA_LIGHT] = RGB15(19, 22, 27);
        PALETTE_MEM[PAL_DUNE_DARK]  = RGB15(24, 21, 16);
        PALETTE_MEM[PAL_DUNE_LIGHT] = RGB15(28, 25, 20);

        PALETTE_MEM[PAL_SAND_TOP]   = RGB15(29, 27, 21);
        PALETTE_MEM[PAL_SAND_SUB]   = RGB15(23, 19, 14);
        PALETTE_MEM[PAL_SAND_DARK]  = RGB15(17, 13, 9);
        PALETTE_MEM[PAL_PEBBLE]     = RGB15(12, 10, 8);
    } else if (theme == THEME_SUNSET) {
        // Fiery Sunset
        for (int i = 0; i < 8; i++) {
            int r = 24 + i;
            int g = 8 + (i * 2);
            int b = 14 - i;
            if (b < 0) b = 0;
            PALETTE_MEM[PAL_SKY_BASE + i] = RGB15(r > 31 ? 31 : r, g > 31 ? 31 : g, b);
        }
        PALETTE_MEM[PAL_SUN]        = RGB15(31, 20, 8);
        PALETTE_MEM[PAL_CLOUD_HI]   = RGB15(31, 22, 22);
        PALETTE_MEM[PAL_CLOUD_LO]   = RGB15(24, 12, 15);

        PALETTE_MEM[PAL_MESA_DARK]  = RGB15(14, 6, 14);
        PALETTE_MEM[PAL_MESA_LIGHT] = RGB15(20, 9, 16);
        PALETTE_MEM[PAL_DUNE_DARK]  = RGB15(22, 11, 10);
        PALETTE_MEM[PAL_DUNE_LIGHT] = RGB15(27, 15, 12);

        PALETTE_MEM[PAL_SAND_TOP]   = RGB15(25, 16, 12);
        PALETTE_MEM[PAL_SAND_SUB]   = RGB15(18, 10, 8);
        PALETTE_MEM[PAL_SAND_DARK]  = RGB15(12, 6, 5);
        PALETTE_MEM[PAL_PEBBLE]     = RGB15(8, 4, 4);
    } else {
        // Deep Indigo Starry Night
        for (int i = 0; i < 8; i++) {
            int r = 1 + i / 3;
            int g = 2 + i / 2;
            int b = 6 + i;
            PALETTE_MEM[PAL_SKY_BASE + i] = RGB15(r, g, b);
        }
        PALETTE_MEM[PAL_MOON]       = RGB15(30, 30, 24);
        PALETTE_MEM[PAL_STAR]       = RGB15(31, 31, 31);
        PALETTE_MEM[PAL_CLOUD_HI]   = RGB15(8, 10, 16);
        PALETTE_MEM[PAL_CLOUD_LO]   = RGB15(4, 5, 10);

        PALETTE_MEM[PAL_MESA_DARK]  = RGB15(3, 4, 8);
        PALETTE_MEM[PAL_MESA_LIGHT] = RGB15(5, 7, 12);
        PALETTE_MEM[PAL_DUNE_DARK]  = RGB15(6, 8, 13);
        PALETTE_MEM[PAL_DUNE_LIGHT] = RGB15(9, 11, 17);

        PALETTE_MEM[PAL_SAND_TOP]   = RGB15(10, 12, 18);
        PALETTE_MEM[PAL_SAND_SUB]   = RGB15(6, 8, 12);
        PALETTE_MEM[PAL_SAND_DARK]  = RGB15(3, 4, 7);
        PALETTE_MEM[PAL_PEBBLE]     = RGB15(16, 18, 22);
    }
}

// ==========================================================================
// SPRITE PIXEL ART
// ==========================================================================

// Color Key:
// '.' = 0, 'O' = Outline, 'D' = Dark, 'G' = Main, 'L' = Light, 'B' = Belly
// 'W' = White, 'K' = Eye/Pupil, 'R' = Red/Blush, 'F' = Flower/Wing Accent, 'Y' = Beak

static const char* dino_run0[22] = {
    "..........OOOOOO......",
    "........OOGLLLLGGO....",
    ".......OGLLLLLLLLGO...",
    ".......OGLLWKLGLGGO...",
    ".......OGLLLLLLGGGO...",
    ".......OGLLGGGGGOOO...",
    ".......OGLLLLLLGO.....",
    ".......OGGLLGGGO......",
    "OO....OOGGGGGGO.......",
    "OGOOOOGGGGGGBBO.......",
    "OGGGGGGGGGGBBBO.......",
    ".OGGGGGGGGGBBBO.......",
    "..OGGGGGGGGBBO........",
    "...OGGGGGGGGO.........",
    "....OGGGGGGGO.........",
    ".....OGGGGGGO.........",
    "......OGGOOGGO........",
    "......OGGO.OGGO.......",
    "......OGGO..OGGO......",
    "......OGGO...OO.......",
    ".....OOOGG............",
    "......................"
};

static const char* dino_run1[22] = {
    "......................",
    "..........OOOOOO......",
    "........OOGLLLLGGO....",
    ".......OGLLLLLLLLGO...",
    ".......OGLLWKLGLGGO...",
    ".......OGLLLLLLGGGO...",
    ".......OGLLGGGGGOOO...",
    ".......OGLLLLLLGO.....",
    ".......OGGLLGGGO......",
    "OO....OOGGGGGGO.......",
    "OGOOOOGGGGGGBBO.......",
    "OGGGGGGGGGGBBBO.......",
    ".OGGGGGGGGGBBBO.......",
    "..OGGGGGGGGBBO........",
    "...OGGGGGGGGO.........",
    "....OGGGGGGGO.........",
    ".....OGGGGGGO.........",
    "......OGGGGO..........",
    "......OGGOGGO.........",
    "......OGG.OGG.........",
    "......OO..OO..........",
    "......................"
};

static const char* dino_run2[22] = {
    "..........OOOOOO......",
    "........OOGLLLLGGO....",
    ".......OGLLLLLLLLGO...",
    ".......OGLLWKLGLGGO...",
    ".......OGLLLLLLGGGO...",
    ".......OGLLGGGGGOOO...",
    ".......OGLLLLLLGO.....",
    ".......OGGLLGGGO......",
    "OO....OOGGGGGGO.......",
    "OGOOOOGGGGGGBBO.......",
    "OGGGGGGGGGGBBBO.......",
    ".OGGGGGGGGGBBBO.......",
    "..OGGGGGGGGBBO........",
    "...OGGGGGGGGO.........",
    "....OGGGGGGGO.........",
    ".....OGGGGGGO.........",
    "......OGGOOGGO........",
    ".....OGGO..OGGO.......",
    "....OGGO...OGGO.......",
    "....OO.....OGGO.......",
    "..........OOOGG.......",
    "......................"
};

static const char* dino_run3[22] = {
    "..........OOOOOO......",
    "........OOGLLLLGGO....",
    ".......OGLLLLLLLLGO...",
    ".......OGLLWKLGLGGO...",
    ".......OGLLLLLLGGGO...",
    ".......OGLLGGGGGOOO...",
    ".......OGLLLLLLGO.....",
    ".......OGGLLGGGO......",
    "OO....OOGGGGGGO.......",
    "OGOOOOGGGGGGBBO.......",
    "OGGGGGGGGGGBBBO.......",
    ".OGGGGGGGGGBBBO.......",
    "..OGGGGGGGGBBO........",
    "...OGGGGGGGGO.........",
    "....OGGGGGGGO.........",
    ".....OGGGGGGO.........",
    "......OGGGGO..........",
    "......OGGGGO..........",
    "......OGGOGG..........",
    "......OGG.OGG.........",
    ".....OO....OO.........",
    "......................"
};

static const char* dino_jump[22] = {
    "..........OOOOOO......",
    "........OOGLLLLGGO....",
    ".......OGLLLLLLLLGO...",
    ".......OGLLWKLGLGGO...",
    ".......OGLLLLLLGGGO...",
    ".......OGLLGGGGGOOO...",
    ".......OGLLLLLLGO.....",
    "OO....OOGLLGGGO.......",
    "OGOOOOGGGGGGGGO.......",
    ".OGGGGGGGGGBBBO.......",
    "..OGGGGGGGGBBBO.......",
    "...OGGGGGGGBBO........",
    "....OGGGGGGGGO........",
    ".....OGGGGGGO.........",
    "......OGGGGO..........",
    "......OGGOOO..........",
    ".....OGGO.OOO.........",
    "....OOOGG.OGGO........",
    "..........OO..........",
    "......................",
    "......................",
    "......................"
};

static const char* dino_dead[22] = {
    "..........OOOOOO......",
    "........OOGLLLLGGO....",
    ".......OGLLLLLLLLGO...",
    ".......OGLLRKRGLGGO...", // Red surprised eye with 'X'
    ".......OGLLLLLLGGGO...",
    ".......OGLLGGGGGOOO...",
    ".......OGLLLLLLGO.....",
    ".......OGGLLGGGO......",
    "OO....OOGGGGGGO.......",
    "OGOOOOGGGGGGBBO.......",
    "OGGGGGGGGGGBBBO.......",
    ".OGGGGGGGGGBBBO.......",
    "..OGGGGGGGGBBO........",
    "...OGGGGGGGGO.........",
    "....OGGGGGGGO.........",
    ".....OGGGGGGO.........",
    "......OGGOOGGO........",
    ".....OGGO...OGGO......",
    "....OGGO.....OGGO.....",
    "....OO........OO......",
    "......................",
    "......................"
};

static const char* dino_duck0[15] = {
    "....................OOOOOO............",
    "..................OOGLLLLGGO..........",
    ".................OGLLLLLLLLGO.........",
    ".................OGLLWKLGLGGO.........",
    ".................OGLLLLLLGGGO.........",
    "OO....OOOOOOOOOOOOGLLGGGGGOOO.........",
    "OGOOOOGGGGGGGGGGGGGLLLLLLGO...........",
    ".OGGGGGGGGGGGGGGGBBLLGGGO.............",
    "..OGGGGGGGGGGGGGGBBBOOO...............",
    "...OGGGGGGGGGGGGGBBBO.................",
    "....OGGGGGGGGGGGGBBO..................",
    ".....OGGGGOOOGGGGO....................",
    ".....OGGO...OGGO......................",
    "....OOOGG..OOOGG......................",
    "......................................"
};

static const char* dino_duck1[15] = {
    "....................OOOOOO............",
    "..................OOGLLLLGGO..........",
    ".................OGLLLLLLLLGO.........",
    ".................OGLLWKLGLGGO.........",
    ".................OGLLLLLLGGGO.........",
    "OO....OOOOOOOOOOOOGLLGGGGGOOO.........",
    "OGOOOOGGGGGGGGGGGGGLLLLLLGO...........",
    ".OGGGGGGGGGGGGGGGBBLLGGGO.............",
    "..OGGGGGGGGGGGGGGBBBOOO...............",
    "...OGGGGGGGGGGGGGBBBO.................",
    "....OGGGGGGGGGGGGBBO..................",
    ".....OGGGGOOOGGGGO....................",
    "......OGGO.OGGO.......................",
    "......OO...OO.........................",
    "......................................"
};

static const char* cactus_small_art[18] = {
    "....OOOO....",
    "...OLLGGO...",
    "...OLLGGO...",
    ".OOOLLGGO...",
    ".OGGLLGGO.OO",
    ".OGGLLGGOGGO",
    ".OGGLLGGOGGO",
    ".OGGLLLLLLGO",
    "..OOLLLLGGO.",
    "...OLLGGOO..",
    "...OLLGGO...",
    "...OLLGGO...",
    "...OLLGGO...",
    "...OLLGGO...",
    "...OLLGGO...",
    "...OLLGGO...",
    "...OLLGGO...",
    "...OOOOOO..."
};

static const char* cactus_tall_art[26] = {
    "......FF......", // Flower on top!
    ".....FFFF.....",
    "......OO......",
    "....OOLLGGO...",
    "...OLLLLGGO...",
    "...OLLLLGGO...",
    ".OOOLLLLGGO...",
    ".OGGLLLLGGO.OO",
    ".OGGLLLLGGOGGO",
    ".OGGLLLLGGOGGO",
    ".OGGLLLLGGOGGO",
    ".OGGLLLLLLLLGO",
    "..OOLLLLLLGGO.",
    "...OLLLLGGOO..",
    "...OLLLLGGO...",
    "...OLLLLGGO...",
    "...OLLLLGGO...",
    "...OLLLLGGO...",
    "...OLLLLGGO...",
    "...OLLLLGGO...",
    "...OLLLLGGO...",
    "...OLLLLGGO...",
    "...OLLLLGGO...",
    "...OLLLLGGO...",
    "...OLLLLGGO...",
    "...OOOOOOOO..."
};

static const char* ptero_frame0[16] = { // Wings Up
    "...........OO..................",
    "..........OFFO.................",
    ".........OFFFFO................",
    "........OFFFFFLLOO.............",
    ".......OFFFFFFFFLLOO...........",
    "......OFFFFFFFFFFLLLO..........",
    "OOOO.OFFFFFFFFFFFFLLO..........",
    "OBBBOOFFFFFFFFFFFFFLLO..OOOO...",
    "OBBBBOOFFFFFFFFFFFFFLLOOOBBBO..",
    ".OBBBBBOOFFFFFFFFFFFFLLOBBBOO..",
    "..OBBBBBBOOOOOOOOOOOOOOWKBYYYO.",
    "...OBBBBBBBBBBBBBBBBBBBBBYYYYO.",
    "....OOOOOBBBBBBBBBBBBBBBBYYYO..",
    "........OOOOOOOOOOOOOOOOOOOO...",
    "...............................",
    "..............................."
};

static const char* ptero_frame1[16] = { // Wings Glide
    "...............................",
    "...............................",
    "...............................",
    "...............................",
    "OOOO....................OOOO...",
    "OBBBBOOOOOOOOOOOOOOOOOOOBBBOO..",
    ".OBBBBLLLLLLLLLLLLLLLLLOWKBYYYO",
    "..OBBBFFFFFFFFFFFFFFFFFFBYYYYO.",
    "...OBBFFFFFFFFFFFFFFFFFFBYYYO..",
    "....OOOOOBBBBBBBBBBBBBBBOOOO...",
    "........OOOOOOOOOOOOOOOO.......",
    "...............................",
    "...............................",
    "...............................",
    "...............................",
    "...............................
};

static const char* ptero_frame2[16] = { // Wings Down
    "...............................",
    "...............................",
    "...............................",
    "...............................",
    "...............................",
    "OOOO....................OOOO...",
    "OBBBOOOOOOOOOOOOOOOOOOOOBBBOO..",
    ".OBBBBLLLLLLLLLLLLLLLLLOWKBYYYO",
    "..OBBBBBBBBBBBBBBBBBBBBBBYYYYO.",
    "...OBBBBBOOFFFFFFFFFFFFFBYYYO..",
    "....OOOO.OFFFFFFFFFFFFFFOOOO...",
    "........OFFFFFFFFFFFLLLO.......",
    ".......OFFFFFFFFLLLLOO.........",
    "......OFFFFFLLLLOO.............",
    ".....OFFFLO....................",
    ".....OOOO......................"
};

// ==========================================================================
// SPRITE PARSING & BUFFERING
// ==========================================================================

#define SPR_DINO_W 22
#define SPR_DINO_H 22
static unsigned char spr_dino_run[4][SPR_DINO_H][SPR_DINO_W];
static unsigned char spr_dino_jump[SPR_DINO_H][SPR_DINO_W];
static unsigned char spr_dino_dead[SPR_DINO_H][SPR_DINO_W];

#define SPR_DUCK_W 38
#define SPR_DUCK_H 15
static unsigned char spr_dino_duck[2][SPR_DUCK_H][SPR_DUCK_W];

#define SPR_CSM_W 12
#define SPR_CSM_H 18
static unsigned char spr_cactus_sm[SPR_CSM_H][SPR_CSM_W];

#define SPR_CLG_W 14
#define SPR_CLG_H 26
static unsigned char spr_cactus_lg[SPR_CLG_H][SPR_CLG_W];

#define SPR_PTERO_W 31
#define SPR_PTERO_H 16
static unsigned char spr_ptero[3][SPR_PTERO_H][SPR_PTERO_W];

static unsigned char map_char_to_pal(char c) {
    switch (c) {
        case 'O': return PAL_DINO_OUTLINE;
        case 'D': return PAL_DINO_DARK;
        case 'G': return PAL_DINO_MAIN;
        case 'L': return PAL_DINO_LIGHT;
        case 'B': return PAL_DINO_BELLY;
        case 'W': return PAL_DINO_WHITE;
        case 'K': return PAL_DINO_EYE;
        case 'R': return PAL_DINO_RED;
        case 'F': return PAL_CAC_FLOWER;
        case 'Y': return PAL_PTERO_BEAK;
        default:  return PAL_TRANS;
    }
}

static void decode_sprite(const char** ascii, unsigned char* dest, int w, int h) {
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            dest[y * w + x] = map_char_to_pal(ascii[y][x]);
        }
    }
}

static void init_all_sprites(void) {
    decode_sprite(dino_run0, (unsigned char*)spr_dino_run[0], SPR_DINO_W, SPR_DINO_H);
    decode_sprite(dino_run1, (unsigned char*)spr_dino_run[1], SPR_DINO_W, SPR_DINO_H);
    decode_sprite(dino_run2, (unsigned char*)spr_dino_run[2], SPR_DINO_W, SPR_DINO_H);
    decode_sprite(dino_run3, (unsigned char*)spr_dino_run[3], SPR_DINO_W, SPR_DINO_H);

    decode_sprite(dino_jump, (unsigned char*)spr_dino_jump, SPR_DINO_W, SPR_DINO_H);
    decode_sprite(dino_dead, (unsigned char*)spr_dino_dead, SPR_DINO_W, SPR_DINO_H);

    decode_sprite(dino_duck0, (unsigned char*)spr_dino_duck[0], SPR_DUCK_W, SPR_DUCK_H);
    decode_sprite(dino_duck1, (unsigned char*)spr_dino_duck[1], SPR_DUCK_W, SPR_DUCK_H);

    decode_sprite(cactus_small_art, (unsigned char*)spr_cactus_sm, SPR_CSM_W, SPR_CSM_H);
    decode_sprite(cactus_tall_art, (unsigned char*)spr_cactus_lg, SPR_CLG_W, SPR_CLG_H);

    decode_sprite(ptero_frame0, (unsigned char*)spr_ptero[0], SPR_PTERO_W, SPR_PTERO_H);
    decode_sprite(ptero_frame1, (unsigned char*)spr_ptero[1], SPR_PTERO_W, SPR_PTERO_H);
    decode_sprite(ptero_frame2, (unsigned char*)spr_ptero[2], SPR_PTERO_W, SPR_PTERO_H);
}

// ==========================================================================
// SOUND ENGINE
// ==========================================================================
static void init_sound(void) {
    REG_SOUNDCNT_X = 0x0080;
    REG_SOUNDCNT_L = 0x7777;
    REG_SOUNDCNT_H = 0x0002;
}

static inline void snd_jump(void)  {
    REG_SOUND1CNT_L = 0x0015; // Sweep pitch up!
    REG_SOUND1CNT_H = 0xB240;
    REG_SOUND1CNT_X = 0x8640;
}

static inline void snd_point(void) {
    REG_SOUND1CNT_L = 0x0000;
    REG_SOUND1CNT_H = 0xF240;
    REG_SOUND1CNT_X = 0x8740;
    REG_SOUND2CNT_L = 0xF180;
    REG_SOUND2CNT_H = 0x8790;
}

static inline void snd_death(void) {
    REG_SOUND4CNT_L = 0xF300; // Noise crash
    REG_SOUND4CNT_H = 0x8054;
    REG_SOUND1CNT_L = 0x003A; // Descending tone
    REG_SOUND1CNT_H = 0xA280;
    REG_SOUND1CNT_X = 0x8200;
}

// PRNG
static unsigned int rng = 0x87654321;
static inline unsigned int rand_next(void) {
    rng = rng * 1664525u + 1013904223u;
    return (rng >> 16);
}

// ==========================================================================
// PARTICLE SYSTEM
// ==========================================================================
#define MAX_PARTICLES 28
typedef struct {
    int active;
    int x, y;   // 16.16 FP
    int vx, vy; // 16.16 FP
    int life;
    int max_life;
    unsigned char color;
} Particle;

static Particle particles[MAX_PARTICLES];

static void spawn_particle(int x, int y, int vx, int vy, int life, unsigned char color) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!particles[i].active) {
            particles[i].active = 1;
            particles[i].x = INT_TO_FP(x);
            particles[i].y = INT_TO_FP(y);
            particles[i].vx = vx;
            particles[i].vy = vy;
            particles[i].life = life;
            particles[i].max_life = life;
            particles[i].color = color;
            break;
        }
    }
}

static void update_particles(void) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (particles[i].active) {
            particles[i].x += particles[i].vx;
            particles[i].y += particles[i].vy;
            particles[i].vy += 1200; // Particle gravity
            particles[i].life--;
            if (particles[i].life <= 0) particles[i].active = 0;
        }
    }
}

// ==========================================================================
// GAME STATE
// ==========================================================================
#define OBS_CACTUS_SM 0
#define OBS_CACTUS_LG 1
#define OBS_PTERO     2
#define MAX_OBS       3

typedef struct {
    int active;
    int x; // 16.16 FP
    int y;
    int type;
    int w, h;
    int anim_frame;
} Obstacle;

static Obstacle obstacles[MAX_OBS];

typedef struct {
    int x;
    int y;  // 16.16 FP
    int vy; // 16.16 FP
    int is_jumping;
    int is_ducking;
    int was_jumping;
    int dead;
    int anim_tick;
} Dino;

static Dino dino;
static int distance_ran_fp = 0;
static int dist_since_last_spawn_fp = 0;
static int next_spawn_gap = 320;
static int score = 0;
static int high_score = 0;
static int speed_fp = SPEED_INITIAL;
static int ground_scroll = 0;
static int milestone_blink = 0;
static int game_over = 0;
static int screen_shake = 0;

static void reset_game(void) {
    load_palette(THEME_DAY);

    dino.x = 26;
    dino.y = INT_TO_FP(GROUND_Y - SPR_DINO_H);
    dino.vy = 0;
    dino.is_jumping = 0;
    dino.is_ducking = 0;
    dino.was_jumping = 0;
    dino.dead = 0;
    dino.anim_tick = 0;

    distance_ran_fp = 0;
    dist_since_last_spawn_fp = 0;
    next_spawn_gap = 360;

    score = 0;
    speed_fp = SPEED_INITIAL;
    ground_scroll = 0;
    milestone_blink = 0;
    game_over = 0;
    screen_shake = 0;

    for (int i = 0; i < MAX_OBS; i++) obstacles[i].active = 0;
    for (int i = 0; i < MAX_PARTICLES; i++) particles[i].active = 0;
}

static void spawn_obstacle(void) {
    if (FP_TO_INT(dist_since_last_spawn_fp) < next_spawn_gap) return;

    int idx = -1;
    for (int i = 0; i < MAX_OBS; i++) {
        if (!obstacles[i].active) { idx = i; break; }
    }
    if (idx == -1) return;

    Obstacle* ob = &obstacles[idx];
    ob->active = 1;
    ob->x = INT_TO_FP(SCREEN_W);
    ob->anim_frame = 0;

    int r = rand_next() % 100;
    if (score > 300 && r < 36) {
        ob->type = OBS_PTERO;
        ob->w = SPR_PTERO_W;
        ob->h = SPR_PTERO_H;
        int alt = rand_next() % 3;
        if (alt == 0)      ob->y = GROUND_Y - 14; // Low (must duck!)
        else if (alt == 1) ob->y = GROUND_Y - 28; // Mid (must jump!)
        else               ob->y = GROUND_Y - 48; // High (overhead)
    } else if (r < 72) {
        ob->type = OBS_CACTUS_SM;
        ob->w = SPR_CSM_W;
        ob->h = SPR_CSM_H;
        ob->y = GROUND_Y - SPR_CSM_H;
    } else {
        ob->type = OBS_CACTUS_LG;
        ob->w = SPR_CLG_W;
        ob->h = SPR_CLG_H;
        ob->y = GROUND_Y - SPR_CLG_H;
    }

    dist_since_last_spawn_fp = 0;
    next_spawn_gap = 230 + (rand_next() % 130);
}

static inline int check_collision(Obstacle* ob) {
    int ox = FP_TO_INT(ob->x) + 4;
    int oy = ob->y + 2;
    int ow = ob->w - 8;
    int oh = ob->h - 4;

    int dx = dino.x + 4;
    int dy = FP_TO_INT(dino.y) + 3;
    int dw = dino.is_ducking ? (SPR_DUCK_W - 8) : (SPR_DINO_W - 8);
    int dh = dino.is_ducking ? (SPR_DUCK_H - 4) : (SPR_DINO_H - 4);

    return (dx < ox + ow && dx + dw > ox && dy < oy + oh && dy + dh > oy);
}

// ==========================================================================
// FAST RENDERING & DISPLAY
// ==========================================================================

static inline void vsync(void) {
    while (REG_VCOUNT >= 160);
    while (REG_VCOUNT < 160);
}

static inline void flip_screen_dma(void) {
    vsync();
    REG_DMA3SAD = (unsigned int)back_buffer;
    REG_DMA3DAD = (unsigned int)VRAM;
    REG_DMA3CNT = 0x84000000 | (SCREEN_W * SCREEN_H / 2); // 16-bit copy
}

static void draw_sprite(int sx, int sy, const unsigned char* sprite, int w, int h) {
    for (int y = 0; y < h; y++) {
        int py = sy + y;
        if (py < 0 || py >= SCREEN_H) continue;
        int row = py * SCREEN_W;
        for (int x = 0; x < w; x++) {
            int px = sx + x;
            if (px < 0 || px >= SCREEN_W) continue;
            unsigned char col = sprite[y * w + x];
            if (col != PAL_TRANS) {
                back_buffer[row + px] = col;
            }
        }
    }
}

// 4x6 Font Glyphs
static const unsigned short font4x6[12] = {
    0x6999, 0x2222, 0xE178, 0xE161, 0x99F1, 0xF8E1, 0xF8E9, 0xF124, 0x6969, 0x6971,
    0x99F9, 0xF44E // 'H', 'I'
};

static void draw_glyph(int x, int y, int idx, unsigned char color) {
    unsigned short data = font4x6[idx];
    for (int r = 0; r < 4; r++) {
        int sy = y + r;
        if (sy >= SCREEN_H) continue;
        int row = sy * SCREEN_W;
        int line = (data >> ((3 - r) * 4)) & 0xF;
        for (int c = 0; c < 4; c++) {
            if (line & (1 << (3 - c))) {
                int sx = x + c;
                if (sx < SCREEN_W) {
                    back_buffer[row + sx] = color;
                }
            }
        }
    }
}

static void draw_number(int x, int y, int val, unsigned char col) {
    char digits[5];
    int temp = val;
    for (int i = 4; i >= 0; i--) {
        digits[i] = temp % 10;
        temp /= 10;
    }
    for (int i = 0; i < 5; i++) {
        draw_glyph(x + i * 5, y, digits[i], col);
    }
}

// ==========================================================================
// MAIN LOOP & LOGIC
// ==========================================================================

int main(void) {
    REG_DISPCNT = 0x0404; // Mode 4, BG2 enabled
    init_sound();
    init_all_sprites();
    reset_game();

    while (1) {
        unsigned short keys = ~REG_KEYINPUT;

        if (!game_over) {
            // Dynamic Time of Day Transition
            int phase = (score % 900);
            if (phase < 450) {
                if (current_theme != THEME_DAY) load_palette(THEME_DAY);
            } else if (phase < 600) {
                if (current_theme != THEME_SUNSET) load_palette(THEME_SUNSET);
            } else {
                if (current_theme != THEME_NIGHT) load_palette(THEME_NIGHT);
            }

            // Input: Chromium 1:1 Jump & Variable Fall
            dino.was_jumping = dino.is_jumping;
            if ((keys & KEY_A) || (keys & KEY_UP)) {
                if (!dino.is_jumping && !dino.is_ducking) {
                    dino.vy = JUMP_INITIAL_VEL;
                    dino.is_jumping = 1;
                    snd_jump();
                    // Dust puff on jump liftoff
                    spawn_particle(dino.x + 6, GROUND_Y - 2, -0x8000, -0x8000, 10, PAL_DUST_LIGHT);
                    spawn_particle(dino.x + 10, GROUND_Y - 2, -0x4000, -0xC000, 12, PAL_DUST_DARK);
                }
            } else {
                if (dino.is_jumping && dino.vy < JUMP_DROP_VEL) {
                    dino.vy = JUMP_DROP_VEL; // Short-hop cutoff
                }
            }

            int grav = GRAVITY;
            if (keys & KEY_DOWN) {
                if (dino.is_jumping) {
                    grav = FAST_FALL_GRAV; // 3x fast-fall
                } else {
                    dino.is_ducking = 1;
                    // Dust while sliding
                    if ((dino.anim_tick & 3) == 0) {
                        spawn_particle(dino.x - 2, GROUND_Y - 3, -speed_fp / 2, -0x4000, 8, PAL_DUST_LIGHT);
                    }
                }
            } else {
                dino.is_ducking = 0;
            }

            dino.vy += grav;
            dino.y += dino.vy;

            int floor_y = INT_TO_FP(dino.is_ducking ? (GROUND_Y - SPR_DUCK_H) : (GROUND_Y - SPR_DINO_H));
            if (dino.y >= floor_y) {
                dino.y = floor_y;
                dino.vy = 0;
                dino.is_jumping = 0;

                // Dust burst upon landing!
                if (dino.was_jumping) {
                    spawn_particle(dino.x + 2, GROUND_Y - 2, -0xA000, -0x8000, 12, PAL_DUST_LIGHT);
                    spawn_particle(dino.x + 16, GROUND_Y - 2, 0x8000, -0x8000, 12, PAL_DUST_LIGHT);
                }
            }

            // Running footstep dust
            if (!dino.is_jumping && !dino.is_ducking && (dino.anim_tick % 8 == 0)) {
                spawn_particle(dino.x + 2, GROUND_Y - 2, -speed_fp / 3, -0x3000, 8, PAL_DUST_LIGHT);
            }

            // Move Obstacles
            for (int i = 0; i < MAX_OBS; i++) {
                if (obstacles[i].active) {
                    obstacles[i].x -= speed_fp;
                    if (FP_TO_INT(obstacles[i].x) < -40) {
                        obstacles[i].active = 0;
                    }

                    if (obstacles[i].type == OBS_PTERO && (dino.anim_tick % 8 == 0)) {
                        obstacles[i].anim_frame = (obstacles[i].anim_frame + 1) % 3;
                    }

                    if (check_collision(&obstacles[i])) {
                        dino.dead = 1;
                        game_over = 1;
                        screen_shake = 16;
                        snd_death();
                        if (score > high_score) high_score = score;
                    }
                }
            }

            dist_since_last_spawn_fp += speed_fp;
            spawn_obstacle();
            update_particles();

            ground_scroll += FP_TO_INT(speed_fp);
            if (speed_fp < SPEED_MAX) speed_fp += SPEED_ACCEL;

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
            update_particles();
            if ((keys & KEY_A) || (keys & KEY_START)) reset_game();
        }

        // ==================================================================
        // RENDER PASS
        // ==================================================================

        int shake_offset_y = 0;
        if (screen_shake > 0) {
            shake_offset_y = (screen_shake & 2) ? -2 : 2;
            screen_shake--;
        }

        // 1. Sky Gradient Fill
        for (int y = 0; y < 100; y++) {
            int pal_idx = PAL_SKY_BASE + (y * 8 / 100);
            if (pal_idx > PAL_SKY_BASE + 7) pal_idx = PAL_SKY_BASE + 7;
            unsigned int fill = pal_idx | (pal_idx << 8) | (pal_idx << 16) | (pal_idx << 24);
            unsigned int* line = (unsigned int*)(back_buffer + (y * SCREEN_W));
            for (int x = 0; x < SCREEN_W / 4; x++) line[x] = fill;
        }

        // 2. Celestial Body (Sun / Moon)
        if (current_theme == THEME_DAY || current_theme == THEME_SUNSET) {
            int sun_x = 180;
            int sun_y = (current_theme == THEME_DAY) ? 24 : 50;
            for (int dy = -6; dy <= 6; dy++) {
                for (int dx = -6; dx <= 6; dx++) {
                    if (dx*dx + dy*dy <= 36) {
                        back_buffer[(sun_y + dy) * SCREEN_W + (sun_x + dx)] = PAL_SUN;
                    }
                }
            }
        } else {
            // Crescent Moon & Twinkling Stars
            int moon_x = 190, moon_y = 25;
            for (int dy = -5; dy <= 5; dy++) {
                for (int dx = -5; dx <= 5; dx++) {
                    if (dx*dx + dy*dy <= 25 && ((dx+2)*(dx+2) + dy*dy > 20)) {
                        back_buffer[(moon_y + dy) * SCREEN_W + (moon_x + dx)] = PAL_MOON;
                    }
                }
            }
            // Twinkling Stars
            for (int s = 0; s < 12; s++) {
                int sx = (s * 37 + (ground_scroll / 32)) % (SCREEN_W - 10) + 5;
                int sy = 10 + (s * 17) % 55;
                back_buffer[sy * SCREEN_W + sx] = PAL_STAR;
            }
        }

        // 3. Parallax Layer 1: Distant Mesas (0.1x Speed)
        int mesa_scroll = (ground_scroll >> 4) % 120;
        for (int x = 0; x < SCREEN_W; x++) {
            int mx = (x + mesa_scroll) % 120;
            int h = (mx < 60) ? (mx / 3) : ((120 - mx) / 3);
            for (int y = 92 - h; y < 105; y++) {
                back_buffer[y * SCREEN_W + x] = (mx < 50) ? PAL_MESA_LIGHT : PAL_MESA_DARK;
            }
        }

        // 4. Parallax Layer 2: Rolling Dunes (0.35x Speed)
        int dune_scroll = (ground_scroll >> 2) % 80;
        for (int x = 0; x < SCREEN_W; x++) {
            int dx = (x + dune_scroll) % 80;
            int h = (dx < 40) ? (dx / 4) : ((80 - dx) / 4);
            for (int y = 105 - h; y < GROUND_Y; y++) {
                back_buffer[y * SCREEN_W + x] = (dx < 35) ? PAL_DUNE_LIGHT : PAL_DUNE_DARK;
            }
        }

        // 5. Foreground Ground Strata & Texture
        for (int y = GROUND_Y; y < SCREEN_H; y++) {
            int pal = (y == GROUND_Y) ? PAL_SAND_TOP : ((y < GROUND_Y + 6) ? PAL_SAND_SUB : PAL_SAND_DARK);
            unsigned int fill = pal | (pal << 8) | (pal << 16) | (pal << 24);
            unsigned int* line = (unsigned int*)(back_buffer + (y * SCREEN_W));
            for (int x = 0; x < SCREEN_W / 4; x++) line[x] = fill;
        }

        // Scrolling Desert Grass & Pebbles
        int g_off = ground_scroll % 60;
        for (int p = 0; p < 5; p++) {
            int px = p * 55 - g_off;
            if (px >= 0 && px < SCREEN_W - 4) {
                back_buffer[(GROUND_Y + 3) * SCREEN_W + px] = PAL_PEBBLE;
                back_buffer[(GROUND_Y + 3) * SCREEN_W + px + 1] = PAL_PEBBLE;
                back_buffer[(GROUND_Y + 7) * SCREEN_W + px + 12] = PAL_PEBBLE;
            }
        }

        // 6. Particles
        for (int i = 0; i < MAX_PARTICLES; i++) {
            if (particles[i].active) {
                int px = FP_TO_INT(particles[i].x);
                int py = FP_TO_INT(particles[i].y) + shake_offset_y;
                if (px >= 0 && px < SCREEN_W && py >= 0 && py < SCREEN_H) {
                    back_buffer[py * SCREEN_W + px] = particles[i].color;
                    if (particles[i].life > 6 && px + 1 < SCREEN_W) {
                        back_buffer[py * SCREEN_W + px + 1] = particles[i].color;
                    }
                }
            }
        }

        // 7. Obstacles
        for (int i = 0; i < MAX_OBS; i++) {
            if (!obstacles[i].active) continue;
            int ox = FP_TO_INT(obstacles[i].x);
            int oy = obstacles[i].y + shake_offset_y;

            if (obstacles[i].type == OBS_CACTUS_SM) {
                draw_sprite(ox, oy, (const unsigned char*)spr_cactus_sm, SPR_CSM_W, SPR_CSM_H);
            } else if (obstacles[i].type == OBS_CACTUS_LG) {
                draw_sprite(ox, oy, (const unsigned char*)spr_cactus_lg, SPR_CLG_W, SPR_CLG_H);
            } else if (obstacles[i].type == OBS_PTERO) {
                draw_sprite(ox, oy, (const unsigned char*)spr_ptero[obstacles[i].anim_frame], SPR_PTERO_W, SPR_PTERO_H);
            }
        }

        // 8. Dino
        int dx = dino.x;
        int dy = FP_TO_INT(dino.y) + shake_offset_y;

        if (dino.dead) {
            draw_sprite(dx, dy, (const unsigned char*)spr_dino_dead, SPR_DINO_W, SPR_DINO_H);
        } else if (dino.is_ducking && !dino.is_jumping) {
            int duck_frame = (dino.anim_tick / 4) & 1;
            draw_sprite(dx, dy, (const unsigned char*)spr_dino_duck[duck_frame], SPR_DUCK_W, SPR_DUCK_H);
        } else if (dino.is_jumping) {
            draw_sprite(dx, dy, (const unsigned char*)spr_dino_jump, SPR_DINO_W, SPR_DINO_H);
        } else {
            int run_frame = (dino.anim_tick / 4) % 4; // 4-Frame fluid run
            draw_sprite(dx, dy, (const unsigned char*)spr_dino_run[run_frame], SPR_DINO_W, SPR_DINO_H);
        }

        // 9. HUD / Scoreboard
        // High Score
        draw_glyph(152, 6, 10, PAL_TEXT_GOLD);
        draw_glyph(157, 6, 11, PAL_TEXT_GOLD);
        draw_number(166, 6, high_score, PAL_TEXT_GOLD);

        // Current Score
        if (milestone_blink == 0 || (milestone_blink & 8)) {
            draw_number(204, 6, score, PAL_TEXT_WHITE);
        }

        if (game_over) {
            draw_number(108, 62, score, PAL_TEXT_WHITE);
        }

        flip_screen_dma();
    }

    return 0;
}

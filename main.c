/* ==========================================================================
   CHROME DINO GBA - DELUXE EDITION
   ==========================================================================
   Full hardware rewrite:
   - Mode 0 tiled engine + hardware sprites (OAM) instead of slow bitmap fill
   - HDMA scanline gradient sky (DMA3 on HBlank, re-latched every VBlank)
   - 3-layer parallax: far ridge (BG2), mountains (BG1), ground (BG0)
   - Smooth day / dusk / night / dawn cycle: every palette entry lerps,
     sun sets, moon rises, stars twinkle in
   - Dino: 4-frame run cycle, jump frame, 2-frame duck, dead X-eye
   - Affine squash & stretch on jump / landing (hardware rotation params)
   - Particle system: landing dust, run dust, death burst, milestone sparkles
   - Screen shake, death flash (palette invert), score blink on milestones
   - Cactus clusters (1-3), pterodactyls with flap anim, bob and speed drift
   - PSG sound: jump sweep, landing blip, milestone arpeggio, death crash
   - SRAM high-score persistence, START = pause, Chromium 1:1 physics kept
   Build: arm-none-eabi-gcc -O2 -mthumb -mthumb-interwork -mcpu=arm7tdmi
          -nostartfiles -T gba_cart.ld main.c crt0.s -o dino.elf
          arm-none-eabi-objcopy -O binary dino.elf dino.gba
   Controls: A / B / UP = jump (hold = higher, release = short hop)
             DOWN = duck / fast-fall, START = pause / restart
   ========================================================================== */

#define REG_DISPCNT   (volatile unsigned short*)0x04000000
#define REG_DISPSTAT  (volatile unsigned short*)0x04000004
#define REG_VCOUNT    (volatile unsigned short*)0x04000006
#define REG_BG0CNT    (volatile unsigned short*)0x04000008
#define REG_BG1CNT    (volatile unsigned short*)0x0400000A
#define REG_BG2CNT    (volatile unsigned short*)0x0400000C
#define REG_BG0HOFS   (volatile unsigned short*)0x04000010
#define REG_BG0VOFS   (volatile unsigned short*)0x04000012
#define REG_BG1HOFS   (volatile unsigned short*)0x04000014
#define REG_BG1VOFS   (volatile unsigned short*)0x04000016
#define REG_BG2HOFS   (volatile unsigned short*)0x04000018
#define REG_BG2VOFS   (volatile unsigned short*)0x0400001A
#define REG_KEYINPUT  (volatile unsigned short*)0x04000130
#define REG_WAITCNT   (volatile unsigned short*)0x04000204

#define REG_DMA1SAD   (volatile unsigned int*)0x040000BC
#define REG_DMA1DAD   (volatile unsigned int*)0x040000C0
#define REG_DMA1CNT   (volatile unsigned int*)0x040000C4
#define REG_DMA3SAD   (volatile unsigned int*)0x040000D4
#define REG_DMA3DAD   (volatile unsigned int*)0x040000D8
#define REG_DMA3CNT   (volatile unsigned int*)0x040000DC

/* --- PSG audio --- */
#define REG_SOUNDCNT_L (volatile unsigned short*)0x04000080
#define REG_SOUNDCNT_H (volatile unsigned short*)0x04000082
#define REG_SOUNDCNT_X (volatile unsigned short*)0x04000084
#define REG_SOUND1CNT_L (volatile unsigned short*)0x04000060
#define REG_SOUND1CNT_H (volatile unsigned short*)0x04000062
#define REG_SOUND1CNT_X (volatile unsigned short*)0x04000064
#define REG_SOUND2CNT_L (volatile unsigned short*)0x04000068
#define REG_SOUND2CNT_H (volatile unsigned short*)0x0400006C
#define REG_SOUND4CNT_L (volatile unsigned short*)0x04000078
#define REG_SOUND4CNT_H (volatile unsigned short*)0x0400007C

/* --- Keys --- */
#define KEY_A     (1 << 0)
#define KEY_B     (1 << 1)
#define KEY_START (1 << 3)
#define KEY_UP    (1 << 6)
#define KEY_DOWN  (1 << 7)

#define RGB15(r, g, b) ((r) | ((g) << 5) | ((b) << 10))

#define SCREEN_W 240
#define SCREEN_H 160
#define GROUND_Y 128

/* --- Memory regions --- */
#define BG_PAL    ((volatile unsigned short*)0x05000000)
#define OBJ_PAL   ((volatile unsigned short*)0x05000200)
#define OBJ_VRAM  ((unsigned char*)0x06010000)
#define BG_VRAM   ((unsigned char*)0x06000000)
#define BG0_MAP   ((volatile unsigned short*)0x0600E000)   /* screen base 28 */
#define BG1_MAP   ((volatile unsigned short*)0x0600D000)   /* screen base 26 */
#define BG2_MAP   ((volatile unsigned short*)0x0600C000)   /* screen base 24 */
#define OAM_MEM   ((volatile unsigned short*)0x07000000)
#define SRAM_HI   ((volatile unsigned char*)0x0E000000)

/* --- 16.16 fixed point --- */
#define FP_SHIFT 16
#define INT_TO_FP(x) ((x) << FP_SHIFT)
#define FP_TO_INT(x) ((x) >> FP_SHIFT)

/* --- DISPCNT values: mode 0, BG0/1/2 + OBJ, 1D tile mapping --- */
#define DISP_MODE0  0x0000
#define DISP_OBJ_1D 0x0040
#define DISP_BLANK  0x0080
#define DISP_BG0    0x0100
#define DISP_BG1    0x0200
#define DISP_BG2    0x0400
#define DISP_OBJ    0x1000

/* --- DMA control bits --- */
#define DMA_ENABLE  0x80000000
#define DMA_32BIT   0x00000400   /* bit22? no: see below - unused helper   */
/* NOTE: we build control words inline where used, keep it explicit. */

/* --- OBJ attribute bits --- */
#define ATTR0_Y(y)        ((y) & 0xFF)
#define ATTR0_AFFINE      0x0100
#define ATTR0_HIDE        0x0200
#define ATTR0_AFFINE_DBL  0x0300
#define ATTR0_SQ          (0 << 14)
#define ATTR0_WIDE        (1 << 14)
#define ATTR0_TALL        (2 << 14)
#define ATTR1_X(x)        ((x) & 0x1FF)
#define ATTR1_SIZE(n)     ((n) << 14)
#define ATTR1_AFF_GRP(n)  ((n) << 9)
#define ATTR2_TILE(t)     ((t) & 0x3FF)
#define ATTR2_PRIO(p)     ((p) << 10)
#define ATTR2_PAL(b)      ((b) << 12)

/* Sprite shapes/sizes: square 8/16/32/64, wide 16x8/32x8/32x16/64x32,
   tall 8x16/8x32/16x32/32x64 */
#define SZ_SQ8  0
#define SZ_SQ16 1
#define SZ_SQ32 2
#define SZ_W32X16 2   /* with ATTR0_WIDE */
#define SZ_T16X32 2   /* with ATTR0_TALL */

/* ==========================================================================
   CHROMIUM 1:1 PHYSICS (kept from original)
   ========================================================================== */
#define SPEED_INITIAL 157286    /* 2.40 px/frame */
#define SPEED_MAX     340787    /* 5.20 px/frame */
#define SPEED_ACCEL   26        /* 0.0004 px/frame^2, ~2 min to max */

#define GRAVITY        16384    /* 0.25 px/frame^2 */
#define JUMP_VEL      -275251   /* -4.20 px/frame full jump */
#define JUMP_DROP_VEL -137626   /* -2.10 short-hop cutoff */
#define FAST_FALL_G   49152     /* 3x gravity */

/* ==========================================================================
   ART FORMAT: strings, one char per pixel.
   '.' = transparent, '1'-'9' = palette index. Packed to 4bpp at boot.
   ========================================================================== */

/* ---------------- DINO: 32x24 body (legs stamped on rows 24-31) --------- */
static const char* const art_dino_body[24] = {
"..................11111111111...",
"................1222222222221...",
"...............122222222444221..",
"...............122222222444521..",
"...............122222222444221..",
"...............1222222222222....",
"...............122222222222.....",
"...............1222222222221....",
"................1222222222221...",
".................1222222221.....",
"..............122222222221......",
"..121.......122222222222221.....",
".1222221..12222222222222221.....",
"122222222222222222222222221.....",
"..1222222222222222222222.21.....",
"..122222222233333322222121......",
"...1222222223333332222211.......",
"...12222222233333322221.........",
"...122222222333333222221........",
"....122222222333333222221.......",
"....12222222233333322221........",
".....12222222233333322221.......",
"......1222222222333332211.......",
".......122222222222222211......."
};

/* Legs variants: 32x8, stamped at rows 24-31 */
static const char* const art_leg_a[8] = {
"........121.....121.............",
"........121.....121.............",
"........121.....1211............",
"........121.....12111...........",
"........121......1211...........",
"......111111.....121............",
"......1111111...................",
"................................"
};
static const char* const art_leg_b[8] = {
"........121.....121.............",
"........121.....121.............",
".......1121.....121.............",
"......11121.....121.............",
".......1121.....121.............",
"........121...111111............",
".............1111111............",
"................................"
};
static const char* const art_leg_mid[8] = {
"........121.....121.............",
"........121.....121.............",
"........121.....121.............",
"........121.....121.............",
"........121.....121.............",
"........121.....121.............",
"......111111...111111...........",
"......111111...111111..........."
};
static const char* const art_leg_jump[8] = {
"........121.....121.............",
"........121.....121.............",
"........121.....121.............",
".......11121...11121............",
"......111111..111111............",
"................................",
"................................",
"................................"
};
static const char* const art_eye_x[5] = {
"44444",
"41414",
"44144",
"41414",
"44444"
};

/* ---------------- DINO DUCK: 32x12 body + 32x4 feet x2 ------------------ */
static const char* const art_duck_body[12] = {
"..................111111111111..",
".................1222222222221..",
"................122222244521....",
"................12222224421.....",
"................12222222221.....",
".................12222222221....",
".................1222222221.....",
"111.............12222222221.....",
".1221.........122222222221......",
"..12221111122222222222221.......",
"...122222222222222222221........",
"....112222222222222221.........."
};
static const char* const art_duck_leg_a[4] = {
"......1211......121.............",
"......1211......121.............",
".....11111.....11111............",
"................................"
};
static const char* const art_duck_leg_b[4] = {
"......121.......1211............",
"......121.......1211............",
".....11111.....11111............",
"................................"
};

/* ---------------- CACTUS SMALL: 16x32 (top 8 rows empty) ---------------- */
static const char* const art_cact_s[32] = {
"................",
"................",
"................",
"................",
"................",
"................",
"................",
"................",
"......1111......",
"......1221......",
"1121..1221..1121",
"1221..1221..1221",
"1221..1221..1221",
"1221..1221..1221",
"1221..1221..1221",
".1221112211221..",
"..12222222221...",
"...1221221221...",
"....11222211....",
"......1221......",
"......1221......",
"......1221......",
"......1221......",
"......1221......",
"......1221......",
"......1221......",
"......1221......",
"......1221......",
"......1221......",
"......1221......",
"......1221......",
"......1221......"
};

/* ---------------- CACTUS LARGE: 32x32 (flower on top) ------------------- */
static const char* const art_cact_l[32] = {
".............44444..............",
".............45554..............",
".............44444..............",
"............11111111............",
"............12222221............",
"............12233221............",
"............12233221............",
"............12233221............",
"............12233221............",
"11111.......12233221............",
"122211......12233221............",
"122221......12233221............",
"122221......12233221........1111",
"122221......12233221........1221",
"122221......12233221........1221",
".12221122222222222221122222211..",
"..122222222222222222222222221...",
"...11222222222222222222222211...",
".....11222222222222222222111....",
"........12222221................",
"........12233221................",
"........12233221................",
"........12233221................",
"........12233221................",
"........12233221................",
"........12233221................",
"........12233221................",
"........12233221................",
"........12233221................",
"........12233221................",
"........12233221................",
"........12233221................"
};

/* ---------------- PTERODACTYL: 32x32, 2 frames (hitbox at cols 7-24, rows 9-22) --- */
static const char* const art_ptero_up[32] = {
"................................",
"................................",
".............11.................",
"............1221................",
"...........122221...............",
"..........12222211..............",
".........1222222221.............",
"........122222222221............",
".......12222222222211...........",
"......1122222222222221..........",
".....112222222222222221.........",
"....1122222222222222221.........",
"....122222222222222111111.......",
"....122222222222221222221.......",
".....122222222222212455211111...",
"......12222222222212455522221...",
".......112222222221124444221....",
".........112222222112111111.....",
"...........11222211.............",
"............112211..............",
".............1121...............",
"................................",
"................................",
"................................",
"................................",
"................................",
"................................",
"................................",
"................................",
"................................",
"................................",
"................................"
};
static const char* const art_ptero_down[32] = {
"................................",
"................................",
"................................",
"................................",
"................................",
"................................",
"................................",
"................................",
"................................",
"................................",
"................................",
"................................",
"....122222222222222111111.......",
"....122222222222221222221.......",
".....122222222222212455211111...",
"......12222222222212455522221...",
".......112222222221124444221....",
"........112222222211211111......",
".........1122222211.............",
"..........122222211.............",
"..........122222221.............",
".........12222222221............",
"........1222222222221...........",
".......122222222222221..........",
".......12222222222221...........",
"......12222222222211............",
".....1222222222211..............",
"....112222222211................",
"....1222222221..................",
"....11222211....................",
".....1121.......................",
"................................"
};

/* ---------------- CLOUD 32x16 ------------------------------------------- */
static const char* const art_cloud[16] = {
"................................",
"................................",
"..........11111.................",
"........112222211...............",
".......12222222221..............",
"..1111222222222222111...........",
".122222222222222222221..........",
"122222222222222222222211........",
"122222222222222222222222111111..",
".1222222222222222222222222221...",
"..111111111111111111111111111...",
"................................",
"................................",
"................................",
"................................",
"................................"
};

/* ---------------- SUN 32x32, MOON 16x16 --------------------------------- */
static const char* const art_sun[32] = {
"................................",
"................................",
"................................",
"................................",
"...............2................",
"...............2................",
"...............2................",
".......2................2.......",
"........2..............2........",
".........2...111111...2.........",
"............11111111............",
"...........1111111111...........",
"..........111111111111..........",
".........11111111111111.........",
".........11111111111111.........",
".........11111111111111.........",
"....222..11111111111111..2222...",
".........11111111111111.........",
".........11111111111111.........",
"..........111111111111..........",
"...........1111111111...........",
"............11111111............",
".........2...111111...2.........",
"........2..............2........",
".......2................2.......",
"................2...............",
"................2...............",
"................2...............",
"................2...............",
"................................",
"................................",
"................................"
};
static const char* const art_moon[16] = {
"................",
"......1.........",
"....11..........",
"...111..........",
"..1111..........",
"..111...........",
".1112...........",
".11111..........",
".11111..........",
".111121.........",
"..111121........",
"..11111111..11..",
"...1111111111...",
"....11111111....",
"......1111......",
"................"
};

/* ---------------- STAR 8x8 x2, DUST 8x8 x2, SPARK 8x8 x2 ---------------- */
static const char* const art_star_a[8] = {
"...1....",
"...1....",
"..111...",
".11111..",
"..111...",
"...1....",
"...1....",
"........"
};
static const char* const art_star_b[8] = {
"........",
"...1....",
"........",
"..1.1...",
"........",
"...1....",
"........",
"........"
};
static const char* const art_dust_a[8] = {
"........",
"..111...",
".11111..",
".11111..",
"..111...",
"........",
"........",
"........"
};
static const char* const art_dust_b[8] = {
"........",
"........",
"..22....",
".222....",
"..2.....",
"........",
"........",
"........"
};
static const char* const art_spark_a[8] = {
"...1....",
"...1....",
".11211..",
"...1....",
"...1....",
"........",
"........",
"........"
};
static const char* const art_spark_b[8] = {
"........",
"........",
"..11....",
"........",
"........",
"........",
"........",
"........"
};

/* ---------------- RESTART ICON 16x16 (circular arrow) ------------------- */
static const char* const art_restart[16] = {
"................",
"....111111......",
"..1122222211....",
".122111122221...",
".121...1222.21..",
"1211...1221.21..",
"121....1221.21..",
"121....1222.21..",
"121.....122221..",
"121.....12211...",
"1221...1211.....",
".122111211......",
"..1122211.......",
"....111.........",
"................",
"................"
};

/* ---------------- 8x8 FONT: A D E G H I M O P R S T U V + 0-9 ----------- */
static const char* const art_font_A[8] = {
"........",
"..1111..",
".1....1.",
".1....1.",
".111111.",
".1....1.",
".1....1.",
"........"
};
static const char* const art_font_D[8] = {
"........",
".11111..",
".1....1.",
".1....1.",
".1....1.",
".1....1.",
".11111..",
"........"
};
static const char* const art_font_E[8] = {
"........",
".111111.",
".1......",
".1......",
".11111..",
".1......",
".111111.",
"........"
};
static const char* const art_font_G[8] = {
"........",
"..11111.",
".1......",
".1......",
".1...111",
".1....1.",
".111111.",
"........"
};
static const char* const art_font_H[8] = {
"........",
".1....1.",
".1....1.",
".1....1.",
".111111.",
".1....1.",
".1....1.",
"........"
};
static const char* const art_font_I[8] = {
"........",
".111111.",
"...11...",
"...11...",
"...11...",
"...11...",
".111111.",
"........"
};
static const char* const art_font_M[8] = {
"........",
".1....1.",
".11..11.",
".1.11.1.",
".1.11.1.",
".1....1.",
".1....1.",
"........"
};
static const char* const art_font_O[8] = {
"........",
"..1111..",
".1....1.",
".1....1.",
".1....1.",
".1....1.",
"..1111..",
"........"
};
static const char* const art_font_P[8] = {
"........",
".111111.",
".1....1.",
".1....1.",
".111111.",
".1......",
".1......",
"........"
};
static const char* const art_font_R[8] = {
"........",
".111111.",
".1....1.",
".1....1.",
".111111.",
".1...1..",
".1....1.",
"........"
};
static const char* const art_font_S[8] = {
"........",
".111111.",
".1......",
".1......",
".111111.",
"......1.",
".111111.",
"........"
};
static const char* const art_font_T[8] = {
"........",
".111111.",
"...11...",
"...11...",
"...11...",
"...11...",
"...11...",
"........"
};
static const char* const art_font_U[8] = {
"........",
".1....1.",
".1....1.",
".1....1.",
".1....1.",
".1....1.",
".111111.",
"........"
};
static const char* const art_font_V[8] = {
"........",
".1....1.",
".1....1.",
".1....1.",
".1....1.",
"..1..1..",
"...11...",
"........"
};
static const char* const art_font_0[8] = {
"........",
"..1111..",
".11..11.",
".11.111.",
".111.11.",
".11..11.",
"..1111..",
"........"
};
static const char* const art_font_1[8] = {
"........",
"...11...",
"..111...",
"...11...",
"...11...",
"...11...",
".111111.",
"........"
};
static const char* const art_font_2[8] = {
"........",
".11111..",
"1.....1.",
".....11.",
"....11..",
"...11...",
".111111.",
"........"
};
static const char* const art_font_3[8] = {
"........",
".11111..",
"1.....1.",
"....111.",
"....111.",
"1....11.",
".11111..",
"........"
};
static const char* const art_font_4[8] = {
"........",
"....11..",
"...111..",
"..1111..",
".11.11..",
".111111.",
"....11..",
"........"
};
static const char* const art_font_5[8] = {
"........",
".111111.",
".1......",
".111111.",
"......11",
"1.....1.",
".11111..",
"........"
};
static const char* const art_font_6[8] = {
"........",
"..1111..",
".1......",
".111111.",
".1....1.",
".1....1.",
"..1111..",
"........"
};
static const char* const art_font_7[8] = {
"........",
".111111.",
"......1.",
".....11.",
"....11..",
"...11...",
"...11...",
"........"
};
static const char* const art_font_8[8] = {
"........",
"..1111..",
".1....1.",
"..1111..",
".1....1.",
".1....1.",
"..1111..",
"........"
};
static const char* const art_font_9[8] = {
"........",
"..1111..",
".1....1.",
".1....11",
".111111.",
"......1.",
".1111...",
"........"
};

/* ---------------- BG TILES: ground strip -------------------------------- */
static const char* const art_t_line[8] = {
"11111111",
"22222222",
"22222222",
"22222222",
"22222222",
"22222222",
"22222222",
"22222222"
};
static const char* const art_t_ground[8] = {
"22222222",
"22222222",
"22222222",
"22222222",
"22222222",
"22222222",
"22222222",
"22222222"
};
static const char* const art_t_bump[8] = {
"........",
"........",
"........",
"........",
"........",
"........",
"........",
"..111..."
};
static const char* const art_t_peb1[8] = {
"22222222",
"22222222",
"22322222",
"22222222",
"22222222",
"22232222",
"22222222",
"22222222"
};
static const char* const art_t_peb2[8] = {
"22222222",
"22222222",
"22222222",
"22232222",
"22222222",
"22322222",
"22222222",
"22222222"
};
static const char* const art_t_peb3[8] = {
"22222222",
"22322223",
"22222222",
"22222222",
"22222222",
"22222222",
"22322222",
"22222222"
};
static const char* const art_t_peb4[8] = {
"22222222",
"22222222",
"22222322",
"22222222",
"23222222",
"22222222",
"22222222",
"22222222"
};

/* ---------------- BG TILES: mountains ----------------------------------- */
static const char* const art_mtn_apex[8] = {
"....2...",
"...22...",
"...112..",
"..1112..",
"..11112.",
".111112.",
".1111112",
"11111112"
};
static const char* const art_mtn_body[8] = {
"11111111",
"11111111",
"11111111",
"11111111",
"11111111",
"11111111",
"11111111",
"11111111"
};

/* ==========================================================================
   ART PACKING: string art -> 4bpp tiles (low nibble = left pixel)
   ========================================================================== */
static unsigned char artbuf[32 * 32];
static int obj_off;   /* byte offset into OBJ tile VRAM */

static int art_val(char c) {
  if (c >= '1' && c <= '9') return c - '0';
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return 0;
}

static void art_load(const char* const* art, int w, int h) {
  int x, y;
  for (y = 0; y < h; y++)
    for (x = 0; x < w; x++)
      artbuf[y * w + x] = (unsigned char)art_val(art[y][x]);
}

static void art_stamp(const char* const* art, int w, int h, int ox, int oy) {
  int x, y;
  for (y = 0; y < h; y++)
    for (x = 0; x < w; x++) {
      int v = art_val(art[y][x]);
      if (v) artbuf[(oy + y) * 32 + ox + x] = (unsigned char)v;
    }
}

static int pack_obj(int w, int h) {
  unsigned char* dst = OBJ_VRAM + obj_off;
  int tx, ty, py, px;
  for (ty = 0; ty < h / 8; ty++)
    for (py = 0; py < 8; py++)
      for (tx = 0; tx < w / 8; tx++)
        for (px = 0; px < 4; px++) {
          int x = tx * 8 + px * 2;
          int a = artbuf[(ty * 8 + py) * w + x];
          int b = artbuf[(ty * 8 + py) * w + x + 1];
          *dst++ = (unsigned char)((a & 15) | ((b & 15) << 4));
        }
  {
    int idx = obj_off / 32;
    obj_off += (w / 8) * (h / 8) * 32;
    return idx;
  }
}

static void pack_bg_tile(int tile_idx, const char* const* art) {
  unsigned char* dst = BG_VRAM + tile_idx * 32;
  int x, y;
  for (y = 0; y < 8; y++)
    for (x = 0; x < 4; x++) {
      int a = art_val(art[y][x * 2]);
      int b = art_val(art[y][x * 2 + 1]);
      dst[y * 4 + x] = (unsigned char)((a & 15) | ((b & 15) << 4));
    }
}

/* ==========================================================================
   THEMES: day / dusk / night, all colors lerped smoothly across the cycle
   ========================================================================== */
enum {
  SKY_TOP, SKY_HOR, GR_LINE, GR_FILL, GR_PEB,
  MTN_NEAR, MTN_APEX, MTN_FAR, MTN_FAPEX,
  UI, UI_DIM, CLOUD_F, CLOUD_S,
  D_OUT, D_BODY, D_BELLY, C_OUT, C_BODY, C_STRIPE,
  P_OUT, P_BODY, P_WING, STAR, SUN, MOON,
  DUST_A, DUST_B, SPARK, THEME_N
};

static const unsigned short THEMES[3][THEME_N] = {
  /* DAY */
  { RGB15(10,20,30), RGB15(22,29,31), RGB15(10,10,11), RGB15(27,24,18), RGB15(17,15,12),
    RGB15(16,18,26), RGB15(22,24,30), RGB15(20,22,29), RGB15(25,27,31),
    RGB15(10,10,11), RGB15(15,15,16), RGB15(31,31,31), RGB15(21,26,31),
    RGB15(4,9,5),  RGB15(10,22,10), RGB15(20,28,16),
    RGB15(3,8,4),  RGB15(7,18,9),  RGB15(13,24,14),
    RGB15(7,4,12), RGB15(16,10,24), RGB15(22,16,29),
    RGB15(10,20,30), RGB15(30,26,8), RGB15(24,25,28),
    RGB15(26,23,17), RGB15(21,18,13), RGB15(30,24,10) },
  /* DUSK */
  { RGB15(12,6,20), RGB15(30,15,8), RGB15(9,7,8), RGB15(22,16,12), RGB15(14,11,9),
    RGB15(12,8,18), RGB15(18,11,20), RGB15(15,10,18), RGB15(20,14,21),
    RGB15(9,7,8), RGB15(13,10,11), RGB15(29,21,19), RGB15(24,14,16),
    RGB15(4,7,4),  RGB15(8,16,8), RGB15(15,21,12),
    RGB15(3,6,3),  RGB15(6,14,7), RGB15(11,19,11),
    RGB15(6,4,10), RGB15(12,8,18), RGB15(17,12,22),
    RGB15(19,13,20), RGB15(30,18,6), RGB15(19,17,22),
    RGB15(20,17,12), RGB15(16,14,10), RGB15(29,21,9) },
  /* NIGHT */
  { RGB15(1,1,6), RGB15(5,4,12), RGB15(22,22,26), RGB15(5,5,8), RGB15(12,12,16),
    RGB15(4,4,9), RGB15(7,7,13), RGB15(6,6,12), RGB15(9,9,15),
    RGB15(24,24,28), RGB15(16,16,20), RGB15(10,11,18), RGB15(7,8,14),
    RGB15(3,5,4),  RGB15(5,10,6), RGB15(9,13,8),
    RGB15(3,6,4),  RGB15(4,9,5), RGB15(7,12,7),
    RGB15(4,3,7), RGB15(8,6,12), RGB15(11,9,15),
    RGB15(30,30,29), RGB15(2,2,6), RGB15(26,27,30),
    RGB15(10,10,12), RGB15(8,8,10), RGB15(28,26,20) },
};

static unsigned short cur[THEME_N];
static int night_amt;         /* 0..256: weight of night theme */
static unsigned short sky_lines[232];  /* one color per scanline for HDMA */

static unsigned short lerp15(unsigned short a, unsigned short b, int t) {
  int ar = a & 31, ag = (a >> 5) & 31, ab = (a >> 10) & 31;
  int br = b & 31, bg = (b >> 5) & 31, bb = (b >> 10) & 31;
  int r, g, bl;
  if (t < 0) t = 0; if (t > 256) t = 256;
  r = ar + (((br - ar) * t) >> 8);
  g = ag + (((bg - ag) * t) >> 8);
  bl = ab + (((bb - ab) * t) >> 8);
  return (unsigned short)RGB15(r, g, bl);
}

/* Evaluate cycle position p (16.16, 0..65536) into cur[] */
static void eval_theme(int p) {
  int a = 0, b = 0, t = 0, u, i;
  night_amt = 0;
  if (p < 43254) {            /* 0.66 : pure day */
    a = 0; t = 0;
  } else if (p < 49807) {     /* 0.66-0.76 : day -> dusk */
    a = 0; b = 1; u = ((p - 43254) * 256) / (49807 - 43254);
    t = (u * u * (768 - 2 * u)) >> 16;
    if (t > 256) t = 256;
  } else if (p < 55706) {     /* 0.76-0.84 : dusk -> night */
    a = 1; b = 2; u = ((p - 49807) * 256) / (55706 - 49807);
    t = (u * u * (768 - 2 * u)) >> 16;
    if (t > 256) t = 256;
    night_amt = t;
  } else if (p < 58982) {     /* 0.84-0.90 : night */
    a = 2; t = 0; night_amt = 256;
  } else if (p < 62915) {     /* 0.90-0.96 : night -> dusk */
    a = 2; b = 1; u = ((p - 58982) * 256) / (62915 - 58982);
    t = (u * u * (768 - 2 * u)) >> 16;
    if (t > 256) t = 256;
    night_amt = 256 - t;
  } else {                    /* 0.96-1.0 : dusk -> day */
    a = 1; b = 0; u = ((p - 62915) * 256) / (65536 - 62915);
    t = (u * u * (768 - 2 * u)) >> 16;
    if (t > 256) t = 256;
  }
  for (i = 0; i < THEME_N; i++) {
    if (b) cur[i] = lerp15(THEMES[a][i], THEMES[b][i], t);
    else   cur[i] = THEMES[a][i];
  }
}

static void apply_palettes(int flash) {
  int inv = flash ? 0x7FFF : 0;

  BG_PAL[0] = cur[SKY_HOR] ^ inv;
  BG_PAL[1] = cur[GR_LINE] ^ inv;  BG_PAL[2] = cur[GR_FILL] ^ inv;
  BG_PAL[3] = cur[GR_PEB] ^ inv;
  BG_PAL[17] = cur[MTN_NEAR] ^ inv; BG_PAL[18] = cur[MTN_APEX] ^ inv;
  BG_PAL[33] = cur[MTN_FAR] ^ inv;  BG_PAL[34] = cur[MTN_FAPEX] ^ inv;

  OBJ_PAL[(0 << 4) + 1] = cur[D_OUT] ^ inv;
  OBJ_PAL[(0 << 4) + 2] = cur[D_BODY] ^ inv;
  OBJ_PAL[(0 << 4) + 3] = cur[D_BELLY] ^ inv;
  OBJ_PAL[(0 << 4) + 4] = RGB15(31,31,31) ^ inv;
  OBJ_PAL[(0 << 4) + 5] = RGB15(2,2,2) ^ inv;
  OBJ_PAL[(1 << 4) + 4] = RGB15(28,14,20) ^ inv;   /* cactus flower pink */
  OBJ_PAL[(1 << 4) + 5] = RGB15(30,26,10) ^ inv;   /* flower center      */

  OBJ_PAL[(1 << 4) + 1] = cur[C_OUT] ^ inv;
  OBJ_PAL[(1 << 4) + 2] = cur[C_BODY] ^ inv;
  OBJ_PAL[(1 << 4) + 3] = cur[C_STRIPE] ^ inv;

  OBJ_PAL[(2 << 4) + 1] = cur[P_OUT] ^ inv;
  OBJ_PAL[(2 << 4) + 2] = cur[P_BODY] ^ inv;
  OBJ_PAL[(2 << 4) + 3] = cur[P_WING] ^ inv;
  OBJ_PAL[(2 << 4) + 4] = RGB15(30,20,6) ^ inv;    /* beak */
  OBJ_PAL[(2 << 4) + 5] = RGB15(31,31,31) ^ inv;   /* eye  */

  OBJ_PAL[(3 << 4) + 1] = cur[CLOUD_S] ^ inv;
  OBJ_PAL[(3 << 4) + 2] = cur[CLOUD_F] ^ inv;
  OBJ_PAL[(3 << 4) + 11] = RGB15(31,31,31) ^ inv;  /* spark core */
  OBJ_PAL[(3 << 4) + 3] = cur[SUN] ^ inv;
  OBJ_PAL[(3 << 4) + 4] = RGB15(28,16,4) ^ inv;    /* sun rays */
  OBJ_PAL[(3 << 4) + 5] = cur[MOON] ^ inv;
  OBJ_PAL[(3 << 4) + 6] = RGB15(20,21,24) ^ inv;   /* moon crater */
  OBJ_PAL[(3 << 4) + 7] = cur[STAR] ^ inv;
  OBJ_PAL[(3 << 4) + 8] = cur[DUST_A] ^ inv;
  OBJ_PAL[(3 << 4) + 9] = cur[DUST_B] ^ inv;
  OBJ_PAL[(3 << 4) + 10] = cur[SPARK] ^ inv;

  OBJ_PAL[(4 << 4) + 1] = cur[UI] ^ inv;
  OBJ_PAL[(4 << 4) + 2] = cur[UI_DIM] ^ inv;
  OBJ_PAL[(5 << 4) + 1] = cur[UI_DIM] ^ inv;
}

/* Build 160-scanline gradient buffer for the HDMA sky */
static void build_sky(void) {
  int y;
  for (y = 0; y < 160; y++) {
    int t = (y * 255) / 159;
    int t2 = (t * t) >> 8;
    sky_lines[y] = lerp15(cur[SKY_TOP], cur[SKY_HOR], t2);
  }
  for (y = 160; y < 232; y++) sky_lines[y] = cur[SKY_HOR];
}

/* ==========================================================================
   SHADOW OAM + SPRITE HELPERS
   ========================================================================== */
typedef struct { unsigned short a0, a1, a2, a3; } ObjAttr;
static ObjAttr oam_sh[128];

static void obj_hide(int i) { oam_sh[i].a0 = ATTR0_HIDE; }

static void obj_set(int i, int x, int y, int shape, int size, int tile, int bank, int prio) {
  oam_sh[i].a0 = (unsigned short)(ATTR0_Y(y) | shape);
  oam_sh[i].a1 = (unsigned short)(ATTR1_X(x) | size);
  oam_sh[i].a2 = (unsigned short)(ATTR2_TILE(tile) | ATTR2_PRIO(prio) | ATTR2_PAL(bank));
}

/* ==========================================================================
   SOUND (PSG)
   ========================================================================== */
static void init_sound(void) {
  *REG_SOUNDCNT_X = 0x8080;   /* master enable (both known bit positions) */
  *REG_SOUNDCNT_L = 0xFF77;
  *REG_SOUNDCNT_H = 0x0002;
}

static void snd_jump(void) {
  *REG_SOUND1CNT_L = 0x0025;                 /* sweep up, fast */
  *REG_SOUND1CNT_H = (10 << 12) | (3 << 8) | (2 << 6);
  *REG_SOUND1CNT_X = 0x8000 | 0x0140;
}

static void snd_land(void) {
  *REG_SOUND2CNT_L = (6 << 12) | (2 << 8) | (1 << 6);
  *REG_SOUND2CNT_H = 0x8000 | 0x00B0;
}

static void snd_death(void) {
  *REG_SOUND4CNT_L = (15 << 12) | (3 << 8);
  *REG_SOUND4CNT_H = 0x8000 | (6 << 4) | (1 << 3) | 2;
  *REG_SOUND1CNT_L = 0x0016;                 /* low sweep down thump */
  *REG_SOUND1CNT_H = (14 << 12) | (3 << 8) | (0 << 6);
  *REG_SOUND1CNT_X = 0x8000 | 0x0060;
}

/* Milestone jingle: 3-note arpeggio C6-E6-G6 sequenced over frames */
static int jingle_t = -1;
static const unsigned short jingle_notes[3] = { 1923, 1949, 1964 };

static void snd_point(void) {
  jingle_t = 0;
  *REG_SOUND2CNT_L = (9 << 12) | (2 << 8) | (2 << 6);
  *REG_SOUND2CNT_H = 0x8000 | jingle_notes[0];
}

static void update_jingle(void) {
  if (jingle_t < 0) return;
  jingle_t++;
  if (jingle_t == 5)  *REG_SOUND2CNT_H = 0x8000 | jingle_notes[1];
  if (jingle_t == 10) *REG_SOUND2CNT_H = 0x8000 | jingle_notes[2];
  if (jingle_t > 14)  jingle_t = -1;
}

/* ==========================================================================
   BG TEXT (font tiles live in BG charblock 0, tiles 16..39, palette bank 0)
   ========================================================================== */
static const char* const* FONT_ART[24] = {
  art_font_A, art_font_D, art_font_E, art_font_G, art_font_H, art_font_I,
  art_font_M, art_font_O, art_font_P, art_font_R, art_font_S, art_font_T,
  art_font_U, art_font_V,
  art_font_0, art_font_1, art_font_2, art_font_3, art_font_4, art_font_5,
  art_font_6, art_font_7, art_font_8, art_font_9
};
#define FONT_BASE 16
#define FONT_N 24

static int font_of(char c) {
  const char* letters = "ADEGHIMOPRSTUV";
  int i;
  for (i = 0; i < 14; i++)
    if (letters[i] == c) return FONT_BASE + i;
  if (c >= '0' && c <= '9') return FONT_BASE + 14 + (c - '0');
  return -1;
}

/* Write text into BG0 map. col_px in screen space; BG0 scroll is frozen
   when text is shown, so cells are stable. */
static void bg_text(const char* s, int col_px, int row, int show) {
  volatile unsigned short* map = BG0_MAP;
  int base = ((*REG_BG0HOFS) >> 3) + (col_px >> 3);
  int i;
  for (i = 0; s[i]; i++) {
    int t = show ? font_of(s[i]) : -1;
    int cell = ((base + i) & 31) + row * 32;
    map[cell] = (t >= 0) ? (unsigned short)t : 0;
  }
}

static void bg_text_clear(int row) {
  int c;
  for (c = 0; c < 32; c++) BG0_MAP[row * 32 + c] = 0;
}

/* ==========================================================================
   BG MAP BUILD
   ========================================================================== */
static void stamp_mtn(volatile unsigned short* map, int cx, int h,
                      int apex_tile, int body_tile, int bank) {
  int dy, dx, row;
  for (dy = 0; dy < h; dy++) {
    row = 16 - h + dy;
    if (row < 0 || row > 15) continue;
    if (dy == 0) {
      map[row * 32 + (cx & 31)] = (unsigned short)(apex_tile | (bank << 12));
    } else {
      for (dx = -dy; dx <= dy; dx++)
        map[row * 32 + ((cx + dx) & 31)] = (unsigned short)(body_tile | (bank << 12));
    }
  }
}

static void build_bg(void) {
  int i, r, c;

  /* tiles */
  pack_bg_tile(1, art_t_line);
  pack_bg_tile(2, art_t_ground);
  pack_bg_tile(3, art_t_bump);
  pack_bg_tile(4, art_t_peb1);
  pack_bg_tile(5, art_t_peb2);
  pack_bg_tile(6, art_t_peb3);
  pack_bg_tile(7, art_t_peb4);
  pack_bg_tile(8, art_mtn_apex);
  pack_bg_tile(9, art_mtn_body);
  for (i = 0; i < FONT_N; i++) pack_bg_tile(FONT_BASE + i, FONT_ART[i]);

  /* BG0: ground strip (line at row 16 = y128) */
  for (r = 0; r < 32; r++)
    for (c = 0; c < 32; c++) BG0_MAP[r * 32 + c] = 0;
  BG0_MAP[15 * 32 + 5]  = 3; BG0_MAP[15 * 32 + 13] = 3;
  BG0_MAP[15 * 32 + 22] = 3; BG0_MAP[15 * 32 + 27] = 3;
  for (c = 0; c < 32; c++) BG0_MAP[16 * 32 + c] = 1;
  for (r = 17; r < 20; r++)
    for (c = 0; c < 32; c++) {
      int k = (c * 7 + r * 5) % 11;
      BG0_MAP[r * 32 + c] = (k == 0) ? 4 : (k == 5) ? 5 : (k == 7) ? 6 : (k == 3) ? 7 : 2;
    }

  /* BG1: near mountains, BG2: far ridge */
  for (r = 0; r < 32; r++)
    for (c = 0; c < 32; c++) { BG1_MAP[r * 32 + c] = 0; BG2_MAP[r * 32 + c] = 0; }
  stamp_mtn(BG1_MAP, 2, 3, 8, 9, 1);
  stamp_mtn(BG1_MAP, 9, 2, 8, 9, 1);
  stamp_mtn(BG1_MAP, 15, 3, 8, 9, 1);
  stamp_mtn(BG1_MAP, 22, 2, 8, 9, 1);
  stamp_mtn(BG1_MAP, 28, 3, 8, 9, 1);
  stamp_mtn(BG2_MAP, 1, 1, 8, 9, 2);
  stamp_mtn(BG2_MAP, 6, 2, 8, 9, 2);
  stamp_mtn(BG2_MAP, 12, 1, 8, 9, 2);
  stamp_mtn(BG2_MAP, 17, 2, 8, 9, 2);
  stamp_mtn(BG2_MAP, 23, 1, 8, 9, 2);
  stamp_mtn(BG2_MAP, 30, 2, 8, 9, 2);
}

/* ==========================================================================
   SPRITE LOADING (OBJ tiles, 1D mapping)
   ========================================================================== */
static int T_IDLE, T_RUN0, T_RUN1, T_RUN2, T_JUMP, T_DEAD;
static int T_DUCK0, T_DUCK1, T_CACT_S, T_CACT_L, T_PTERO0, T_PTERO1;
static int T_CLOUD, T_SUN, T_MOON, T_STARA, T_STARB, T_DUSTA, T_DUSTB;
static int T_SPARKA, T_SPARKB, T_RESTART, T_DIGITS;

static void load_sprites(void) {
  art_load(art_dino_body, 32, 24);
  art_stamp(art_leg_mid, 32, 8, 0, 24);   T_IDLE = pack_obj(32, 32);
  art_load(art_dino_body, 32, 24);
  art_stamp(art_leg_a,   32, 8, 0, 24);   T_RUN0 = pack_obj(32, 32);
  art_load(art_dino_body, 32, 24);
  art_stamp(art_leg_mid, 32, 8, 0, 24);   T_RUN1 = pack_obj(32, 32);
  art_load(art_dino_body, 32, 24);
  art_stamp(art_leg_b,   32, 8, 0, 24);   T_RUN2 = pack_obj(32, 32);
  art_load(art_dino_body, 32, 24);
  art_stamp(art_leg_jump, 32, 8, 0, 24);  T_JUMP = pack_obj(32, 32);
  art_load(art_dino_body, 32, 24);
  art_stamp(art_leg_mid, 32, 8, 0, 24);
  art_stamp(art_eye_x,    5,  5, 23, 1);  T_DEAD = pack_obj(32, 32);

  art_load(art_duck_body, 32, 12);
  art_stamp(art_duck_leg_a, 32, 4, 0, 12); T_DUCK0 = pack_obj(32, 16);
  art_load(art_duck_body, 32, 12);
  art_stamp(art_duck_leg_b, 32, 4, 0, 12); T_DUCK1 = pack_obj(32, 16);

  art_load(art_cact_s, 16, 32);  T_CACT_S = pack_obj(16, 32);
  art_load(art_cact_l, 32, 32);  T_CACT_L = pack_obj(32, 32);
  art_load(art_ptero_up, 32, 32);  T_PTERO0 = pack_obj(32, 32);
  art_load(art_ptero_down, 32, 32); T_PTERO1 = pack_obj(32, 32);

  art_load(art_cloud, 32, 16);  T_CLOUD = pack_obj(32, 16);
  art_load(art_sun,   32, 32);  T_SUN   = pack_obj(32, 32);
  art_load(art_moon,  16, 16);  T_MOON  = pack_obj(16, 16);
  art_load(art_star_a, 8, 8);   T_STARA = pack_obj(8, 8);
  art_load(art_star_b, 8, 8);   T_STARB = pack_obj(8, 8);
  art_load(art_dust_a, 8, 8);   T_DUSTA = pack_obj(8, 8);
  art_load(art_dust_b, 8, 8);   T_DUSTB = pack_obj(8, 8);
  art_load(art_spark_a, 8, 8);  T_SPARKA = pack_obj(8, 8);
  art_load(art_spark_b, 8, 8);  T_SPARKB = pack_obj(8, 8);
  art_load(art_restart, 16, 16); T_RESTART = pack_obj(16, 16);

  T_DIGITS = obj_off / 32;
  {
    int i;
    for (i = 0; i < 10; i++) { art_load(FONT_ART[14 + i], 8, 8); pack_obj(8, 8); }
    art_load(art_font_H, 8, 8); pack_obj(8, 8);
    art_load(art_font_I, 8, 8); pack_obj(8, 8);
  }
}

/* ==========================================================================
   GAME STATE
   ========================================================================== */
#define OBS_CACT_S 0
#define OBS_CACT_L 1
#define OBS_PTERO  2
#define MAX_OBS 3
#define MAX_P   20
#define MAX_STAR 14
#define MAX_CLOUD 3

typedef struct {
  int active, x, y, w, h, type, count, vx_extra, flap;
} Obstacle;
typedef struct {
  int x, y, vy, is_jumping, is_ducking, dead, anim_tick, land_t, duck_dust;
} Dino;
typedef struct {
  int active, x, y, vx, vy, life, ttl, tile;
} Particle;

static Obstacle obs[MAX_OBS];
static Dino dino;
static Particle parts[MAX_P];

static int star_x[MAX_STAR], star_y[MAX_STAR], star_ph[MAX_STAR];
static int cloud_x[MAX_CLOUD], cloud_y[MAX_CLOUD];
static const int cloud_dep[MAX_CLOUD] = { 8192, 12288, 16384 }; /* 0.125/0.1875/0.25 */

static int score = 0, hi_score = 0;
static int speed_fp = SPEED_INITIAL;
static int ground_dist = 0, hill_dist = 0, far_dist = 0;  /* 16.16 */
static int dist_since_spawn = 0, next_spawn_gap = 350;
static int milestone_blink = 0, game_over = 0, paused = 0;
static int shake_t = 0, flash_t = 0, restart_lock = 0;
static int tick = 0, run_dust_t = 0;
static unsigned int rng = 0x12345678;
static unsigned short keys_held, keys_prev;

static int rand_next(void) {
  rng = rng * 1664525u + 1013904223u;
  rng ^= (unsigned int)tick << 8;
  return (int)(rng >> 16);
}

static int load_hi(void) {
  unsigned int v = SRAM_HI[0] | (SRAM_HI[1] << 8) | (SRAM_HI[2] << 16);
  return (v > 99999) ? 0 : (int)v;
}
static void save_hi(int v) {
  SRAM_HI[0] = (unsigned char)(v & 255);
  SRAM_HI[1] = (unsigned char)((v >> 8) & 255);
  SRAM_HI[2] = (unsigned char)((v >> 16) & 255);
}

static void reset_game(void) {
  int i;
  dino.x = 24; dino.y = INT_TO_FP(GROUND_Y - 32);
  dino.vy = 0; dino.is_jumping = 0; dino.is_ducking = 0;
  dino.dead = 0; dino.anim_tick = 0; dino.land_t = 0; dino.duck_dust = 0;

  score = 0; speed_fp = SPEED_INITIAL;
  ground_dist = 0; hill_dist = 0; far_dist = 0;
  dist_since_spawn = 0; next_spawn_gap = 350;
  milestone_blink = 0; game_over = 0; shake_t = 0; flash_t = 0;
  run_dust_t = 0; jingle_t = -1;

  for (i = 0; i < MAX_OBS; i++) obs[i].active = 0;
  for (i = 0; i < MAX_P; i++) parts[i].active = 0;

  bg_text_clear(6); bg_text_clear(7); bg_text_clear(9);
}

static void init_stars_clouds(void) {
  int i;
  for (i = 0; i < MAX_STAR; i++) {
    star_x[i] = rand_next() % 236;
    star_y[i] = 6 + rand_next() % 84;
    star_ph[i] = rand_next() % 64;
  }
  cloud_x[0] = INT_TO_FP(40);  cloud_y[0] = 22;
  cloud_x[1] = INT_TO_FP(130); cloud_y[1] = 44;
  cloud_x[2] = INT_TO_FP(205); cloud_y[2] = 32;
}

/* ==========================================================================
   PARTICLES (screen space, 8.8 motion)
   ========================================================================== */
static void spawn_part(int x, int y, int vx, int vy, int ttl, int tile) {
  int i;
  for (i = 0; i < MAX_P; i++)
    if (!parts[i].active) {
      parts[i].active = 1; parts[i].x = x << 8; parts[i].y = y << 8;
      parts[i].vx = vx; parts[i].vy = vy;
      parts[i].life = 0; parts[i].ttl = ttl; parts[i].tile = tile;
      return;
    }
}

static void land_dust(int n, int spread) {
  int i;
  for (i = 0; i < n; i++)
    spawn_part(dino.x + 4 + (rand_next() % 14), 126 - (rand_next() % 3),
               -((rand_next() % 384) + 128) - (spread ? 128 : 0),
               -(rand_next() % 96),
               18 + rand_next() % 12,
               (rand_next() & 1) ? T_DUSTA : T_DUSTB);
}

static void death_burst(void) {
  int i;
  for (i = 0; i < 12; i++) {
    int a = rand_next() % 256;
    spawn_part(dino.x + 12, FP_TO_INT(dino.y) + 14,
               ((rand_next() & 1) ? 1 : -1) * (384 + (a & 767)),
               -(256 + (a & 767)),
               30 + rand_next() % 22,
               (i & 1) ? T_SPARKA : T_DUSTA);
  }
}

static void milestone_sparkle(void) {
  int i;
  for (i = 0; i < 5; i++)
    spawn_part(200 + rand_next() % 36, 8 + rand_next() % 10,
               (rand_next() % 512) - 256, -(rand_next() % 128),
               22 + rand_next() % 10, T_SPARKA);
}

static void update_particles(void) {
  int i;
  for (i = 0; i < MAX_P; i++) {
    if (!parts[i].active) continue;
    parts[i].x += parts[i].vx;
    parts[i].y += parts[i].vy;
    parts[i].vy += 10;
    if (++parts[i].life >= parts[i].ttl) parts[i].active = 0;
  }
}

/* ==========================================================================
   SPAWN + COLLISION (Chromium spacing logic kept)
   ========================================================================== */
static void spawn_obstacle(void) {
  int r, idx = -1, i;
  if (FP_TO_INT(dist_since_spawn) < next_spawn_gap) return;
  for (i = 0; i < MAX_OBS; i++)
    if (!obs[i].active) { idx = i; break; }
  if (idx == -1) return;

  obs[idx].active = 1;
  obs[idx].x = INT_TO_FP(SCREEN_W + 8);
  obs[idx].flap = 0;
  obs[idx].vx_extra = 0;

  r = rand_next() % 100;
  if (score > 350 && r < 30) {              /* pterodactyl: 3 altitudes */
    int alt = rand_next() % 3;
    obs[idx].type = OBS_PTERO;
    obs[idx].w = 20; obs[idx].h = 12;
    if (alt == 0)      obs[idx].y = GROUND_Y - 26;  /* low: duck under  */
    else if (alt == 1) obs[idx].y = GROUND_Y - 12;  /* mid: jump over   */
    else               obs[idx].y = GROUND_Y - 48;  /* high: run under  */
    obs[idx].vx_extra = -(rand_next() % 3) * 16384;
  } else if (r < 72) {                      /* small cactus cluster 1-3 */
    int n = 1 + rand_next() % 3;
    obs[idx].type = OBS_CACT_S;
    obs[idx].count = n;
    obs[idx].w = 16 + (n - 1) * 14;
    obs[idx].h = 24;
    obs[idx].y = GROUND_Y - 24;
  } else {                                  /* large cactus cluster 1-2 */
    int n = 1 + (rand_next() % 4 == 0);
    obs[idx].type = OBS_CACT_L;
    obs[idx].count = n;
    obs[idx].w = 32 + (n - 1) * 26;
    obs[idx].h = 28;
    obs[idx].y = GROUND_Y - 28;
  }

  dist_since_spawn = 0;
  next_spawn_gap = 220 + rand_next() % 140;
}

static int check_collision(Obstacle* ob) {
  int ox = FP_TO_INT(ob->x), oy = ob->y;
  int dx, dy, dw, dh;
  if (dino.is_ducking) {
    dx = dino.x + 4; dy = FP_TO_INT(dino.y) + 6; dw = 24; dh = 10;
  } else {
    dx = dino.x + 8; dy = FP_TO_INT(dino.y) + 4; dw = 16; dh = 26;
  }
  return (dx < ox + ob->w - 2 && dx + dw > ox + 2 &&
          dy < oy + ob->h - 2 && dy + dh > oy + 2);
}

/* ==========================================================================
   UPDATE
   ========================================================================== */
static void update_gameplay(void) {
  unsigned short keys = keys_held;
  int grav, floor_y, prev_vy, i, prev_score;

  eval_theme((score % 700) * 65536 / 700);

  /* variable-height jump (hold = full, release = short hop) */
  if (keys & (KEY_A | KEY_B | KEY_UP)) {
    if (!dino.is_jumping && !dino.is_ducking) {
      dino.vy = JUMP_VEL; dino.is_jumping = 1; snd_jump();
    }
  } else if (dino.is_jumping && dino.vy < JUMP_DROP_VEL) {
    dino.vy = JUMP_DROP_VEL;
  }

  prev_vy = dino.vy;

  grav = GRAVITY;
  if (keys & KEY_DOWN) {
    if (dino.is_jumping) grav = FAST_FALL_G;
    else if (!dino.is_ducking) { dino.is_ducking = 1; dino.duck_dust = 1; }
  } else {
    dino.is_ducking = 0;
  }
  if (dino.duck_dust && dino.is_ducking) { dino.duck_dust = 0; land_dust(3, 0); }

  dino.vy += grav;
  dino.y += dino.vy;

  floor_y = INT_TO_FP(dino.is_ducking ? (GROUND_Y - 16) : (GROUND_Y - 32));
  if (!dino.is_jumping) {
    dino.y = floor_y;              /* grounded: snap (handles duck/stand) */
  } else if (dino.y >= floor_y) {
    int fast = prev_vy > 131072;   /* impact faster than 2 px/f */
    dino.y = floor_y;
    dino.land_t = fast ? 12 : 8;
    land_dust(fast ? 8 : 4, fast);
    if (prev_vy > 196608) { snd_land(); shake_t = 4; }
    dino.vy = 0; dino.is_jumping = 0;
  }
  if (dino.land_t > 0) dino.land_t--;

  /* obstacles */
  for (i = 0; i < MAX_OBS; i++) {
    if (!obs[i].active) continue;
    obs[i].x -= speed_fp + obs[i].vx_extra;
    if (FP_TO_INT(obs[i].x) < -64) { obs[i].active = 0; continue; }
    if (obs[i].type == OBS_PTERO && (tick % 10) == 0) obs[i].flap ^= 1;
    if (check_collision(&obs[i])) {
      dino.dead = 1; game_over = 1;
      snd_death();
      if (score > hi_score) { hi_score = score; save_hi(hi_score); }
      shake_t = 14; flash_t = 4; restart_lock = 25;
      death_burst();
      bg_text("GAME OVER", 84, 6, 1);
      return;
    }
  }

  dist_since_spawn += speed_fp;
  spawn_obstacle();

  /* clouds */
  for (i = 0; i < MAX_CLOUD; i++) {
    cloud_x[i] -= ((speed_fp * cloud_dep[i]) >> 16) + 180;
    if (FP_TO_INT(cloud_x[i]) < -34) {
      cloud_x[i] = INT_TO_FP(SCREEN_W + (rand_next() % 50));
      cloud_y[i] = 16 + rand_next() % 46;
    }
  }

  /* speed, parallax distances */
  if (speed_fp < SPEED_MAX) speed_fp += SPEED_ACCEL;
  ground_dist += speed_fp;
  hill_dist += (speed_fp * 29491) >> 16;
  far_dist  += (speed_fp * 13107) >> 16;

  /* score */
  prev_score = score;
  score = FP_TO_INT(ground_dist) / 16;
  if (score / 100 > prev_score / 100) {
    snd_point(); milestone_blink = 60; milestone_sparkle();
  }
  if (milestone_blink > 0) milestone_blink--;

  /* running dust */
  if (!dino.is_jumping && ++run_dust_t >= 13) {
    run_dust_t = 0;
    spawn_part(dino.x + 6 + (dino.is_ducking ? 8 : 0), 125,
               -((speed_fp >> 9) + 128) - (rand_next() % 64), -80,
               14 + rand_next() % 8, T_DUSTB);
  }

  dino.anim_tick++;
  update_particles();
  if (shake_t > 0) shake_t--;
  if (flash_t > 0) flash_t--;
  update_jingle();
}

static void update_gameover(void) {
  if (restart_lock > 0) restart_lock--;
  else if (keys_held & (KEY_A | KEY_START)) { reset_game(); return; }
  bg_text("PRESS START", 76, 9, ((tick >> 4) & 1));
  update_particles();
  if (shake_t > 0) shake_t--;
  if (flash_t > 0) flash_t--;
  update_jingle();
}

/* ==========================================================================
   OAM BUILD + RENDER
   ========================================================================== */
static void build_oam(void) {
  int i, n;
  int shk_x = 0, shk_y = 0;
  if (shake_t > 0) {
    shk_x = (rand_next() & 7) - 4;
    shk_y = (rand_next() & 3) - 2;
    if (shake_t < 4) { shk_x >>= 1; shk_y >>= 1; }
  }

  /* ---- dino: slot 0, affine squash & stretch ---- */
  if (!dino.is_ducking || dino.is_jumping) {
    int sy = 256, sx = 256, h, w, ax, ay, tile;
    if (dino.dead) tile = T_DEAD;
    else if (dino.is_jumping) tile = T_JUMP;
    else {
      int period = 9 - (FP_TO_INT(speed_fp) << 1);
      int ph;
      if (period < 3) period = 3;
      ph = (dino.anim_tick / period) & 3;
      tile = (ph == 0) ? T_RUN0 : (ph == 2) ? T_RUN2 : T_RUN1;
    }
    if (!dino.dead) {
      if (dino.land_t > 0) { sy = 256 - dino.land_t * 5; sx = 256 + dino.land_t * 4; }
      else if (dino.is_jumping) {
        int k = (-dino.vy) >> 16;
        if (k < 0) k = 0; if (k > 3) k = 3;
        sy = 256 + k * 8; sx = 256 - k * 5;
      }
    }
    h = (32 * sy) >> 8; w = (32 * sx) >> 8;
    ax = dino.x + ((32 - w) >> 1) - 16 + shk_x;
    ay = FP_TO_INT(dino.y) - 16 +
         (dino.is_jumping ? ((32 - h) >> 1) : (32 - h)) + shk_y;
    obj_set(0, ax, ay, ATTR0_SQ, ATTR1_SIZE(2), tile, 0, 1);
    oam_sh[0].a0 |= ATTR0_AFFINE_DBL;
    oam_sh[0].a1 |= ATTR1_AFF_GRP(0);
    oam_sh[0].a3 = (unsigned short)sx;   /* pa */
    oam_sh[1].a3 = 0;                    /* pb */
    oam_sh[2].a3 = 0;                    /* pc */
    oam_sh[3].a3 = (unsigned short)sy;   /* pd */
  } else {
    int tile = ((dino.anim_tick >> 3) & 1) ? T_DUCK1 : T_DUCK0;
    obj_set(0, dino.x + shk_x, FP_TO_INT(dino.y) + shk_y,
            ATTR0_WIDE, ATTR1_SIZE(2), tile, 0, 1);
    oam_sh[0].a3 = 256; oam_sh[1].a3 = 0; oam_sh[2].a3 = 0; oam_sh[3].a3 = 256;
  }

  /* ---- obstacles: slots 4-12 ---- */
  n = 4;
  for (i = 0; i < MAX_OBS; i++) {
    Obstacle* ob = &obs[i];
    if (!ob->active) continue;
    {
      int ox = FP_TO_INT(ob->x), j;
      if (ob->type == OBS_CACT_S) {
        for (j = 0; j < ob->count; j++)
          obj_set(n++, ox + j * 14 + shk_x, GROUND_Y - 32 + shk_y,
                  ATTR0_TALL, ATTR1_SIZE(2), T_CACT_S, 1, 1);
      } else if (ob->type == OBS_CACT_L) {
        for (j = 0; j < ob->count; j++)
          obj_set(n++, ox + j * 26 + shk_x, GROUND_Y - 32 + shk_y,
                  ATTR0_SQ, ATTR1_SIZE(2), T_CACT_L, 1, 1);
      } else {
        int bob = (tick >> 3) & 1;
        obj_set(n++, ox - 5 + shk_x, ob->y - 11 - bob + shk_y,
                ATTR0_SQ, ATTR1_SIZE(2), ob->flap ? T_PTERO0 : T_PTERO1, 2, 1);
      }
    }
  }
  for (i = n; i < 13; i++) obj_hide(i);

  /* ---- clouds: slots 13-15 ---- */
  for (i = 0; i < MAX_CLOUD; i++)
    obj_set(13 + i, FP_TO_INT(cloud_x[i]) + shk_x, cloud_y[i] + shk_y,
            ATTR0_WIDE, ATTR1_SIZE(2), T_CLOUD, 3, 2);

  /* ---- sun (16) + moon (17), driven by cycle position ---- */
  {
    int p = (score % 700) * 65536 / 700;
    int sun_vis = 1, sun_y = 30, moon_vis = 0, moon_y = 26;
    if (p > 43254) sun_y = 30 + ((p - 43254) * 130) / 9175;
    if (sun_y > 150) sun_vis = 0;
    if (p >= 48497) {
      moon_vis = 1;
      if (p < 53739)      moon_y = 150 - ((p - 48497) * 124) / 5242;
      else if (p < 58982) moon_y = 26;
      else                moon_y = 26 + ((p - 58982) * 124) / 3933;
      if (moon_y > 150) moon_vis = 0;
    }
    if (sun_vis) obj_set(16, 188 + shk_x, sun_y + shk_y, ATTR0_SQ, ATTR1_SIZE(2), T_SUN, 3, 2);
    else obj_hide(16);
    if (moon_vis) obj_set(17, 196 + shk_x, moon_y + shk_y, ATTR0_SQ, ATTR1_SIZE(1), T_MOON, 3, 2);
    else obj_hide(17);
  }

  /* ---- stars: slots 18-31 ---- */
  if (night_amt > 15) {
    for (i = 0; i < MAX_STAR; i++) {
      int tl = ((((tick >> 3) + star_ph[i]) & 15) < 2) ? T_STARB : T_STARA;
      obj_set(18 + i, star_x[i] + shk_x, star_y[i] + shk_y,
              ATTR0_SQ, ATTR1_SIZE(0), tl, 3, 2);
    }
  } else {
    for (i = 0; i < MAX_STAR; i++) obj_hide(18 + i);
  }

  /* ---- particles: slots 32-51 ---- */
  n = 32;
  for (i = 0; i < MAX_P; i++) {
    if (!parts[i].active) continue;
    {
      int t = parts[i].tile;
      if (parts[i].life * 3 > parts[i].ttl * 2) {
        if (t == T_DUSTA) t = T_DUSTB;
        else if (t == T_SPARKA) t = T_SPARKB;
      }
      obj_set(n++, (parts[i].x >> 8) + shk_x, (parts[i].y >> 8) + shk_y,
              ATTR0_SQ, ATTR1_SIZE(0), t, 3, 1);
    }
  }
  for (i = n; i < 52; i++) obj_hide(i);

  /* ---- UI: HI (52-58, dim) + score (59-63, bright) ---- */
  {
    int v, d;
    obj_set(52, 120, 8, ATTR0_SQ, ATTR1_SIZE(0), T_DIGITS + 10, 5, 0);
    obj_set(53, 128, 8, ATTR0_SQ, ATTR1_SIZE(0), T_DIGITS + 11, 5, 0);
    v = hi_score;
    for (d = 4; d >= 0; d--) {
      obj_set(54 + d, 144 + d * 8, 8, ATTR0_SQ, ATTR1_SIZE(0), T_DIGITS + (v % 10), 5, 0);
      v /= 10;
    }
    if (milestone_blink & 8) {
      for (d = 0; d < 5; d++) obj_hide(59 + d);
    } else {
      v = score;
      for (d = 4; d >= 0; d--) {
        obj_set(59 + d, 192 + d * 8, 8, ATTR0_SQ, ATTR1_SIZE(0), T_DIGITS + (v % 10), 4, 0);
        v /= 10;
      }
    }
  }

  /* ---- restart icon: slot 64 ---- */
  if (game_over) obj_set(64, 112, 74 + ((tick >> 4) & 1), ATTR0_SQ, ATTR1_SIZE(1), T_RESTART, 4, 0);
  else obj_hide(64);

  obj_hide(1); obj_hide(2); obj_hide(3);
  for (i = 65; i < 128; i++) obj_hide(i);
}

/* ==========================================================================
   MAIN
   ========================================================================== */
int main(void) {
  int i;

  *REG_DISPCNT = DISP_BLANK;
  *REG_WAITCNT = (unsigned short)(*REG_WAITCNT | 0x0003);

  load_sprites();
  build_bg();

  for (i = 0; i < 128; i++) {
    oam_sh[i].a0 = ATTR0_HIDE; oam_sh[i].a1 = 0; oam_sh[i].a2 = 0; oam_sh[i].a3 = 0;
  }
  oam_sh[0].a3 = 256; oam_sh[3].a3 = 256;   /* identity affine params */

  init_sound();
  hi_score = load_hi();

  rng = 0x12345678u ^ ((unsigned int)(*REG_VCOUNT) << 8);
  init_stars_clouds();
  reset_game();

  eval_theme(0);
  build_sky();
  apply_palettes(0);

  *REG_BG0CNT = 0x1C01;   /* prio 1, char base 0, screen base 28 */
  *REG_BG1CNT = 0x1A01;   /* prio 1, screen base 26 */
  *REG_BG2CNT = 0x1802;   /* prio 2, screen base 24 */
  *REG_BG0VOFS = 0; *REG_BG1VOFS = 0; *REG_BG2VOFS = 0;
  *REG_BG0HOFS = 0; *REG_BG1HOFS = 0; *REG_BG2HOFS = 0;

  *REG_DISPCNT = (unsigned short)(DISP_MODE0 | DISP_OBJ_1D | DISP_BG0 |
                                  DISP_BG1 | DISP_BG2 | DISP_OBJ);

  /* HDMA sky gradient: one halfword per HBlank into the backdrop color */
  *REG_DMA3SAD = (unsigned int)sky_lines;
  *REG_DMA3DAD = 0x05000000;
  *REG_DMA3CNT = 0x81220001;   /* enable | HBlank | repeat | dst fixed | 1 */

  while (1) {
    while (*REG_VCOUNT >= 160);
    while (*REG_VCOUNT < 160);

    tick++;
    keys_prev = keys_held;
    keys_held = (unsigned short)(~(*REG_KEYINPUT) & 0x03FF);

    if ((keys_held & KEY_START) && !(keys_prev & KEY_START) && !game_over) {
      paused ^= 1;
      bg_text("PAUSED", 96, 7, paused);
    }

    if (game_over) update_gameover();
    else if (!paused) update_gameplay();
    else if ((tick & 31) == 0) bg_text("PAUSED", 96, 7, ((tick >> 5) & 1));

    if (!game_over && !paused) build_sky();
    apply_palettes(flash_t > 0);

    build_oam();

    /* push shadow OAM -> OAM via DMA1, 256 words, 32-bit */
    *REG_DMA1SAD = (unsigned int)oam_sh;
    *REG_DMA1DAD = 0x07000000;
    *REG_DMA1CNT = DMA_ENABLE | 0x00000400 | 256;

    {
      int shk = (shake_t > 0) ? ((rand_next() & 7) - 4) : 0;
      *REG_BG0HOFS = (unsigned short)(FP_TO_INT(ground_dist) + shk);
      *REG_BG1HOFS = (unsigned short)(FP_TO_INT(hill_dist) + shk / 2);
      *REG_BG2HOFS = (unsigned short)(FP_TO_INT(far_dist) + shk / 3);
    }

    /* re-latch HDMA every frame so the sky buffer stays scanline-aligned */
    *REG_DMA3CNT = 0;
    *REG_DMA3SAD = (unsigned int)sky_lines;
    *REG_DMA3DAD = 0x05000000;
    *REG_DMA3CNT = 0x81220001;
  }
  return 0;
}

/* ==========================================================================
 * CHROME DINO GBA: HARDWARE MODE 0 EDITION
 * - True Hardware Sprites (OAM) & 1D Tile Mapping
 * - Zero-CPU Hardware Parallax Scrolling
 * - 4-Frame Fluid Run Cycle & Day/Night Color Interpolation
 * ========================================================================== */

// --- Hardware Registers ---
#define REG_DISPCNT     *(volatile unsigned int*)0x04000000
#define REG_VCOUNT      *(volatile unsigned short*)0x04000006
#define REG_KEYINPUT    *(volatile unsigned short*)0x04000130
#define REG_BG1CNT      *(volatile unsigned short*)0x0400000A
#define REG_BG1HOFS     *(volatile unsigned short*)0x04000014

#define REG_DMA3SAD     *(volatile unsigned int*)0x040000D4
#define REG_DMA3DAD     *(volatile unsigned int*)0x040000D8
#define REG_DMA3CNT     *(volatile unsigned int*)0x040000DC

// --- Sound Registers ---
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
#define KEY_A (1 << 0)
#define KEY_UP (1 << 6)
#define KEY_DOWN (1 << 7)
#define KEY_START (1 << 3)

#define RGB15(r, g, b) ((r) | ((g) << 5) | ((b) << 10))

#define GROUND_Y 128
#define SCREEN_W 240
#define SCREEN_H 160

// 16.16 Fixed Point Math
#define FP_SHIFT 16
#define INT_TO_FP(x) ((x) << FP_SHIFT)
#define FP_TO_INT(x) ((x) >> FP_SHIFT)

// Chromium 1:1 Physics
#define SPEED_INITIAL 157286   // 2.40 px/f
#define SPEED_MAX 340787       // 5.20 px/f
#define SPEED_ACCEL 26         // +0.0004 px/f
#define GRAVITY 16384          // 0.25 px/f^2
#define JUMP_INITIAL_VEL -275251 
#define JUMP_DROP_VEL -137626
#define FAST_FALL_GRAV 49152   // 3x Gravity

// --- OAM (Object Attribute Memory) ---
typedef struct {
    unsigned short attr0;
    unsigned short attr1;
    unsigned short attr2;
    short pad;
} OBJ_ATTR;

OBJ_ATTR obj_buffer[128];

#define ATTR0_Y_MASK        0x00FF
#define ATTR0_SHAPE_SQUARE  (0<<14)
#define ATTR0_SHAPE_WIDE    (1<<14)
#define ATTR0_SHAPE_TALL    (2<<14)

#define ATTR1_X_MASK        0x01FF
#define ATTR1_SIZE_8        (0<<14)
#define ATTR1_SIZE_16       (1<<14)
#define ATTR1_SIZE_32       (2<<14)
#define ATTR1_SIZE_64       (3<<14)

// ==========================================================================
// ASCII-ART COMPACT SPRITE LOADER
// ==========================================================================
// 1 = Dark Blue/Grey, 2 = Chrome Blue, 3 = White, 4 = Dark Green, 5 = Light Green
// 6 = Dark Grey, 7 = Light Grey, 8 = Red (Dead), C = Cloud White, D = Font

const char* art_dino_stand =
".......1111111......"
"......1223222211...."
"......1222222221...."
"......1222221111...."
"......122222221....."
"..11..12211111......"
".121..121..........."
"1221111121.........."
"12222222221........."
"122222222221........"
"1222222222221......."
".1222222222221......"
"..122222222221......"
"...1222222221......."
"....11222221........"
"......11111........."
"......1...1........."
"......1...1........."
".....11...1........."
"..........11........";

const char* art_dino_run1 =
".......1111111......"
"......1223222211...."
"......1222222221...."
"......1222221111...."
"......122222221....."
"..11..12211111......"
".121..121..........."
"1221111121.........."
"12222222221........."
"122222222221........"
"1222222222221......."
".1222222222221......"
"..122222222221......"
"...1222222221......."
"....11222221........"
"......11111........."
"......1...1........."
".....11....1........"
".....1......11......"
"....11..............";

const char* art_dino_run2 =
".......1111111......"
"......1223222211...."
"......1222222221...."
"......1222221111...."
"......122222221....."
"..11..12211111......"
".121..121..........."
"1221111121.........."
"12222222221........."
"122222222221........"
"1222222222221......."
".1222222222221......"
"..122222222221......"
"...1222222221......."
"....11222221........"
"......11111........."
".......1.1.........."
".......1.1.........."
".......11.11........"
"....................";

const char* art_dino_run3 =
".......1111111......"
"......1223222211...."
"......1222222221...."
"......1222221111...."
"......122222221....."
"..11..12211111......"
".121..121..........."
"1221111121.........."
"12222222221........."
"122222222221........"
"1222222222221......."
".1222222222221......"
"..122222222221......"
"...1222222221......."
"....11222221........"
"......11111........."
"......1...1........."
".......1...11......."
"......11............"
".......11...........";

const char* art_dino_duck1 = 
"............................"
"...................1111111.."
"..11...............12232211."
".121......11111111122222221."
"122111111122222222222222111."
"1222222222222222222222221..."
"122222222222222222222221...."
".1111122222222222222111....."
"......11111111111111........"
"......1.......1............."
".....11........1............"
"................11..........";

const char* art_dino_duck2 = 
"............................"
"...................1111111.."
"..11...............12232211."
".121......11111111122222221."
"122111111122222222222222111."
"1222222222222222222222221..."
"122222222222222222222221...."
".1111122222222222222111....."
"......11111111111111........"
".......1......1............."
".......11.....1............."
"...............11...........";

const char* art_dino_dead =
".......1111111......"
"......1283282211...."
"......1222222221...."
"......1222221111...."
"......122222221....."
"..11..12211111......"
".121..121..........."
"1221111121.........."
"12222222221........."
"122222222221........"
"1222222222221......."
".1222222222221......"
"..122222222221......"
"...1222222221......."
"....11222221........"
"......11111........."
"......1...1........."
"......1...1........."
".....11...1........."
"..........11........";

const char* art_cac_sm =
"....444........."
"....454........."
".44.454.44......"
"454.454.454....."
"45444544454....."
"45555555554....."
".444454444......"
"....454........."
"....454........."
"....454........."
"....454........."
"....444.........";

const char* art_cac_lg =
"........444............."
"........454............."
"........454...444......."
"...444..454...454......."
"...454..454...454......."
"...454..454...454......."
"...45444454444454......."
"...45555555555554......."
"....444445444444........"
"........454............."
"........454............."
"........454............."
"........444.............";

const char* art_ptero_1 =
".......666......................"
"......6776......................"
".....6776......................."
"....6776........................"
"...6776............66..........."
"..6776...........6676..........."
".6776666666666666776............"
"6777777777777777776............."
"666666666666666666.............."
".....6776......................."
"......6776......................"
".......666......................";

const char* art_ptero_2 =
"................................"
"...................66..........."
".................6676..........."
".6666666666666666776............"
"6777777777777777776............."
"666666666666666666.............."
".....6776......................."
"....6776........................"
"...6776........................."
"..6776.........................."
".6776..........................."
"666.............................";

const char* art_cloud =
"........CCCC............"
"......CCCCCCCC.........."
"....CCCCCCCCCCCC........"
"..CCCCCCCCCCCCCCCC......"
".CCCCCCCCCCCCCCCCCC....."
"CCCCCCCCCCCCCCCCCCCC....";

static const unsigned char font3x5[10][5] = {
    {0x7,0x5,0x5,0x5,0x7}, {0x2,0x6,0x2,0x2,0x7}, {0x7,0x1,0x7,0x4,0x7},
    {0x7,0x1,0x7,0x1,0x7}, {0x5,0x5,0x7,0x1,0x1}, {0x7,0x4,0x7,0x1,0x7},
    {0x7,0x4,0x7,0x5,0x7}, {0x7,0x1,0x2,0x4,0x4}, {0x7,0x5,0x7,0x5,0x7},
    {0x7,0x5,0x7,0x1,0x7}
};

// ==========================================================================
// RENDER SETUP
// ==========================================================================

void load_art(int tile_idx, int w_pixels, int h_pixels, const char* art, int sprite_w_tiles, int sprite_h_tiles) {
    unsigned int* vram = (unsigned int*)(0x06010000 + (tile_idx * 32));
    for (int i = 0; i < (sprite_w_tiles * sprite_h_tiles * 8); i++) vram[i] = 0; // Clear memory
    
    for (int y = 0; y < h_pixels; y++) {
        for (int x = 0; x < w_pixels; x++) {
            char c = art[y * w_pixels + x];
            if (c == '.' || c == ' ') continue;
            int col = (c >= '0' && c <= '9') ? (c - '0') : (c - 'A' + 10);
            
            int tx = x / 8;
            int ty = y / 8;
            int px = x % 8;
            int py = y % 8;
            
            unsigned int* tile = vram + ((ty * sprite_w_tiles + tx) * 8);
            unsigned int row = tile[py];
            row &= ~(0xF << (px * 4)); // Clear pixel
            row |= (col << (px * 4));  // Set pixel
            tile[py] = row;
        }
    }
}

void load_font() {
    unsigned int* vram = (unsigned int*)(0x06010000 + (200 * 32));
    for (int i = 0; i < 10; i++) {
        for(int j=0; j<8; j++) vram[i*8 + j] = 0;
        for (int y = 0; y < 5; y++) {
            unsigned int row = 0;
            for (int x = 0; x < 3; x++) {
                if (font3x5[i][y] & (1 << (2 - x))) {
                    row |= (13 << (x * 4)); // Color 13 (Font)
                }
            }
            vram[i * 8 + y + 1] = row; // +1 to center vertically
        }
    }
}

void init_graphics() {
    // Enable Mode 0, 1D OBJ Mapping, OBJ Enable, BG1 Enable
    REG_DISPCNT = 0x1340; 
    
    // BG1 Setup (ScreenBlock 31, CharBlock 0, 4bpp)
    REG_BG1CNT = (31 << 8); 

    // Setup Palettes
    unsigned short* bg_pal = (unsigned short*)0x05000000;
    bg_pal[0]  = RGB15(16, 24, 31); // Sky Day
    bg_pal[1]  = RGB15(28, 26, 22); // Sand Light
    bg_pal[2]  = RGB15(24, 21, 16); // Sand Dark

    unsigned short* obj_pal = (unsigned short*)0x05000200;
    obj_pal[1] = RGB15(8, 12, 16);  // Dino Outline
    obj_pal[2] = RGB15(10, 16, 28); // Chrome Blue
    obj_pal[3] = RGB15(31, 31, 31); // Eye White
    obj_pal[4] = RGB15(6, 16, 8);   // Cactus Dark
    obj_pal[5] = RGB15(12, 24, 14); // Cactus Light
    obj_pal[6] = RGB15(12, 12, 14); // Ptero Dark
    obj_pal[7] = RGB15(20, 20, 22); // Ptero Light
    obj_pal[8] = RGB15(31, 5, 5);   // Dead Red
    obj_pal[12] = RGB15(31, 31, 31); // Cloud White
    obj_pal[13] = RGB15(8, 8, 8);   // Font Grey

    // Generate Background Map & Tiles
    unsigned int* bg_vram = (unsigned int*)0x06000000;
    for(int i=0; i<8; i++) bg_vram[i] = 0;             // Sky
    for(int i=0; i<8; i++) bg_vram[8+i] = 0x11111111;  // Sand Solid
    for(int i=0; i<8; i++) bg_vram[16+i] = 0x11111111; // Sand Detail
    bg_vram[16+2] = 0x11211111;
    
    unsigned short* bg_map = (unsigned short*)0x0600F800; // SB 31
    for (int y = 0; y < 32; y++) {
        for (int x = 0; x < 32; x++) {
            if (y < (GROUND_Y / 8)) bg_map[y * 32 + x] = 0;
            else if (y == (GROUND_Y / 8)) bg_map[y * 32 + x] = ((x * y) % 4 == 0) ? 2 : 1;
            else bg_map[y * 32 + x] = 1;
        }
    }

    // Load OAM Graphics (1D Mapping Layout)
    load_art(0,  20, 20, art_dino_stand, 4, 4);   // Tile 0
    load_art(16, 20, 20, art_dino_run1, 4, 4);    // Tile 16
    load_art(32, 20, 20, art_dino_run2, 4, 4);    // Tile 32
    load_art(48, 20, 20, art_dino_run3, 4, 4);    // Tile 48
    load_art(64, 20, 20, art_dino_run2, 4, 4);    // Tile 64 (Re-uses run2 for 4th frame!)
    load_art(80, 28, 12, art_dino_duck1, 4, 2);   // Tile 80
    load_art(96, 28, 12, art_dino_duck2, 4, 2);   // Tile 96
    load_art(112, 20, 20, art_dino_dead, 4, 4);   // Tile 112
    
    load_art(128, 32, 12, art_ptero_1, 4, 2);     // Tile 128
    load_art(136, 32, 12, art_ptero_2, 4, 2);     // Tile 136
    
    load_art(144, 16, 12, art_cac_sm, 2, 4);      // Tile 144
    load_art(152, 24, 13, art_cac_lg, 4, 4);      // Tile 152
    
    load_art(168, 24, 6, art_cloud, 4, 2);        // Tile 168
    load_font();                                  // Tile 200+
}

inline void set_obj(int index, int x, int y, int shape, int size, int tile) {
    obj_buffer[index].attr0 = (y & ATTR0_Y_MASK) | shape;
    obj_buffer[index].attr1 = (x & ATTR1_X_MASK) | size;
    obj_buffer[index].attr2 = tile;
}

inline void hide_obj(int index) { obj_buffer[index].attr0 = 160; }

void draw_score_objs(int start_idx, int x, int y, int val) {
    int temp = val;
    for(int i = 4; i >= 0; i--) {
        set_obj(start_idx + i, x + i * 8, y, ATTR0_SHAPE_SQUARE, ATTR1_SIZE_8, 200 + (temp % 10));
        temp /= 10;
    }
}

// ==========================================================================
// GAME STATE & AUDIO
// ==========================================================================

#define OBS_CACTUS_SM 0
#define OBS_CACTUS_LG 1
#define OBS_PTERO 2
#define MAX_OBS 4
#define MAX_CLOUDS 3

typedef struct { int active, x, y, type, w, h, frame; } Obstacle;
Obstacle obstacles[MAX_OBS];

typedef struct { int x, y, vy, is_jumping, is_ducking, dead, tick; } Dino;
Dino dino;

int speed_fp, distance_ran_fp, dist_since_last_spawn_fp, next_spawn_gap;
int score, high_score, game_over;
int ground_scroll;
int is_night;

int cloud_x[MAX_CLOUDS] = {80, 180, 260};
int cloud_y[MAX_CLOUDS] = {30, 50, 20};

static unsigned int rng = 0x12345678;
static inline unsigned int rand_next() { rng = rng * 1664525u + 1013904223u; return (rng >> 16); }

static inline void snd_jump()  { REG_SOUND1CNT_L = 0; REG_SOUND1CNT_H = 0xA180; REG_SOUND1CNT_X = 0x8680; }
static inline void snd_point() { REG_SOUND2CNT_L = 0xF180; REG_SOUND2CNT_H = 0x8740; }
static inline void snd_death() { REG_SOUND4CNT_L = 0xC200; REG_SOUND4CNT_H = 0x8052; }

void reset_game() {
    is_night = 0;
    dino.x = 24; dino.y = INT_TO_FP(GROUND_Y - 20);
    dino.vy = 0; dino.is_jumping = 0; dino.is_ducking = 0; 
    dino.dead = 0; dino.tick = 0;
    distance_ran_fp = dist_since_last_spawn_fp = 0;
    next_spawn_gap = 350;
    score = ground_scroll = game_over = 0;
    speed_fp = SPEED_INITIAL;
    for (int i = 0; i < MAX_OBS; i++) obstacles[i].active = 0;
}

// Hardware color interpolation for Day/Night cycle
static int sky_r = 16, sky_g = 24, sky_b = 31;
static int sky_tick = 0;
void update_sky() {
    int tr = is_night ? 2 : 16;
    int tg = is_night ? 2 : 24;
    int tb = is_night ? 6 : 31;

    if (++sky_tick > 5) {
        sky_tick = 0;
        if (sky_r < tr) sky_r++; else if (sky_r > tr) sky_r--;
        if (sky_g < tg) sky_g++; else if (sky_g > tg) sky_g--;
        if (sky_b < tb) sky_b++; else if (sky_b > tb) sky_b--;
        ((unsigned short*)0x05000000)[0] = RGB15(sky_r, sky_g, sky_b);
    }
}

// ==========================================================================
// MAIN LOOP
// ==========================================================================

int main(void) {
    REG_SOUNDCNT_X = 0x0080; REG_SOUNDCNT_L = 0x7777; REG_SOUNDCNT_H = 0x0002;
    init_graphics();
    reset_game();

    while (1) {
        unsigned short keys = ~REG_KEYINPUT;

        if (!game_over) {
            is_night = (score % 700) >= 500;
            update_sky();

            // Physics (Chromium)
            if ((keys & KEY_A) || (keys & KEY_UP)) {
                if (!dino.is_jumping && !dino.is_ducking) {
                    dino.vy = JUMP_INITIAL_VEL; dino.is_jumping = 1; snd_jump();
                }
            } else if (dino.is_jumping && dino.vy < JUMP_DROP_VEL) {
                dino.vy = JUMP_DROP_VEL;
            }

            int grav = (keys & KEY_DOWN) ? (dino.is_jumping ? FAST_FALL_GRAV : GRAVITY) : GRAVITY;
            dino.is_ducking = (keys & KEY_DOWN) && !dino.is_jumping;
            
            dino.vy += grav;
            dino.y += dino.vy;
            
            int floor_y = INT_TO_FP(dino.is_ducking ? GROUND_Y - 12 : GROUND_Y - 20);
            if (dino.y >= floor_y) {
                dino.y = floor_y; dino.vy = 0; dino.is_jumping = 0;
            }

            // Move Environment
            ground_scroll += FP_TO_INT(speed_fp);
            for(int i = 0; i < MAX_CLOUDS; i++) {
                cloud_x[i] -= 1;
                if (cloud_x[i] < -32) {
                    cloud_x[i] = 240 + (rand_next() % 40); 
                    cloud_y[i] = 10 + (rand_next() % 40);
                }
            }

            // Obstacles
            for (int i = 0; i < MAX_OBS; i++) {
                if (obstacles[i].active) {
                    obstacles[i].x -= speed_fp;
                    if (FP_TO_INT(obstacles[i].x) < -32) obstacles[i].active = 0;
                    
                    if (obstacles[i].type == OBS_PTERO && dino.tick % 10 == 0) 
                        obstacles[i].frame ^= 1;

                    // Tight Hitboxes
                    int ox = FP_TO_INT(obstacles[i].x), oy = obstacles[i].y;
                    int dx = dino.x + 4, dy = FP_TO_INT(dino.y) + 2;
                    int dw = dino.is_ducking ? 20 : 12, dh = dino.is_ducking ? 10 : 18;
                    
                    if (dx < ox + obstacles[i].w - 2 && dx + dw > ox + 2 &&
                        dy < oy + obstacles[i].h - 2 && dy + dh > oy + 2) {
                        dino.dead = 1; game_over = 1; snd_death();
                        if (score > high_score) high_score = score;
                    }
                }
            }

            // Spawning
            dist_since_last_spawn_fp += speed_fp;
            if (FP_TO_INT(dist_since_last_spawn_fp) >= next_spawn_gap) {
                for (int i = 0; i < MAX_OBS; i++) {
                    if (!obstacles[i].active) {
                        obstacles[i].active = 1;
                        obstacles[i].x = INT_TO_FP(240);
                        obstacles[i].frame = 0;
                        int r = rand_next() % 100;
                        if (score > 350 && r < 35) {
                            obstacles[i].type = OBS_PTERO; obstacles[i].w = 24; obstacles[i].h = 10;
                            int alt = rand_next() % 3;
                            obstacles[i].y = alt == 0 ? GROUND_Y - 12 : (alt == 1 ? GROUND_Y - 26 : GROUND_Y - 45);
                        } else if (r < 75) {
                            obstacles[i].type = OBS_CACTUS_SM; obstacles[i].w = 12; obstacles[i].h = 12;
                            obstacles[i].y = GROUND_Y - 12;
                        } else {
                            obstacles[i].type = OBS_CACTUS_LG; obstacles[i].w = 20; obstacles[i].h = 13;
                            obstacles[i].y = GROUND_Y - 13;
                        }
                        dist_since_last_spawn_fp = 0;
                        next_spawn_gap = 200 + (rand_next() % 120);
                        break;
                    }
                }
            }

            if (speed_fp < SPEED_MAX) speed_fp += SPEED_ACCEL;
            
            distance_ran_fp += speed_fp;
            int prev_score = score;
            score = FP_TO_INT(distance_ran_fp) / 16;
            if (score / 100 > prev_score / 100) snd_point();

            dino.tick++;
        } else {
            if ((keys & KEY_A) || (keys & KEY_START)) reset_game();
        }

        // --- HARDWARE GRAPHICS UPDATE ---
        for (int i = 0; i < 128; i++) hide_obj(i);

        // 1. Dino Anim (4-Frame Run Cycle)
        int dtile = 0, dshape = ATTR0_SHAPE_SQUARE, dsize = ATTR1_SIZE_32;
        if (dino.dead) dtile = 112;
        else if (dino.is_ducking) { dshape = ATTR0_SHAPE_WIDE; dtile = 80 + ((dino.tick / 6) % 2) * 16; }
        else if (dino.is_jumping) dtile = 0;
        else dtile = 16 + ((dino.tick / 5) % 4) * 16;
        set_obj(0, dino.x, FP_TO_INT(dino.y), dshape, dsize, dtile);

        // 2. Obstacles
        for(int i = 0; i < MAX_OBS; i++) {
            if (obstacles[i].active) {
                int ox = FP_TO_INT(obstacles[i].x);
                if (ox > -32 && ox < 240) {
                    if (obstacles[i].type == OBS_CACTUS_SM)
                        set_obj(1+i, ox, obstacles[i].y, ATTR0_SHAPE_TALL, ATTR1_SIZE_16, 144);
                    else if (obstacles[i].type == OBS_CACTUS_LG)
                        set_obj(1+i, ox, obstacles[i].y, ATTR0_SHAPE_SQUARE, ATTR1_SIZE_32, 152);
                    else 
                        set_obj(1+i, ox, obstacles[i].y, ATTR0_SHAPE_WIDE, ATTR1_SIZE_32, 128 + obstacles[i].frame * 8);
                }
            }
        }

        // 3. Clouds
        for(int i = 0; i < MAX_CLOUDS; i++)
            set_obj(10+i, cloud_x[i], cloud_y[i], ATTR0_SHAPE_WIDE, ATTR1_SIZE_32, 168);

        // 4. Score
        draw_score_objs(20, 196, 6, score);
        draw_score_objs(26, 140, 6, high_score);

        // VSYNC & DMA Transfers (Zero screen tearing!)
        while (REG_VCOUNT >= 160);
        while (REG_VCOUNT < 160);

        REG_BG1HOFS = ground_scroll; 
        
        REG_DMA3SAD = (unsigned int)obj_buffer;
        REG_DMA3DAD = 0x07000000;
        REG_DMA3CNT = 0x85000000 | 256; // 128 objs * 2 words
    }
    return 0;
}

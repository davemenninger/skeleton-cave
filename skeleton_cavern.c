#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define INIT_MODE 0
#define CHARACTER_MODE 2
#define WEAPONS_MODE 3
#define ITEMS_MODE 4
#define ROOM_DRAW_MODE 5
#define ROOM_DOOR_MODE 6
#define DOOR_MODE 7
#define MONSTERS_MODE 8
#define FIGHTING_MODE 9
#define TREASURE_MODE 10

#define GRAPH_PAPER_WIDTH 32
#define GRAPH_PAPER_HEIGHT 24
#define CELL_SIZE 16
#define GRAPH_PAPER_X 72
#define GRAPH_PAPER_Y 0

/* Screen dimension constants */
const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;
const int PAD = 2;

typedef struct {
  int strength, dexterity, charisma, wits;
  int gold;
  char class[7];
  char race[8];
  int health, will;
  int room_id;
} Character;

typedef struct {
  int size;
  int type;
  int door_count;
  int monster_type;
  int monster_count;
} Room;

typedef struct {
  int from_i;
  int from_j;
  int to_i;
  int to_j;
  int from_room_id;
  int to_room_id;
} Door;

typedef struct {
  char name[17];
  int max_count;
  int h_dmg;
  int w_dmg;
  int lf;
} MonsterType;

typedef struct {
  Room room_list[GRAPH_PAPER_WIDTH * GRAPH_PAPER_HEIGHT];
  Door door_list[(GRAPH_PAPER_WIDTH + 1 * GRAPH_PAPER_HEIGHT + 1) * 2];
} Dungeon;

typedef struct {
  int room_id;
} Cell;

typedef struct {
  Cell cells[GRAPH_PAPER_WIDTH][GRAPH_PAPER_HEIGHT];
} GraphPaper;

SDL_Event event;
SDL_Rect rect;
SDL_Renderer *rend;
SDL_Surface *surface = NULL;
SDL_Window *window = NULL;
SDL_bool quit = SDL_FALSE;
SDL_Texture *gTexture = NULL;
Uint32 render_flags =
    0; // SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC;
Uint32 *pixels;

int mouse_x = 0, mouse_y = 0;
int selected_cell_i = -1, selected_cell_j = -1;
int door_pick_from_i = -1, door_pick_from_j = -1, door_pick_to_i = -1,
    door_pick_to_j = -1;

Character character;
Dungeon dungeon;
GraphPaper graphpaper;

MonsterType monster_types[] = {(MonsterType){"", 0, 0, 0, 0},
                               (MonsterType){"Putrid Rat", 6, 1, 1, 1},
                               (MonsterType){"Winged Rat", 6, 1, 1, 2},
                               (MonsterType){"Floating Skull", 5, 1, 1, 5},
                               (MonsterType){"Skeleton Archer", 4, 1, 1, 7},
                               (MonsterType){"Skeleton Warrior", 3, 1, 1, 10},
                               (MonsterType){"Necromancer", 1, 1, 1, 20}};

int game_mode = INIT_MODE;

// room generating
Room current_gen_room;
int gen_room_size = 0;
int room_count = 0;
int door_count = 0;

Uint32 theme[] = {0x000000, 0x4D2600,                               /* dirt */
                  0x996633,                                         /* muddy */
                  0x33677,                                          /* watery */
                  0x666666,                                         /* stone */
                  0x669999,                                         /* crypt */
                  0x990000,                                         /* altar */
                  0xFFFFFF, 0x72DEC2, 0x666666, 0x222222, 0xBBBB11, /* gold */
                  0xAA1111, 0x11AA11};

Uint8 icons[][8] = {
    {0x10, 0x00, 0x10, 0x00, 0x10, 0x00, 0x10, 0x00}, /* ruler */
    {0x55, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0xaa}, /* unknown */
    {0x00, 0x00, 0x00, 0x10, 0x08, 0x10, 0x00, 0x00}, /* tab */
    {0x04, 0x08, 0x50, 0xa4, 0x50, 0x08, 0x04, 0x00},
    {0x28, 0x14, 0x28, 0x14, 0x28, 0x14, 0x28, 0x14}, /* scroll:bg */
    {0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c}, /* scroll:fg */
    {0x00, 0x00, 0x00, 0x82, 0x44, 0x38, 0x00, 0x00}, /* eye open */
    {0x00, 0x38, 0x44, 0x92, 0x28, 0x10, 0x00, 0x00}, /* eye closed */
    {0x10, 0x54, 0x28, 0xc6, 0x28, 0x54, 0x10, 0x00}, /* unsaved */
    {0x18, 0x24, 0x42, 0x42, 0x81, 0x81, 0x81, 0xFF}, /* door */
    {0x10, 0x0C, 0x02, 0x21, 0x7D, 0xFE, 0x24, 0x48},  /* putrid rat */
    {0xEF, 0x01, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE},  /* monster 2 */
    {0x6C, 0x92, 0x92, 0x6C, 0x7C, 0x54, 0x00, 0x00},  /* floating skull */
    {0x39, 0x0C, 0x06, 0x73, 0xA9, 0xF9, 0x70, 0x70},  /* skeleton archer */
    {0x02, 0x02, 0x02, 0x77, 0xAA, 0xF8, 0x70, 0x70},  /* skeleton warrior */
    {0x23, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0, 0x12}   /* monster 6 */
};

Uint8 font[][8] = {{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* space */
                   {0x00, 0x08, 0x08, 0x08, 0x08, 0x08, 0x00, 0x08},
                   {0x00, 0x14, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00},
                   {0x00, 0x22, 0x7f, 0x22, 0x22, 0x22, 0x7f, 0x22},
                   {0x00, 0x08, 0x7f, 0x40, 0x3e, 0x01, 0x7f, 0x08},
                   {0x00, 0x21, 0x52, 0x24, 0x08, 0x12, 0x25, 0x42},
                   {0x00, 0x3e, 0x41, 0x42, 0x38, 0x05, 0x42, 0x3d},
                   {0x00, 0x08, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00}, /* quote */
                   {0x00, 0x08, 0x10, 0x10, 0x10, 0x10, 0x10, 0x08},
                   {0x00, 0x08, 0x04, 0x04, 0x04, 0x04, 0x04, 0x08},
                   {0x00, 0x00, 0x2a, 0x1c, 0x3e, 0x1c, 0x2a, 0x00},
                   {0x00, 0x00, 0x08, 0x08, 0x3e, 0x08, 0x08, 0x00},
                   {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x10},
                   {0x00, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x00, 0x00},
                   {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08},
                   {0x00, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40},
                   {0x00, 0x3e, 0x41, 0x41, 0x41, 0x41, 0x41, 0x3e},
                   {0x00, 0x08, 0x18, 0x08, 0x08, 0x08, 0x08, 0x1c},
                   {0x00, 0x7e, 0x01, 0x01, 0x3e, 0x40, 0x40, 0x7f},
                   {0x00, 0x7e, 0x01, 0x01, 0x3e, 0x01, 0x01, 0x7e},
                   {0x00, 0x11, 0x21, 0x41, 0x7f, 0x01, 0x01, 0x01},
                   {0x00, 0x7f, 0x40, 0x40, 0x3e, 0x01, 0x01, 0x7e},
                   {0x00, 0x3e, 0x41, 0x40, 0x7e, 0x41, 0x41, 0x3e},
                   {0x00, 0x7f, 0x01, 0x01, 0x02, 0x04, 0x08, 0x08},
                   {0x00, 0x3e, 0x41, 0x41, 0x3e, 0x41, 0x41, 0x3e}, /* 8 */
                   {0x00, 0x3e, 0x41, 0x41, 0x3f, 0x01, 0x02, 0x04},
                   {0x00, 0x00, 0x00, 0x08, 0x00, 0x08, 0x00, 0x00},
                   {0x00, 0x00, 0x00, 0x08, 0x00, 0x08, 0x08, 0x10},
                   {0x00, 0x00, 0x08, 0x10, 0x20, 0x10, 0x08, 0x00},
                   {0x00, 0x00, 0x00, 0x1c, 0x00, 0x1c, 0x00, 0x00},
                   {0x00, 0x00, 0x10, 0x08, 0x04, 0x08, 0x10, 0x00},
                   {0x00, 0x3e, 0x41, 0x01, 0x06, 0x08, 0x00, 0x08},
                   {0x00, 0x3e, 0x41, 0x5d, 0x55, 0x45, 0x59, 0x26},
                   {0x00, 0x3e, 0x41, 0x41, 0x7f, 0x41, 0x41, 0x41},
                   {0x00, 0x7e, 0x41, 0x41, 0x7e, 0x41, 0x41, 0x7e},
                   {0x00, 0x3e, 0x41, 0x40, 0x40, 0x40, 0x41, 0x3e},
                   {0x00, 0x7c, 0x42, 0x41, 0x41, 0x41, 0x42, 0x7c},
                   {0x00, 0x3f, 0x40, 0x40, 0x7f, 0x40, 0x40, 0x3f},
                   {0x00, 0x7f, 0x40, 0x40, 0x7e, 0x40, 0x40, 0x40},
                   {0x00, 0x3e, 0x41, 0x50, 0x4e, 0x41, 0x41, 0x3e},
                   {0x00, 0x41, 0x41, 0x41, 0x7f, 0x41, 0x41, 0x41},
                   {0x00, 0x1c, 0x08, 0x08, 0x08, 0x08, 0x08, 0x1c},
                   {0x00, 0x7f, 0x01, 0x01, 0x01, 0x01, 0x01, 0x7e},
                   {0x00, 0x41, 0x42, 0x44, 0x78, 0x44, 0x42, 0x41},
                   {0x00, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x7f},
                   {0x00, 0x63, 0x55, 0x49, 0x41, 0x41, 0x41, 0x41},
                   {0x00, 0x61, 0x51, 0x51, 0x49, 0x49, 0x45, 0x43},
                   {0x00, 0x1c, 0x22, 0x41, 0x41, 0x41, 0x22, 0x1c},
                   {0x00, 0x7e, 0x41, 0x41, 0x7e, 0x40, 0x40, 0x40},
                   {0x00, 0x3e, 0x41, 0x41, 0x41, 0x45, 0x42, 0x3d},
                   {0x00, 0x7e, 0x41, 0x41, 0x7e, 0x44, 0x42, 0x41},
                   {0x00, 0x3f, 0x40, 0x40, 0x3e, 0x01, 0x01, 0x7e},
                   {0x00, 0x7f, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08},
                   {0x00, 0x41, 0x41, 0x41, 0x41, 0x41, 0x42, 0x3d},
                   {0x00, 0x41, 0x41, 0x41, 0x41, 0x22, 0x14, 0x08},
                   {0x00, 0x41, 0x41, 0x41, 0x41, 0x49, 0x55, 0x63},
                   {0x00, 0x41, 0x22, 0x14, 0x08, 0x14, 0x22, 0x41},
                   {0x00, 0x41, 0x22, 0x14, 0x08, 0x08, 0x08, 0x08},
                   {0x00, 0x7f, 0x01, 0x02, 0x1c, 0x20, 0x40, 0x7f},
                   {0x00, 0x18, 0x10, 0x10, 0x10, 0x10, 0x10, 0x18},
                   {0x00, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01},
                   {0x00, 0x18, 0x08, 0x08, 0x08, 0x08, 0x08, 0x18},
                   {0x00, 0x08, 0x14, 0x22, 0x00, 0x00, 0x00, 0x00},
                   {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7f},
                   {0x00, 0x08, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00},
                   {0x00, 0x00, 0x00, 0x7e, 0x01, 0x3e, 0x41, 0x7d},
                   {0x00, 0x00, 0x00, 0x40, 0x7e, 0x41, 0x41, 0x7e},
                   {0x00, 0x00, 0x00, 0x3e, 0x41, 0x40, 0x41, 0x3e},
                   {0x00, 0x00, 0x00, 0x01, 0x3f, 0x41, 0x41, 0x3f},
                   {0x00, 0x00, 0x00, 0x3e, 0x41, 0x7e, 0x40, 0x3f},
                   {0x00, 0x00, 0x00, 0x3f, 0x40, 0x7e, 0x40, 0x40},
                   {0x00, 0x00, 0x00, 0x3f, 0x41, 0x3f, 0x01, 0x7e},
                   {0x00, 0x00, 0x00, 0x40, 0x7e, 0x41, 0x41, 0x41},
                   {0x00, 0x00, 0x00, 0x08, 0x00, 0x08, 0x08, 0x08},
                   {0x00, 0x00, 0x00, 0x7f, 0x01, 0x01, 0x02, 0x7c},
                   {0x00, 0x00, 0x00, 0x41, 0x46, 0x78, 0x46, 0x41},
                   {0x00, 0x00, 0x00, 0x40, 0x40, 0x40, 0x40, 0x3f},
                   {0x00, 0x00, 0x00, 0x76, 0x49, 0x41, 0x41, 0x41},
                   {0x00, 0x00, 0x00, 0x61, 0x51, 0x49, 0x45, 0x43},
                   {0x00, 0x00, 0x00, 0x3e, 0x41, 0x41, 0x41, 0x3e},
                   {0x00, 0x00, 0x00, 0x7e, 0x41, 0x7e, 0x40, 0x40},
                   {0x00, 0x00, 0x00, 0x3f, 0x41, 0x3f, 0x01, 0x01},
                   {0x00, 0x00, 0x00, 0x5e, 0x61, 0x40, 0x40, 0x40},
                   {0x00, 0x00, 0x00, 0x3f, 0x40, 0x3e, 0x01, 0x7e},
                   {0x00, 0x00, 0x00, 0x7f, 0x08, 0x08, 0x08, 0x08},
                   {0x00, 0x00, 0x00, 0x41, 0x41, 0x41, 0x42, 0x3d},
                   {0x00, 0x00, 0x00, 0x41, 0x41, 0x22, 0x14, 0x08},
                   {0x00, 0x00, 0x00, 0x41, 0x41, 0x41, 0x49, 0x37},
                   {0x00, 0x00, 0x00, 0x41, 0x22, 0x1c, 0x22, 0x41},
                   {0x00, 0x00, 0x00, 0x41, 0x22, 0x1c, 0x08, 0x08},
                   {0x00, 0x00, 0x00, 0x7f, 0x01, 0x3e, 0x40, 0x7f},
                   {0x00, 0x08, 0x10, 0x10, 0x20, 0x10, 0x10, 0x08},
                   {0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08},
                   {0x00, 0x08, 0x04, 0x04, 0x02, 0x04, 0x04, 0x08},
                   {0x00, 0x00, 0x00, 0x30, 0x49, 0x06, 0x00, 0x00}};

int setup();
void update_stuff();
int screenshot();

int error(char *msg, const char *err) {
  printf("Error %s: %s\n", msg, err);
  return 1;
}

void putpixel(Uint32 *dst, int x, int y, int color) {
  if (x >= 0 && x < SCREEN_WIDTH - 8 && y >= 0 && y < SCREEN_HEIGHT - 8)
    dst[(y + PAD * 8) * SCREEN_WIDTH + (x + PAD * 8)] = theme[color];
}

Uint8 *geticn(char c) {
  if (c < 0) /* unknown */
    return icons[1];
  if (c == '\t') /* tab */
    return icons[2];
  if (c < 32) /* special */
    return font[0];
  return font[(int)c - 32];
}

void drawicn(Uint32 *dst, int x, int y, Uint8 *sprite, int fg, int bg) {
  int v, h;
  for (v = 0; v < 8; v++)
    for (h = 0; h < 8; h++) {
      int ch1 = (sprite[v] >> (7 - h)) & 0x1;
      putpixel(dst, x + h, y + v, ch1 ? fg : bg);
    }
}

void drawmouse(Uint32 *dst) { putpixel(dst, mouse_x, mouse_y, 1); }

void drawstr(Uint32 *dst, int x, int y) {
  drawicn(dst, x * 8, y, geticn('S'), 10, 0);
  drawicn(dst, (x + 1) * 8, y, geticn('T'), 10, 0);
  drawicn(dst, (x + 2) * 8, y, geticn(character.strength + 48), 7, 0);
  if (strncmp(character.race, "human", 5) == 0) {
    drawicn(dst, (x + 3) * 8, y, geticn('+'), 8, 0);
    drawicn(dst, (x + 4) * 8, y, geticn('1'), 8, 0);
  }
}

void drawdex(Uint32 *dst, int x, int y) {
  drawicn(dst, x * 8, y, geticn('D'), 10, 0);
  drawicn(dst, (x + 1) * 8, y, geticn('E'), 10, 0);
  drawicn(dst, (x + 2) * 8, y, geticn(character.dexterity + 48), 7, 0);
  if (strncmp(character.race, "halfling", 8) == 0) {
    drawicn(dst, (x + 3) * 8, y, geticn('+'), 8, 0);
    drawicn(dst, (x + 4) * 8, y, geticn('1'), 8, 0);
  }
}

void drawwits(Uint32 *dst, int x, int y) {
  drawicn(dst, x * 8, y, geticn('W'), 10, 0);
  drawicn(dst, (x + 1) * 8, y, geticn('I'), 10, 0);
  drawicn(dst, (x + 2) * 8, y, geticn(character.wits + 48), 7, 0);
  if (strncmp(character.race, "dwarf", 5) == 0) {
    drawicn(dst, (x + 3) * 8, y, geticn('+'), 8, 0);
    drawicn(dst, (x + 4) * 8, y, geticn('1'), 8, 0);
  }
}

void drawcha(Uint32 *dst, int x, int y) {
  drawicn(dst, x * 8, y, geticn('C'), 10, 0);
  drawicn(dst, (x + 1) * 8, y, geticn('H'), 10, 0);
  drawicn(dst, (x + 2) * 8, y, geticn(character.charisma + 48), 7, 0);
  if (strncmp(character.race, "elf", 3) == 0) {
    drawicn(dst, (x + 3) * 8, y, geticn('+'), 8, 0);
    drawicn(dst, (x + 4) * 8, y, geticn('1'), 8, 0);
  }
}

void drawgold(Uint32 *dst, int x, int y) {
  char g[4] = "";
  sprintf(g, "%d", character.gold);
  drawicn(dst, x * 8, y, geticn('G'), 10, 0);
  drawicn(dst, (x + 1) * 8, y, geticn('P'), 10, 0);
  for (int i = 0; i < 4; i++) {
    drawicn(dst, (x + 2 + i) * 8, y, geticn(g[i]), 11, 0);
  }
}

void drawrace(Uint32 *dst, int x, int y) {
  for (int i = 0; i < 8; i++) {
    char c = character.race[i];
    drawicn(dst, (x + i) * 8, y, geticn(c), 9, 0);
  }
}

void drawclass(Uint32 *dst, int x, int y) {
  for (int i = 0; i < 7; i++) {
    char c = character.class[i];
    drawicn(dst, (x + i) * 8, y, geticn(c), 9, 0);
  }
}

void drawhealth(Uint32 *dst, int x, int y) {
  char h[3] = "";
  sprintf(h, "%d", character.health);
  drawicn(dst, x * 8, y, geticn('H'), 10, 0);
  drawicn(dst, (x + 1) * 8, y, geticn('P'), 10, 0);
  for (int i = 0; i < 3; i++) {
    drawicn(dst, (x + 2 + i) * 8, y, geticn(h[i]), 6, 0);
  }
}

void drawwill(Uint32 *dst, int x, int y) {
  char w[3] = "";
  sprintf(w, "%d", character.will);
  drawicn(dst, x * 8, y, geticn('W'), 10, 0);
  drawicn(dst, (x + 1) * 8, y, geticn('P'), 10, 0);
  for (int i = 0; i < 3; i++) {
    drawicn(dst, (x + 2 + i) * 8, y, geticn(w[i]), 8, 0);
  }
}

void drawstats(Uint32 *dst, int x, int y) {
  drawrace(dst, x, y);
  drawclass(dst, x, y + 8);
  drawstr(dst, x, y + 16);
  drawdex(dst, x, y + 24);
  drawwits(dst, x, y + 32);
  drawcha(dst, x, y + 40);
  drawhealth(dst, x, y + 56);
  drawwill(dst, x, y + 64);
  drawgold(dst, x, y + 72);
}

void drawmode(Uint32 *dst, int x, int y) {
  drawicn(dst, x * 8, y, geticn('M'), 3, 0);
  drawicn(dst, (x + 1) * 8, y, geticn('O'), 3, 0);
  drawicn(dst, (x + 2) * 8, y, geticn('D'), 3, 0);
  drawicn(dst, (x + 3) * 8, y, geticn('E'), 3, 0);
  drawicn(dst, (x + 4) * 8, y, geticn(game_mode + 48), 3, 0);
}

void drawroom(Uint32 *dst, int x, int y) {
  int room_id = character.room_id;
  if (room_id != 0) {
    drawicn(dst, x * 8, y, geticn('R'), 10, 0);
    drawicn(dst, (x + 1) * 8, y, geticn('O'), 10, 0);
    drawicn(dst, (x + 2) * 8, y, geticn('O'), 10, 0);
    drawicn(dst, (x + 3) * 8, y, geticn('M'), 10, 0);
    drawicn(dst, (x + 4) * 8, y, geticn(room_id + 48), 10, 0);
    Room room = dungeon.room_list[room_id];
    int box_size = 56;
    for (int i = 0; i < box_size; i++) {
      for (int j = 0; j < box_size; j++) {
        if (i == 0 || j == 0 || i == box_size - 1 || j == box_size - 1) {
          putpixel(dst, x + i, y + j + 8, 10);
        } else {
          putpixel(dst, x + i, y + j + 8, dungeon.room_list[room_id].type);
        }
      }
    }
    for (int m = 0; m < room.monster_count; m++) {
      drawicn(dst, x + ((((m % 3) * 2) + 1) * 8), y + (16 * ((m / 3) + 1)),
              icons[room.monster_type + 9], 7, dungeon.room_list[room_id].type);
    }
  }
}

void drawcell(Uint32 *dst, int x, int y, Cell cell) {
  for (int i = 0; i < CELL_SIZE; i++) {
    for (int j = 0; j < CELL_SIZE; j++) {
      if (i != 0 && j != 0 && i != CELL_SIZE - 1 && j != CELL_SIZE - 1) {
        putpixel(dst, x + i, y + j, dungeon.room_list[cell.room_id].type);
      }
    }
  }
}

void drawdoor(Uint32 *dst, Door door) {
  int x = GRAPH_PAPER_X + (CELL_SIZE * door.from_i);
  int y = GRAPH_PAPER_Y + (CELL_SIZE * door.from_j);

  if (door.from_i > door.to_i) {
    // west door
    x = x - 4;
    y = y + 4;
  } else if (door.from_i < door.to_i) {
    // east door
    x = x + CELL_SIZE - 4;
    y = y + 4;
  } else if (door.from_j > door.to_j) {
    // north door
    x = x + 4;
    y = y - 4;
  } else if (door.from_j < door.to_j) {
    // south door
    x = x + 4;
    y = y + CELL_SIZE - 4;
  }
  drawicn(dst, x, y, icons[9], 2, 0);
}

void drawgraphpaper(Uint32 *dst, int x, int y) {
  for (int i = 0; i < GRAPH_PAPER_WIDTH; i++) {
    for (int j = 0; j < GRAPH_PAPER_HEIGHT; j++) {
      if (i == 0 || j == 0 || i == GRAPH_PAPER_WIDTH - 1 ||
          j == GRAPH_PAPER_HEIGHT - 1) {
        putpixel(dst, (i * CELL_SIZE) + x, (j * CELL_SIZE) + y, 10);
      }

      drawcell(dst, (i * CELL_SIZE) + x, (j * CELL_SIZE) + y,
               graphpaper.cells[i][j]);

      if (i == selected_cell_i && j == selected_cell_j) {
        putpixel(dst, (i * CELL_SIZE) + x + 5, (j * CELL_SIZE) + y + 5, 1);
      }

      if (i == door_pick_from_i && j == door_pick_from_j) {
        drawicn(dst, (i * CELL_SIZE) + x + (CELL_SIZE / 4),
                (j * CELL_SIZE) + y + (CELL_SIZE / 4), icons[9], 1, 0);
      }
    }
  }
  for (int i = 0; i <= door_count; i++) {
    if (dungeon.door_list[i].from_room_id > 0) {
      drawdoor(dst, dungeon.door_list[i]);
    }
  }
}

void clear(Uint32 *dst) {
  int i, j;
  for (i = 0; i < SCREEN_HEIGHT; i++)
    for (j = 0; j < SCREEN_WIDTH; j++)
      dst[i * SCREEN_WIDTH + j] = theme[0];
}

void redraw(Uint32 *dst) {

  clear(pixels);
  drawmouse(dst);
  drawstats(dst, 0, 0);
  drawmode(dst, 0, 400);
  drawroom(dst, 0, 240);
  drawgraphpaper(dst, GRAPH_PAPER_X, GRAPH_PAPER_Y);

  SDL_UpdateTexture(gTexture, NULL, dst, SCREEN_WIDTH * sizeof(Uint32));
  SDL_RenderClear(rend);
  SDL_RenderCopy(rend, gTexture, NULL, NULL);
  SDL_RenderPresent(rend);
}

int count_cells_for_room_id(int room_id) {
  int num = 0;
  for (int i = 0; i < GRAPH_PAPER_WIDTH; i++) {
    for (int j = 0; j < GRAPH_PAPER_HEIGHT; j++) {
      if (graphpaper.cells[i][j].room_id == room_id) {
        num++;
      }
    }
  }
  return num;
}

int is_selected_cell_adjacent_room_id(int room_id) {
  int num = 0;
  for (int i = 0; i < GRAPH_PAPER_WIDTH; i++) {
    for (int j = 0; j < GRAPH_PAPER_HEIGHT; j++) {
      // if this cell is same as room_id, and:
      // either same row, plus or minus column,
      // or same column, plus or minus row
      if (graphpaper.cells[i][j].room_id == room_id &&
          (((i == selected_cell_i + 1 || i == selected_cell_i - 1) &&
            j == selected_cell_j) ||
           ((j == selected_cell_j + 1 || j == selected_cell_j - 1) &&
            i == selected_cell_i))) {
        num++;
      }
    }
  }
  return num;
}

int validate_cell_add(int room_id) {
  int existing_cells = 0;
  existing_cells = count_cells_for_room_id(room_id);
  if (existing_cells >= dungeon.room_list[room_id].size) {
    return 0;
  }

  if (existing_cells == 0) {
    return 1;
  }

  // selected_cell_i and selected_cell_j must be adjacent an existing cell for
  // this room_id
  if (is_selected_cell_adjacent_room_id(room_id) == 0) {
    return 0;
  }

  return 1;
}

int count_doors_for_room_id(int room_id) {
  int num = 0;
  for (int i = 0; i < GRAPH_PAPER_WIDTH; i++) {
    if (dungeon.door_list[i].from_room_id == room_id) {
      num++;
    }
  }
  return num;
}

int validate_door_add(int room_id) {
  int existing_doors = 0;
  existing_doors = count_doors_for_room_id(room_id);
  if (existing_doors >= dungeon.room_list[room_id].door_count) {
    game_mode = MONSTERS_MODE;
    return 0;
  }

  // is this door already exists
  for (int i = 0; i < door_count; i++) {
    Door d = dungeon.door_list[i];
    if (d.from_i == door_pick_from_i && d.from_j == door_pick_from_j &&
        d.to_i == selected_cell_i && d.to_j == selected_cell_j) {
      return 0;
    }
  }

  // is selected_cell adjacent to door_pick_from and a different room
  if (graphpaper.cells[door_pick_from_i][door_pick_from_j].room_id !=
          graphpaper.cells[selected_cell_i][selected_cell_j].room_id &&
      (((door_pick_from_i == selected_cell_i + 1 ||
         door_pick_from_i == selected_cell_i - 1) &&
        door_pick_from_j == selected_cell_j) ||
       ((door_pick_from_j == selected_cell_j + 1 ||
         door_pick_from_j == selected_cell_j - 1) &&
        door_pick_from_i == selected_cell_i))) {
    return 1;
  }

  return 0;
}

void add_door() {
  Door d;
  d.from_i = door_pick_from_i;
  d.from_j = door_pick_from_j;
  d.to_i = selected_cell_i;
  d.to_j = selected_cell_j;
  d.from_room_id = room_count;
  dungeon.door_list[door_count] = d;
  door_count++;
  door_pick_from_i = -1;
  door_pick_from_j = -1;
  int existing_doors = 0;
  existing_doors = count_doors_for_room_id(room_count);
  if (existing_doors >= dungeon.room_list[room_count].door_count) {
    game_mode = MONSTERS_MODE;
  }
}

void do_mouse(SDL_Event *event) {
  mouse_x = event->motion.x - 20;
  mouse_y = event->motion.y - 20;
}

void do_click() {
  if (game_mode == ROOM_DRAW_MODE) {
    int r_id = room_count;
    if (validate_cell_add(r_id) == 1) {
      graphpaper.cells[selected_cell_i][selected_cell_j].room_id = r_id;
    }
  } else if (game_mode == ROOM_DOOR_MODE) {
    if (door_pick_from_i == -1 && door_pick_from_j == -1) {
      // door start hasn't been picked yet
      if (graphpaper.cells[selected_cell_i][selected_cell_j].room_id ==
          room_count) {
        // door must start inside the current room
        door_pick_from_i = selected_cell_i;
        door_pick_from_j = selected_cell_j;
      }
    } else {
      // door start has been picked
      if (validate_door_add(room_count) == 1) {
        // add these two neighboring cells to adjcency/door list
        add_door();
      } else {
        door_pick_from_i = -1;
        door_pick_from_j = -1;
      }
    }
  }
}

int main(int argc, char *args[]) {
  (void)argc;
  (void)args;

  printf("dpfi: %d\n", door_pick_from_i);

  if (setup() != 0)
    return error("Setup", "Error");

  game_mode = ROOM_DRAW_MODE;

  printf("dpfi: %d\n", door_pick_from_i);

  while (!quit) {
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
      case SDL_QUIT:
        quit = SDL_TRUE;
        break;
      case SDL_KEYUP:
        switch (event.key.keysym.scancode) {
        case SDL_SCANCODE_D:
        case SDL_SCANCODE_RIGHT:
          break;
        case SDL_SCANCODE_A:
        case SDL_SCANCODE_LEFT:
          break;
        case SDL_SCANCODE_W:
        case SDL_SCANCODE_UP:
          break;
        case SDL_SCANCODE_S:
        case SDL_SCANCODE_DOWN:
          break;
        default:
          (void)0;
        }
        break;
      case SDL_KEYDOWN:
        switch (event.key.keysym.scancode) {
        case SDL_SCANCODE_D:
        case SDL_SCANCODE_RIGHT:
          break;
        case SDL_SCANCODE_A:
        case SDL_SCANCODE_LEFT:
          break;
        case SDL_SCANCODE_W:
        case SDL_SCANCODE_UP:
          break;
        case SDL_SCANCODE_S:
        case SDL_SCANCODE_DOWN:
          break;
        default:
          (void)0;
        }
        break;
      case SDL_MOUSEMOTION:
        do_mouse(&event);
        break;
      case SDL_MOUSEBUTTONDOWN:
        do_click();
        break;
      default:
        (void)0;
      }
    }

    update_stuff();
    redraw(pixels);

    SDL_Delay(1000 / 60);
  }

  screenshot();

  /* close */
  SDL_FreeSurface(surface);
  surface = NULL;
  SDL_DestroyWindow(window);
  window = NULL;
  SDL_Quit();

  return 0;
}

int validate_character(Character *character) {
  if (character->strength > 4)
    return error("Character", "strength > 4");
  if (character->dexterity > 4)
    return error("Character", "dexterity > 4");
  if (character->charisma > 4)
    return error("Character", "charisma > 4");
  if (character->wits > 4)
    return error("Character", "wits > 4");

  if (character->strength < 1)
    return error("Character", "strength < 1");
  if (character->dexterity < 1)
    return error("Character", "dexterity < 1");
  if (character->charisma < 1)
    return error("Character", "charisma < 1");
  if (character->wits < 1)
    return error("Character", "wits < 1");

  if (character->strength + character->dexterity + character->charisma +
          character->wits >
      7)
    return error("Character", "sum of stats > 7");

  if (strncmp(character->class, "fighter", 7) != 0 &&
      strncmp(character->class, "ranger", 6) != 0 &&
      strncmp(character->class, "wizard", 6) != 0 &&
      strncmp(character->class, "bard", 4) != 0)
    return error("Character", "unknown class");

  if (strncmp(character->race, "human", 5) != 0 &&
      strncmp(character->race, "halfling", 8) != 0 &&
      strncmp(character->race, "dwarf", 5) != 0 &&
      strncmp(character->race, "elf", 3) != 0)
    return error("Character", "unknown race");

  return 0;
}

int computestr(Character *character) {
  int str = character->strength;
  if (strncmp(character->race, "human", 5) == 0)
    str += 1;
  return str;
}

int computedex(Character *character) {
  int dex = character->dexterity;
  if (strncmp(character->race, "halfling", 8) == 0)
    dex += 1;
  return dex;
}

int computewit(Character *character) {
  int wit = character->wits;
  if (strncmp(character->race, "dwarf", 5) == 0)
    wit += 1;
  return wit;
}

int computecha(Character *character) {
  int cha = character->charisma;
  if (strncmp(character->race, "elf", 3) == 0)
    cha += 1;
  return cha;
}

void compute_health_and_will(Character *character) {
  character->health = 20 + computestr(character) + computedex(character);
  character->will = 20 + computewit(character) + computecha(character);
}

int open_character(Character *character, char *name) {
  char line[256];
  printf("Loading: %s...\n", name);
  FILE *f = fopen(name, "r");
  if (!f)
    return error("Load", "Invalid character file");
  while (fgets(line, 256, f)) {
    sscanf(line, "strength=%d\n", &character->strength);
    sscanf(line, "dexterity=%d\n", &character->dexterity);
    sscanf(line, "wits=%d\n", &character->wits);
    sscanf(line, "charisma=%d\n", &character->charisma);
    sscanf(line, "class=%s\n", character->class);
    sscanf(line, "race=%s\n", character->race);
  }
  if (validate_character(character) != 0)
    return error("Validate", "invalid character");

  printf("Loaded: %s\n", name);
  fclose(f);

  return 0;
}

int dice(int n, int d) {
  int sum = 0;
  printf("dice: %dd%d\n", n, d);
  for (int i = 0; i < n; i++) {
    int roll = (rand() % d) + 1;
    printf("roll: %d\n", roll);
    sum += roll;
  }

  return sum;
}

int dice_n(char *notation) {
  int n = 0, d = 1, m = 0;
  char s[10];
  sscanf(notation, "%dd%9s", &n, s);
  sscanf(s, "%d+%d", &d, &m);
  if (m == 0) {
    sscanf(s, "%d-%d", &d, &m);
  }
  return dice(n, d) + m;
}

void roll_for_gold(Character *character) {
  printf("rolling for initial gold...\n");
  character->gold = dice_n("2d6");
}

void erase_graph_paper(GraphPaper *graphpaper) {
  for (int i = 0; i < GRAPH_PAPER_WIDTH; i++) {
    for (int j = 0; j < GRAPH_PAPER_HEIGHT; j++) {
      graphpaper->cells[i][j].room_id = 0;
    }
  }
}

void gen_room() {
  printf("rolling for room size...\n");
  current_gen_room.size = dice(1, 6);
  printf("rolling for room type...\n");
  current_gen_room.type = dice(1, 6);
  printf("rolling for num doors...\n");
  current_gen_room.door_count = dice(1, 3);
  printf("roll for monster type...\n");
  current_gen_room.monster_type = dice(1, 6);
  printf("roll for monster count...\n");
  current_gen_room.monster_count = 99;
  while (current_gen_room.monster_count >
         monster_types[current_gen_room.monster_type].max_count) {
    current_gen_room.monster_count = dice(1, 6);
  }
}

int setup() {
  srand(time(NULL));

  if (SDL_Init(SDL_INIT_VIDEO) < 0)
    return error("init", SDL_GetError());

  window = SDL_CreateWindow("Skeleton Cavern", SDL_WINDOWPOS_UNDEFINED,
                            SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH,
                            SCREEN_HEIGHT, SDL_WINDOW_SHOWN);

  if (window == NULL)
    return error("window", SDL_GetError());

  rend = SDL_CreateRenderer(window, -1, render_flags);
  SDL_RenderClear(rend);

  gTexture =
      SDL_CreateTexture(rend, SDL_PIXELFORMAT_ARGB8888,
                        SDL_TEXTUREACCESS_STATIC, SCREEN_WIDTH, SCREEN_HEIGHT);
  if (gTexture == NULL)
    return error("Texture", SDL_GetError());
  pixels = (Uint32 *)malloc(SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(Uint32));
  if (pixels == NULL)
    return error("Pixels", "Failed to allocate memory");

  clear(pixels);

  if (open_character(&character, "player.character") != 0)
    return error("Character", "problem with character file");

  erase_graph_paper(&graphpaper);

  roll_for_gold(&character);
  compute_health_and_will(&character);
  character.room_id = 0;

  fflush(stdout);

  return 0;
}

void update_stuff() {
  if (mouse_x > GRAPH_PAPER_X && mouse_y > GRAPH_PAPER_Y &&
      mouse_x < GRAPH_PAPER_X + (GRAPH_PAPER_WIDTH * CELL_SIZE) &&
      mouse_y < GRAPH_PAPER_Y + (GRAPH_PAPER_HEIGHT * CELL_SIZE)) {

    selected_cell_i = (mouse_x - GRAPH_PAPER_X) / CELL_SIZE;
    selected_cell_j = (mouse_y - GRAPH_PAPER_Y) / CELL_SIZE;
  }

  if (game_mode == ROOM_DRAW_MODE) {
    if (current_gen_room.size == 0) { // we haven't started this room yet
      gen_room();
      room_count++;
      dungeon.room_list[room_count] = current_gen_room;
      character.room_id = room_count;
    } else if ( // we're done drawing cells
        current_gen_room.size == count_cells_for_room_id(room_count)) {
      game_mode = ROOM_DOOR_MODE;
    }
  } else if (game_mode == ROOM_DOOR_MODE) {
  } else if (game_mode == MONSTERS_MODE) {
  }

  fflush(stdout);
}

int screenshot() {
  surface =
      SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32, 0, 0, 0, 0);

  if (SDL_RenderReadPixels(rend, NULL, surface->format->format, surface->pixels,
                           surface->pitch) != 0) {
    return error("SDL_RenderReadPixels failed: ", SDL_GetError());
  }

  if (SDL_SaveBMP(surface, "screenshot.bmp") != 0) {
    return error("SDL_SaveBMP failed:", SDL_GetError());
  }

  return 0;
}

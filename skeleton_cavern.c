#include <SDL2/SDL.h>
#include <stdio.h>
#include <unistd.h>

#define INIT_MODE 0
#define CHARACTER_MODE 2
#define WEAPONS_MODE 3
#define ITEMS_MODE 4
#define ROOM_SIZE_MODE 5
#define ROOM_TYPE_MODE 6
#define DOOR_MODE 7
#define MONSTERS_MODE 8
#define FIGHTING_MODE 9
#define TREASURE_MODE 10

#define GRAPH_PAPER_WIDTH 16
#define GRAPH_PAPER_HEIGHT 16
#define CELL_SIZE 16
#define GRAPH_PAPER_X 64
#define GRAPH_PAPER_Y 64

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
} Character;

typedef struct {
  int size;
  int type;
} Room;

typedef struct {
  Room rooms[50];
} Dungeon;

typedef struct {
  int color;
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

int mouse_x, mouse_y = 0;
int selected_cell_i, selected_cell_j = 0;

Character character;
Dungeon dungeon;
GraphPaper graphpaper;

int game_mode = INIT_MODE;

// room generating
Room current_gen_room;
int gen_room_size = 0;

Uint32 theme[] = {0x000000, 0xFFFFFF, 0x72DEC2, 0x666666,
                  0x222222, 0xBBBB11, 0xAA1111};

Uint8 icons[][8] = {
    {0x10, 0x00, 0x10, 0x00, 0x10, 0x00, 0x10, 0x00}, /* ruler */
    {0x55, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0xaa}, /* unknown */
    {0x00, 0x00, 0x00, 0x10, 0x08, 0x10, 0x00, 0x00}, /* tab */
    {0x04, 0x08, 0x50, 0xa4, 0x50, 0x08, 0x04, 0x00},
    {0x28, 0x14, 0x28, 0x14, 0x28, 0x14, 0x28, 0x14}, /* scroll:bg */
    {0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c}, /* scroll:fg */
    {0x00, 0x00, 0x00, 0x82, 0x44, 0x38, 0x00, 0x00}, /* eye open */
    {0x00, 0x38, 0x44, 0x92, 0x28, 0x10, 0x00, 0x00}, /* eye closed */
    {0x10, 0x54, 0x28, 0xc6, 0x28, 0x54, 0x10, 0x00}  /* unsaved */
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
  drawicn(dst, x * 8, y, geticn('S'), 3, 0);
  drawicn(dst, (x + 1) * 8, y, geticn('T'), 3, 0);
  drawicn(dst, (x + 2) * 8, y, geticn(character.strength + 48), 1, 0);
  if (strncmp(character.race, "human", 5) == 0) {
    drawicn(dst, (x + 3) * 8, y, geticn('+'), 2, 0);
    drawicn(dst, (x + 4) * 8, y, geticn('1'), 2, 0);
  }
}

void drawdex(Uint32 *dst, int x, int y) {
  drawicn(dst, x * 8, y, geticn('D'), 3, 0);
  drawicn(dst, (x + 1) * 8, y, geticn('E'), 3, 0);
  drawicn(dst, (x + 2) * 8, y, geticn(character.dexterity + 48), 1, 0);
  if (strncmp(character.race, "halfling", 8) == 0) {
    drawicn(dst, (x + 3) * 8, y, geticn('+'), 2, 0);
    drawicn(dst, (x + 4) * 8, y, geticn('1'), 2, 0);
  }
}

void drawwits(Uint32 *dst, int x, int y) {
  drawicn(dst, x * 8, y, geticn('W'), 3, 0);
  drawicn(dst, (x + 1) * 8, y, geticn('I'), 3, 0);
  drawicn(dst, (x + 2) * 8, y, geticn(character.wits + 48), 1, 0);
  if (strncmp(character.race, "dwarf", 5) == 0) {
    drawicn(dst, (x + 3) * 8, y, geticn('+'), 2, 0);
    drawicn(dst, (x + 4) * 8, y, geticn('1'), 2, 0);
  }
}

void drawcha(Uint32 *dst, int x, int y) {
  drawicn(dst, x * 8, y, geticn('C'), 3, 0);
  drawicn(dst, (x + 1) * 8, y, geticn('H'), 3, 0);
  drawicn(dst, (x + 2) * 8, y, geticn(character.charisma + 48), 1, 0);
  if (strncmp(character.race, "elf", 3) == 0) {
    drawicn(dst, (x + 3) * 8, y, geticn('+'), 2, 0);
    drawicn(dst, (x + 4) * 8, y, geticn('1'), 2, 0);
  }
}

void drawgold(Uint32 *dst, int x, int y) {
  char g[4] = "";
  sprintf(g, "%d", character.gold);
  drawicn(dst, x * 8, y, geticn('G'), 3, 0);
  drawicn(dst, (x + 1) * 8, y, geticn('P'), 3, 0);
  for (int i = 0; i < 4; i++) {
    drawicn(dst, (x + 2 + i) * 8, y, geticn(g[i]), 5, 0);
  }
}

void drawrace(Uint32 *dst, int x, int y) {
  for (int i = 0; i < 8; i++) {
    char c = character.race[i];
    drawicn(dst, (x + i) * 8, y, geticn(c), 1, 0);
  }
}

void drawclass(Uint32 *dst, int x, int y) {
  for (int i = 0; i < 7; i++) {
    char c = character.class[i];
    drawicn(dst, (x + i) * 8, y, geticn(c), 1, 0);
  }
}

void drawhealth(Uint32 *dst, int x, int y) {
  char h[3] = "";
  sprintf(h, "%d", character.health);
  drawicn(dst, x * 8, y, geticn('H'), 3, 0);
  drawicn(dst, (x + 1) * 8, y, geticn('P'), 3, 0);
  for (int i = 0; i < 3; i++) {
    drawicn(dst, (x + 2 + i) * 8, y, geticn(h[i]), 6, 0);
  }
}

void drawwill(Uint32 *dst, int x, int y) {
  char w[3] = "";
  sprintf(w, "%d", character.will);
  drawicn(dst, x * 8, y, geticn('W'), 3, 0);
  drawicn(dst, (x + 1) * 8, y, geticn('P'), 3, 0);
  for (int i = 0; i < 3; i++) {
    drawicn(dst, (x + 2 + i) * 8, y, geticn(w[i]), 2, 0);
  }
}

void drawstats(Uint32 *dst, int x, int y) {
  drawrace(dst, x, y);
  drawclass(dst, x, y + 8);
  drawstr(dst, x, y + 16);
  drawdex(dst, x, y + 24);
  drawwits(dst, x, y + 32);
  drawcha(dst, x, y + 40);
  drawhealth(dst, x + 72, y);
  drawwill(dst, x + 72, y + 8);
  drawgold(dst, x + 72, y + 16);
}

void drawmode(Uint32 *dst, int x, int y) {
  drawicn(dst, x * 8, y, geticn('M'), 3, 0);
  drawicn(dst, (x + 1) * 8, y, geticn('O'), 3, 0);
  drawicn(dst, (x + 2) * 8, y, geticn('D'), 3, 0);
  drawicn(dst, (x + 3) * 8, y, geticn('E'), 3, 0);
  drawicn(dst, (x + 4) * 8, y, geticn(game_mode + 48), 3, 0);
}

void drawroomsquare(Uint32 *dst, int x, int y) {
  for (int i = x; i < 16; i++) {
    for (int j = y; j < 16; j++) {
      putpixel(dst, i, j, 3);
    }
  }
}

void drawroom(Uint32 *dst, Room room) {
  for (int i = 0; i < room.size; i++) {
    drawroomsquare(dst, i * 16, i * 16);
  }
}

void drawcell(Uint32 *dst, int x, int y, Cell cell) {
  for (int i = 0; i < CELL_SIZE; i++) {
    for (int j = 0; j < CELL_SIZE; j++) {
      if (i != 0 && j != 0 && i != CELL_SIZE - 1 && j != CELL_SIZE - 1) {
        putpixel(dst, x + i, y + j, cell.color);
      }
    }
  }
}

void drawgraphpaper(Uint32 *dst, int x, int y) {
  for (int i = 0; i < GRAPH_PAPER_WIDTH; i++) {
    for (int j = 0; j < GRAPH_PAPER_HEIGHT; j++) {
      if (i == 0 || j == 0 || i == GRAPH_PAPER_WIDTH - 1 ||
          j == GRAPH_PAPER_HEIGHT - 1) {
        putpixel(dst, (i * CELL_SIZE) + x, (j * CELL_SIZE) + y, 1);
      }

      drawcell(dst, (i * CELL_SIZE) + x, (j * CELL_SIZE) + y,
               graphpaper.cells[i][j]);

      if (i == selected_cell_i && j == selected_cell_j) {
        putpixel(dst, (i * CELL_SIZE) + x + 5, (j * CELL_SIZE) + y + 5, 6);
      }
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
  drawgraphpaper(dst, GRAPH_PAPER_X, GRAPH_PAPER_X);

  SDL_UpdateTexture(gTexture, NULL, dst, SCREEN_WIDTH * sizeof(Uint32));
  SDL_RenderClear(rend);
  SDL_RenderCopy(rend, gTexture, NULL, NULL);
  SDL_RenderPresent(rend);
}

int main(int argc, char *args[]) {
  (void)argc;
  (void)args;

  if (setup() != 0)
    return error("Setup", "Error");

  game_mode = ROOM_SIZE_MODE;

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
        mouse_x = event.motion.x - 20;
        mouse_y = event.motion.y - 20;
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

void roll_for_gold(Character *character) {
  printf("rolling for initial gold...\n");
  character->gold = dice(2, 6);
}

void erase_graph_paper(GraphPaper *graphpaper) {
  for (int i = 0; i < GRAPH_PAPER_WIDTH; i++) {
    for (int j = 0; j < GRAPH_PAPER_HEIGHT; j++) {
      graphpaper->cells[i][j].color = 4;
    }
  }
}

void gen_room() {
  printf("rolling for room size...\n");
  current_gen_room.size = dice(1, 6);
  printf("rolling for room type...\n");
  current_gen_room.type = dice(1, 6);
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

  fflush(stdout);

  return 0;
}

void update_stuff() {
  if (game_mode == ROOM_SIZE_MODE) {
    if (current_gen_room.size == 0) {
      gen_room();
      printf("room size: %d\n", current_gen_room.size);
    }
  }

  if (mouse_x > GRAPH_PAPER_X && mouse_y > GRAPH_PAPER_Y &&
      mouse_x < GRAPH_PAPER_X + (GRAPH_PAPER_WIDTH * CELL_SIZE) &&
      mouse_y < GRAPH_PAPER_Y + (GRAPH_PAPER_HEIGHT * CELL_SIZE)) {
    selected_cell_i = (mouse_x - GRAPH_PAPER_X) / CELL_SIZE;
    selected_cell_j = (mouse_y - GRAPH_PAPER_Y) / CELL_SIZE;
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

#include <SDL2/SDL.h>
#include <stdio.h>
#include <unistd.h>

/* Screen dimension constants */
const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;

/* a color */
typedef struct rgb {
  unsigned char red;
  unsigned char green;
  unsigned char blue;
} rgb_t;

SDL_Event event;
SDL_Rect rect;
SDL_Renderer *rend;
SDL_Surface *surface = NULL;
SDL_Window *window = NULL;
SDL_bool quit = SDL_FALSE;

Uint32 render_flags =
    0; // SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC;
rgb_t rect_color;

int setup();
void update_stuff();
void draw_stuff();
int screenshot();

int error(char *msg, const char *err) {
  printf("Error %s: %s\n", msg, err);
  return 1;
}

int main(int argc, char *args[]) {
  (void)argc;
  (void)args;

  setup();

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
      default:
        (void)0;
      }
    }

    update_stuff();
    draw_stuff();

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

int setup() {
  if (SDL_Init(SDL_INIT_VIDEO) < 0)
    return error("init", SDL_GetError());

  window = SDL_CreateWindow("Skeleton Cavern", SDL_WINDOWPOS_UNDEFINED,
                            SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH,
                            SCREEN_HEIGHT, SDL_WINDOW_SHOWN);

  if (window == NULL)
    return error("window", SDL_GetError());

  rend = SDL_CreateRenderer(window, -1, render_flags);
  SDL_RenderClear(rend);

  return 0;
}

void update_stuff() {}

void draw_stuff() {
  SDL_SetRenderDrawColor(rend, rect_color.red, rect_color.green,
                         rect_color.blue, 255);
  SDL_RenderFillRect(rend, &rect);
  SDL_RenderDrawRect(rend, &rect);
  SDL_SetRenderDrawColor(rend, 0, 0, 0, 255);

  SDL_RenderPresent(rend);
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

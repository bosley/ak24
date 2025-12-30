#include "kernel/application.h"
#include <SDL.h>
#include <stdbool.h>
#include <stdio.h>

typedef struct {
  SDL_Window *window;
  SDL_Renderer *renderer;
  bool running;
  int box_x;
  int box_y;
  int vel_x;
  int vel_y;
} app_state_t;

static app_state_t *g_state = NULL;
static volatile int keep_running = 1;

APP_ON_SIGNAL(handle_sigint, SIGINT) {
  (void)captured;
  (void)args;
  printf("\n[SIGINT] Shutting down...\n");
  keep_running = 0;
  if (g_state && g_state->window) {
    SDL_Event quit_event;
    quit_event.type = SDL_QUIT;
    SDL_PushEvent(&quit_event);
  }
}

static void handle_events(app_state_t *state) {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    switch (event.type) {
    case SDL_QUIT:
      state->running = false;
      keep_running = 0;
      break;
    case SDL_KEYDOWN:
      switch (event.key.keysym.sym) {
      case SDLK_ESCAPE:
        state->running = false;
        keep_running = 0;
        break;
      case SDLK_SPACE:
        state->vel_x = -state->vel_x;
        state->vel_y = -state->vel_y;
        break;
      }
      break;
    }
  }
}

static void update(app_state_t *state) {
  state->box_x += state->vel_x;
  state->box_y += state->vel_y;

  if (state->box_x <= 0 || state->box_x >= 750) {
    state->vel_x = -state->vel_x;
  }
  if (state->box_y <= 0 || state->box_y >= 550) {
    state->vel_y = -state->vel_y;
  }

  if (state->box_x < 0)
    state->box_x = 0;
  if (state->box_x > 750)
    state->box_x = 750;
  if (state->box_y < 0)
    state->box_y = 0;
  if (state->box_y > 550)
    state->box_y = 550;
}

static void render(app_state_t *state) {
  SDL_SetRenderDrawColor(state->renderer, 20, 20, 60, 255);
  SDL_RenderClear(state->renderer);

  SDL_SetRenderDrawColor(state->renderer, 0, 255, 255, 255);
  SDL_Rect box = {state->box_x, state->box_y, 50, 50};
  SDL_RenderFillRect(state->renderer, &box);

  SDL_RenderPresent(state->renderer);
}

APP_ON_SHUTDOWN(on_shutdown) {
  (void)ctx;

  if (g_state) {
    if (g_state->renderer) {
      SDL_DestroyRenderer(g_state->renderer);
    }
    if (g_state->window) {
      SDL_DestroyWindow(g_state->window);
    }
  }

  SDL_Quit();
  printf("SDL shutdown complete\n");
}

APP_MAIN(app_main) {
  (void)ctx;

  printf("=== AK24 SDL2 Example ===\n");
  printf("Press ESC or Ctrl+C to quit\n");
  printf("Press SPACE to reverse direction\n\n");

  AK24_REGISTER_SIGNAL_HANDLER(handle_sigint);

  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
    return 1;
  }

  g_state = (app_state_t *)AK24_ALLOC(sizeof(app_state_t));
  g_state->running = true;
  g_state->box_x = 400;
  g_state->box_y = 300;
  g_state->vel_x = 3;
  g_state->vel_y = 2;

  g_state->window =
      SDL_CreateWindow("AK24 + SDL2 + Boehm GC", SDL_WINDOWPOS_CENTERED,
                       SDL_WINDOWPOS_CENTERED, 800, 600, SDL_WINDOW_SHOWN);

  if (!g_state->window) {
    fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
    return 1;
  }

  g_state->renderer =
      SDL_CreateRenderer(g_state->window, -1,
                         SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

  if (!g_state->renderer) {
    fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
    return 1;
  }

  while (keep_running && g_state->running) {
    handle_events(g_state);
    update(g_state);
    render(g_state);
    SDL_Delay(16);
  }

  return 0;
}

AK24_APPLICATION("sdl-example", app_main, on_shutdown)

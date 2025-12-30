#include "kernel/application.h"
#include "kernel/list/include/list.h"
#include <SDL.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>

typedef struct {
  float x, y, z;
} vec3_t;

typedef struct {
  int v1, v2;
} edge_t;

typedef struct {
  SDL_Window *window;
  SDL_Renderer *renderer;
  bool running;
  float rot_x;
  float rot_y;
  float rot_z;
  float camera_distance;
  list_t(vec3_t) vertices;
  list_t(edge_t) edges;
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

static int add_vertex(float x, float y, float z) {
  vec3_t v = {x, y, z};
  int idx = list_count(&g_state->vertices);
  list_push(&g_state->vertices, v);
  return idx;
}

static void add_edge(int v1, int v2) {
  edge_t e = {v1, v2};
  list_push(&g_state->edges, e);
}

static void clear_objects(void) {
  list_clear(&g_state->vertices);
  list_clear(&g_state->edges);
}

static vec3_t rotate_x(vec3_t v, float angle) {
  float c = cosf(angle), s = sinf(angle);
  return (vec3_t){v.x, v.y * c - v.z * s, v.y * s + v.z * c};
}

static vec3_t rotate_y(vec3_t v, float angle) {
  float c = cosf(angle), s = sinf(angle);
  return (vec3_t){v.x * c + v.z * s, v.y, -v.x * s + v.z * c};
}

static vec3_t rotate_z(vec3_t v, float angle) {
  float c = cosf(angle), s = sinf(angle);
  return (vec3_t){v.x * c - v.y * s, v.x * s + v.y * c, v.z};
}

static void project(vec3_t v, int *x, int *y) {
  float z = v.z + g_state->camera_distance;
  if (z < 0.1f)
    z = 0.1f;
  *x = (int)(400 + (v.x / z) * 200);
  *y = (int)(300 + (v.y / z) * 200);
}

static void draw_grid_line(SDL_Renderer *renderer, vec3_t p1, vec3_t p2,
                           app_state_t *state) {
  vec3_t v1 = rotate_x(p1, state->rot_x);
  v1 = rotate_y(v1, state->rot_y);
  v1 = rotate_z(v1, state->rot_z);

  vec3_t v2 = rotate_x(p2, state->rot_x);
  v2 = rotate_y(v2, state->rot_y);
  v2 = rotate_z(v2, state->rot_z);

  int x1, y1, x2, y2;
  project(v1, &x1, &y1);
  project(v2, &x2, &y2);
  SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
}

static void draw_grid(app_state_t *state) {
  SDL_SetRenderDrawColor(state->renderer, 40, 40, 40, 255);

  for (int i = -10; i <= 10; i++) {
    draw_grid_line(state->renderer, (vec3_t){i, -10, 0}, (vec3_t){i, 10, 0},
                   state);
    draw_grid_line(state->renderer, (vec3_t){-10, i, 0}, (vec3_t){10, i, 0},
                   state);
  }

  for (int i = -10; i <= 10; i++) {
    draw_grid_line(state->renderer, (vec3_t){i, 0, -10}, (vec3_t){i, 0, 10},
                   state);
    draw_grid_line(state->renderer, (vec3_t){-10, 0, i}, (vec3_t){10, 0, i},
                   state);
  }

  for (int i = -10; i <= 10; i++) {
    draw_grid_line(state->renderer, (vec3_t){0, i, -10}, (vec3_t){0, i, 10},
                   state);
    draw_grid_line(state->renderer, (vec3_t){0, -10, i}, (vec3_t){0, 10, i},
                   state);
  }

  SDL_SetRenderDrawColor(state->renderer, 255, 0, 0, 255);
  draw_grid_line(state->renderer, (vec3_t){0, 0, 0}, (vec3_t){10, 0, 0}, state);

  SDL_SetRenderDrawColor(state->renderer, 0, 255, 0, 255);
  draw_grid_line(state->renderer, (vec3_t){0, 0, 0}, (vec3_t){0, 10, 0}, state);

  SDL_SetRenderDrawColor(state->renderer, 0, 0, 255, 255);
  draw_grid_line(state->renderer, (vec3_t){0, 0, 0}, (vec3_t){0, 0, 10}, state);
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
      case SDLK_UP:
        state->rot_x += 0.1f;
        break;
      case SDLK_DOWN:
        state->rot_x -= 0.1f;
        break;
      case SDLK_LEFT:
        state->rot_y += 0.1f;
        break;
      case SDLK_RIGHT:
        state->rot_y -= 0.1f;
        break;
      case SDLK_w:
        state->camera_distance -= 0.5f;
        if (state->camera_distance < 1.0f)
          state->camera_distance = 1.0f;
        break;
      case SDLK_s:
        state->camera_distance += 0.5f;
        break;
      }
      break;
    }
  }
}

static void update(app_state_t *state) { (void)state; }

static void render(app_state_t *state) {
  SDL_SetRenderDrawColor(state->renderer, 20, 20, 60, 255);
  SDL_RenderClear(state->renderer);

  draw_grid(state);

  SDL_SetRenderDrawColor(state->renderer, 0, 255, 255, 255);

  for (unsigned i = 0; i < list_count(&state->edges); i++) {
    edge_t *edge = list_get(&state->edges, i);
    vec3_t *v1 = list_get(&state->vertices, edge->v1);
    vec3_t *v2 = list_get(&state->vertices, edge->v2);

    vec3_t rv1 = rotate_x(*v1, state->rot_x);
    rv1 = rotate_y(rv1, state->rot_y);
    rv1 = rotate_z(rv1, state->rot_z);

    vec3_t rv2 = rotate_x(*v2, state->rot_x);
    rv2 = rotate_y(rv2, state->rot_y);
    rv2 = rotate_z(rv2, state->rot_z);

    int x1, y1, x2, y2;
    project(rv1, &x1, &y1);
    project(rv2, &x2, &y2);
    SDL_RenderDrawLine(state->renderer, x1, y1, x2, y2);
  }

  SDL_RenderPresent(state->renderer);
}

APP_ON_SHUTDOWN(on_shutdown) {
  (void)ctx;

  if (g_state) {
    list_deinit(&g_state->vertices);
    list_deinit(&g_state->edges);

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

  printf("=== AK24 3D Space Viewer ===\n");
  printf("Press ESC or Ctrl+C to quit\n");
  printf("Use arrow keys to rotate\n");
  printf("Press W/S to zoom in/out\n\n");

  AK24_REGISTER_SIGNAL_HANDLER(handle_sigint);

  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
    return 1;
  }

  g_state = (app_state_t *)AK24_ALLOC(sizeof(app_state_t));
  g_state->running = true;
  g_state->rot_x = 0.0f;
  g_state->rot_y = 0.0f;
  g_state->rot_z = 0.0f;
  g_state->camera_distance = 5.0f;

  list_init(&g_state->vertices);
  list_init(&g_state->edges);

  g_state->window =
      SDL_CreateWindow("AK24 3D Space Viewer", SDL_WINDOWPOS_CENTERED,
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

  int v0 = add_vertex(-2, -2, -2);
  int v1 = add_vertex(0, -2, -2);
  int v2 = add_vertex(0, 0, -2);
  int v3 = add_vertex(-2, 0, -2);
  int v4 = add_vertex(-2, -2, 0);
  int v5 = add_vertex(0, -2, 0);
  int v6 = add_vertex(0, 0, 0);
  int v7 = add_vertex(-2, 0, 0);

  add_edge(v0, v1);
  add_edge(v1, v2);
  add_edge(v2, v3);
  add_edge(v3, v0);
  add_edge(v4, v5);
  add_edge(v5, v6);
  add_edge(v6, v7);
  add_edge(v7, v4);
  add_edge(v0, v4);
  add_edge(v1, v5);
  add_edge(v2, v6);
  add_edge(v3, v7);

  int t0 = add_vertex(2, -2, 0);
  int t1 = add_vertex(4, -2, 0);
  int t2 = add_vertex(3, 0, 0);
  add_edge(t0, t1);
  add_edge(t1, t2);
  add_edge(t2, t0);

  int s0 = add_vertex(1, 2, 0);
  int s1 = add_vertex(3, 2, 0);
  int s2 = add_vertex(3, 4, 0);
  int s3 = add_vertex(1, 4, 0);
  add_edge(s0, s1);
  add_edge(s1, s2);
  add_edge(s2, s3);
  add_edge(s3, s0);

  while (keep_running && g_state->running) {
    handle_events(g_state);
    update(g_state);
    render(g_state);
    SDL_Delay(16);
  }

  return 0;
}

AK24_APPLICATION("sdl-example", app_main, on_shutdown)

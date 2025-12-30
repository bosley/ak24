#include "kernel/application.h"
#include "kernel/buffer/include/buffer.h"
#include "kernel/list/include/list.h"
#include "kernel/scanner/include/scanner.h"
#include <SDL.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  float x, y, z;
} vec3_t;

typedef struct {
  int v1, v2;
} edge_t;

typedef struct {
  char name[64];
  int start_vertex;
  int vertex_count;
} object_t;

typedef struct {
  char object_name[64];
  vec3_t offset;
  int frame;
} frame_command_t;

typedef struct {
  SDL_Window *window;
  SDL_Renderer *renderer;
  bool running;
  float rot_x;
  float rot_y;
  float rot_z;
  float camera_distance;
  list_t(vec3_t) vertices;
  list_t(vec3_t) base_vertices;
  list_t(edge_t) edges;
  list_t(object_t) objects;
  list_t(frame_command_t) frame_commands;
  int current_frame;
  int max_frame;
  bool auto_play;
  Uint32 last_frame_time;
  bool mouse_dragging;
  int last_mouse_x;
  int last_mouse_y;
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
  list_push(&g_state->base_vertices, v);
  return idx;
}

static void add_edge(int v1, int v2) {
  edge_t e = {v1, v2};
  list_push(&g_state->edges, e);
}

static void clear_objects(void) {
  list_clear(&g_state->vertices);
  list_clear(&g_state->base_vertices);
  list_clear(&g_state->edges);
  list_clear(&g_state->objects);
}

static void clear_frame_commands(void) {
  list_clear(&g_state->frame_commands);
  g_state->max_frame = 0;
  g_state->current_frame = 0;
}

static object_t *find_object(const char *name) {
  for (unsigned i = 0; i < list_count(&g_state->objects); i++) {
    object_t *obj = list_get(&g_state->objects, i);
    if (strcmp(obj->name, name) == 0) {
      return obj;
    }
  }
  return NULL;
}

static void apply_frame_commands_up_to(int frame) {
  for (unsigned i = 0; i < list_count(&g_state->vertices); i++) {
    vec3_t *base = list_get(&g_state->base_vertices, i);
    vec3_t *current = list_get(&g_state->vertices, i);
    *current = *base;
  }

  for (unsigned i = 0; i < list_count(&g_state->frame_commands); i++) {
    frame_command_t *cmd = list_get(&g_state->frame_commands, i);
    if (cmd->frame > frame)
      continue;

    object_t *obj = find_object(cmd->object_name);
    if (!obj)
      continue;

    for (int v = 0; v < obj->vertex_count; v++) {
      int idx = obj->start_vertex + v;
      vec3_t *vert = list_get(&g_state->vertices, idx);
      vert->x += cmd->offset.x;
      vert->y += cmd->offset.y;
      vert->z += cmd->offset.z;
    }
  }
}

static float parse_number_from_data(uint8_t *data, size_t len) {
  char buf[64] = {0};
  size_t copy_len = len < 63 ? len : 63;
  memcpy(buf, data, copy_len);
  return atof(buf);
}

static bool parse_vec3(ak_scanner_t *scanner, vec3_t *out) {
  ak_scanner_skip_whitespace_and_comments(scanner);

  ak_scanner_find_group_result_t group =
      ak_scanner_find_group(scanner, '{', '}', NULL, true);
  if (!group.success) {
    return false;
  }

  size_t start = group.index_of_start_symbol + 1;
  size_t end = group.index_of_closing_symbol;
  scanner->position = end + 1;

  ak_buffer_t *buf = scanner->buffer;
  uint8_t *data = ak_buffer_data(buf);

  float x = 0, y = 0, z = 0;
  bool found_x = false, found_y = false, found_z = false;

  for (size_t i = start; i < end; i++) {
    if (data[i] == 'x' && i + 1 < end && data[i + 1] == ':') {
      i += 2;
      while (i < end && (data[i] == ' ' || data[i] == '\t'))
        i++;
      size_t num_start = i;
      while (i < end && (data[i] == '-' || data[i] == '.' ||
                         (data[i] >= '0' && data[i] <= '9')))
        i++;
      x = parse_number_from_data(&data[num_start], i - num_start);
      found_x = true;
    } else if (data[i] == 'y' && i + 1 < end && data[i + 1] == ':') {
      i += 2;
      while (i < end && (data[i] == ' ' || data[i] == '\t'))
        i++;
      size_t num_start = i;
      while (i < end && (data[i] == '-' || data[i] == '.' ||
                         (data[i] >= '0' && data[i] <= '9')))
        i++;
      y = parse_number_from_data(&data[num_start], i - num_start);
      found_y = true;
    } else if (data[i] == 'z' && i + 1 < end && data[i + 1] == ':') {
      i += 2;
      while (i < end && (data[i] == ' ' || data[i] == '\t'))
        i++;
      size_t num_start = i;
      while (i < end && (data[i] == '-' || data[i] == '.' ||
                         (data[i] >= '0' && data[i] <= '9')))
        i++;
      z = parse_number_from_data(&data[num_start], i - num_start);
      found_z = true;
    }
  }

  if (found_x && found_y && found_z) {
    out->x = x;
    out->y = y;
    out->z = z;
    return true;
  }
  return false;
}

static bool parse_def(ak_scanner_t *scanner) {
  ak_scanner_skip_whitespace_and_comments(scanner);

  ak_scanner_static_type_result_t name_result =
      ak_scanner_read_static_base_type(scanner, NULL);
  if (!name_result.success ||
      name_result.data.base != AK24_STATIC_BASE_SYMBOL) {
    return false;
  }

  char obj_name[64] = {0};
  size_t name_len =
      name_result.data.byte_length < 63 ? name_result.data.byte_length : 63;
  memcpy(obj_name, name_result.data.data, name_len);

  ak_scanner_skip_whitespace_and_comments(scanner);

  ak_buffer_t *buf = scanner->buffer;
  uint8_t *data = ak_buffer_data(buf);
  if (scanner->position < ak_buffer_count(buf) &&
      data[scanner->position] == '=') {
    scanner->position++;
  }

  ak_scanner_skip_whitespace_and_comments(scanner);

  ak_scanner_find_group_result_t array_group =
      ak_scanner_find_group(scanner, '[', ']', NULL, true);
  if (!array_group.success) {
    return false;
  }

  size_t array_start = array_group.index_of_start_symbol + 1;
  size_t array_end = array_group.index_of_closing_symbol;
  scanner->position = array_end + 1;

  object_t obj;
  strncpy(obj.name, obj_name, 63);
  obj.start_vertex = list_count(&g_state->vertices);
  obj.vertex_count = 0;

  size_t saved_pos = scanner->position;
  scanner->position = array_start;

  while (scanner->position < array_end) {
    ak_scanner_skip_whitespace_and_comments(scanner);
    if (scanner->position >= array_end)
      break;

    if (data[scanner->position] == '{') {
      vec3_t v;
      if (parse_vec3(scanner, &v)) {
        add_vertex(v.x, v.y, v.z);
        obj.vertex_count++;
      }
    } else {
      scanner->position++;
    }
  }

  scanner->position = saved_pos;

  for (int i = 0; i < obj.vertex_count - 1; i++) {
    add_edge(obj.start_vertex + i, obj.start_vertex + i + 1);
  }
  if (obj.vertex_count > 2) {
    add_edge(obj.start_vertex + obj.vertex_count - 1, obj.start_vertex);
  }

  list_push(&g_state->objects, obj);
  return true;
}

static bool parse_shift(ak_scanner_t *scanner) {
  ak_scanner_skip_whitespace_and_comments(scanner);

  ak_scanner_static_type_result_t name_result =
      ak_scanner_read_static_base_type(scanner, NULL);
  if (!name_result.success ||
      name_result.data.base != AK24_STATIC_BASE_SYMBOL) {
    return false;
  }

  char obj_name[64] = {0};
  size_t name_len =
      name_result.data.byte_length < 63 ? name_result.data.byte_length : 63;
  memcpy(obj_name, name_result.data.data, name_len);

  vec3_t offset;
  if (!parse_vec3(scanner, &offset)) {
    return false;
  }

  frame_command_t cmd;
  strncpy(cmd.object_name, obj_name, 63);
  cmd.offset = offset;
  cmd.frame = g_state->max_frame + 1;

  list_push(&g_state->frame_commands, cmd);
  g_state->max_frame = cmd.frame;

  return true;
}

static bool parse_omgw_file(const char *filepath) {
  ak_buffer_t *buf = ak_buffer_from_file(filepath);
  if (!buf) {
    fprintf(stderr, "Failed to load file: %s\n", filepath);
    return false;
  }

  ak_scanner_t *scanner = ak_scanner_new(buf, 0);
  if (!scanner) {
    ak_buffer_free(buf);
    return false;
  }

  size_t count = ak_buffer_count(buf);

  while (scanner->position < count) {
    if (!ak_scanner_skip_whitespace_and_comments(scanner)) {
      break;
    }

    ak_scanner_static_type_result_t result =
        ak_scanner_read_static_base_type(scanner, NULL);

    if (!result.success)
      break;

    if (result.data.base == AK24_STATIC_BASE_SYMBOL) {
      char keyword[64] = {0};
      size_t kw_len =
          result.data.byte_length < 63 ? result.data.byte_length : 63;
      memcpy(keyword, result.data.data, kw_len);

      if (strcmp(keyword, "DEF") == 0) {
        if (!parse_def(scanner)) {
          fprintf(stderr, "Failed to parse DEF\n");
        }
      } else if (strcmp(keyword, "SHIFT") == 0) {
        if (!parse_shift(scanner)) {
          fprintf(stderr, "Failed to parse SHIFT\n");
        }
      } else if (strcmp(keyword, "CLEAR") == 0) {
        clear_objects();
      } else if (strcmp(keyword, "RESET") == 0) {
        clear_frame_commands();
        apply_frame_commands_up_to(0);
      }
    }
  }

  ak_scanner_free(scanner);
  ak_buffer_free(buf);

  printf("Loaded %u objects, %u vertices, %u edges, %u frames\n",
         list_count(&g_state->objects), list_count(&g_state->vertices),
         list_count(&g_state->edges), g_state->max_frame);

  return true;
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
    case SDL_MOUSEBUTTONDOWN:
      if (event.button.button == SDL_BUTTON_LEFT) {
        state->mouse_dragging = true;
        state->last_mouse_x = event.button.x;
        state->last_mouse_y = event.button.y;
      }
      break;
    case SDL_MOUSEBUTTONUP:
      if (event.button.button == SDL_BUTTON_LEFT) {
        state->mouse_dragging = false;
      }
      break;
    case SDL_MOUSEMOTION:
      if (state->mouse_dragging) {
        int dx = event.motion.x - state->last_mouse_x;
        int dy = event.motion.y - state->last_mouse_y;
        state->rot_y += dx * 0.01f;
        state->rot_x += dy * 0.01f;
        state->last_mouse_x = event.motion.x;
        state->last_mouse_y = event.motion.y;
      }
      break;
    case SDL_MOUSEWHEEL:
      if (event.wheel.y > 0) {
        state->camera_distance -= 0.5f;
        if (state->camera_distance < 1.0f)
          state->camera_distance = 1.0f;
      } else if (event.wheel.y < 0) {
        state->camera_distance += 0.5f;
      }
      break;
    case SDL_KEYDOWN:
      switch (event.key.keysym.sym) {
      case SDLK_ESCAPE:
        state->running = false;
        keep_running = 0;
        break;
      case SDLK_w:
        state->rot_x += 0.1f;
        break;
      case SDLK_s:
        state->rot_x -= 0.1f;
        break;
      case SDLK_a:
        state->rot_y += 0.1f;
        break;
      case SDLK_d:
        state->rot_y -= 0.1f;
        break;
      case SDLK_q:
        state->rot_z += 0.1f;
        break;
      case SDLK_e:
        state->rot_z -= 0.1f;
        break;
      case SDLK_LEFT:
        if (state->current_frame > 0) {
          state->current_frame--;
          apply_frame_commands_up_to(state->current_frame);
        }
        break;
      case SDLK_RIGHT:
        if (state->current_frame < state->max_frame) {
          state->current_frame++;
          apply_frame_commands_up_to(state->current_frame);
        }
        break;
      case SDLK_SPACE:
        state->auto_play = !state->auto_play;
        printf("Auto-play: %s\n", state->auto_play ? "ON" : "OFF");
        break;
      case SDLK_EQUALS:
      case SDLK_PLUS:
        state->camera_distance -= 0.5f;
        if (state->camera_distance < 1.0f)
          state->camera_distance = 1.0f;
        break;
      case SDLK_MINUS:
        state->camera_distance += 0.5f;
        break;
      }
      break;
    }
  }
}

static void update(app_state_t *state) {
  if (state->auto_play && state->max_frame > 0) {
    Uint32 current_time = SDL_GetTicks();
    if (current_time - state->last_frame_time > 500) {
      state->current_frame++;
      if (state->current_frame > state->max_frame) {
        state->current_frame = 0;
      }
      apply_frame_commands_up_to(state->current_frame);
      state->last_frame_time = current_time;
    }
  }
}

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
    list_deinit(&g_state->base_vertices);
    list_deinit(&g_state->edges);
    list_deinit(&g_state->objects);
    list_deinit(&g_state->frame_commands);

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
  if (list_count(&ctx->args) < 2) {
    char **argv0 = list_get(&ctx->args, 0);
    fprintf(stderr, "Usage: %s <omgw_file>\n", argv0 ? *argv0 : "sdl-example");
    fprintf(stderr, "Example: %s examples/sdl-example/worlds/first.omgw\n",
            argv0 ? *argv0 : "sdl-example");
    return 1;
  }

  printf("=== AK24 3D Space Viewer ===\n");
  printf("Press ESC or Ctrl+C to quit\n");
  printf("Mouse: Click and drag to rotate, scroll wheel to zoom\n");
  printf("WASD: Rotate view (Q/E for Z-axis)\n");
  printf("+/- keys: Zoom in/out\n");
  printf("Space: Toggle auto-play\n");
  printf("Left/Right arrows: Step through frames\n\n");

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
  g_state->current_frame = 0;
  g_state->max_frame = 0;
  g_state->auto_play = false;
  g_state->last_frame_time = 0;
  g_state->mouse_dragging = false;
  g_state->last_mouse_x = 0;
  g_state->last_mouse_y = 0;

  list_init(&g_state->vertices);
  list_init(&g_state->base_vertices);
  list_init(&g_state->edges);
  list_init(&g_state->objects);
  list_init(&g_state->frame_commands);

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

  char **omgw_file = list_get(&ctx->args, 1);
  if (!parse_omgw_file(*omgw_file)) {
    fprintf(stderr, "Failed to parse OMGW file: %s\n", *omgw_file);
    return 1;
  }

  g_state->last_frame_time = SDL_GetTicks();

  while (keep_running && g_state->running) {
    handle_events(g_state);
    update(g_state);
    render(g_state);
    SDL_Delay(16);
  }

  return 0;
}

AK24_APPLICATION("sdl-example", app_main, on_shutdown)

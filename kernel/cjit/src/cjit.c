#include "cjit.h"
#include "kernel.h"

#include <libtcc.h>
#include <string.h>

struct ak_cjit_unit_s {
  TCCState *tcc;
  ak_cjit_state_e state;
  ak_cjit_error_fn error_fn;
  void *error_ctx;
};

static void tcc_error_handler(void *opaque, const char *msg) {
  ak_cjit_unit_t *unit = (ak_cjit_unit_t *)opaque;
  if (unit && unit->error_fn) {
    unit->error_fn(unit->error_ctx, msg);
  }
}

bool ak_cjit_available(void) { return true; }

const char *ak_cjit_backend_name(void) { return "tcc"; }

ak_cjit_config_t ak_cjit_config_default(void) {
  ak_cjit_config_t config = {0};
  config.debug_symbols = false;
  return config;
}

ak_cjit_unit_t *ak_cjit_unit_new(const ak_cjit_config_t *config,
                                 ak_cjit_error_fn error_fn, void *error_ctx) {
  ak_cjit_unit_t *unit = (ak_cjit_unit_t *)AK24_ALLOC(sizeof(ak_cjit_unit_t));
  if (!unit) {
    return NULL;
  }

  memset(unit, 0, sizeof(ak_cjit_unit_t));
  unit->error_fn = error_fn;
  unit->error_ctx = error_ctx;
  unit->state = AK_CJIT_STATE_INIT;

  unit->tcc = tcc_new();
  if (!unit->tcc) {
    AK24_FREE(unit);
    return NULL;
  }

  tcc_set_error_func(unit->tcc, unit, tcc_error_handler);
  tcc_set_output_type(unit->tcc, TCC_OUTPUT_MEMORY);

  if (config) {
    if (config->debug_symbols) {
      tcc_set_options(unit->tcc, "-g");
    }

    for (size_t i = 0; i < config->include_path_count; i++) {
      if (config->include_paths[i]) {
        tcc_add_include_path(unit->tcc, config->include_paths[i]);
      }
    }

    for (size_t i = 0; i < config->library_path_count; i++) {
      if (config->library_paths[i]) {
        tcc_add_library_path(unit->tcc, config->library_paths[i]);
      }
    }

    for (size_t i = 0; i < config->library_count; i++) {
      if (config->libraries[i]) {
        tcc_add_library(unit->tcc, config->libraries[i]);
      }
    }

    for (size_t i = 0; i < config->define_count; i++) {
      if (config->defines[i]) {
        const char *def = config->defines[i];
        const char *eq = strchr(def, '=');
        if (eq) {
          size_t name_len = (size_t)(eq - def);
          char *name = (char *)AK24_ALLOC(name_len + 1);
          if (name) {
            memcpy(name, def, name_len);
            name[name_len] = '\0';
            tcc_define_symbol(unit->tcc, name, eq + 1);
            AK24_FREE(name);
          }
        } else {
          tcc_define_symbol(unit->tcc, def, NULL);
        }
      }
    }
  }

  return unit;
}

void ak_cjit_unit_free(ak_cjit_unit_t *unit) {
  if (!unit) {
    return;
  }

  if (unit->tcc) {
    tcc_delete(unit->tcc);
  }

  AK24_FREE(unit);
}

ak_cjit_state_e ak_cjit_unit_state(const ak_cjit_unit_t *unit) {
  if (!unit) {
    return AK_CJIT_STATE_ERROR;
  }
  return unit->state;
}

ak_cjit_error_e ak_cjit_add_source(ak_cjit_unit_t *unit, const char *source,
                                   const char *filename) {
  if (!unit || !source) {
    return AK_CJIT_ERROR_INVALID_STATE;
  }

  if (unit->state == AK_CJIT_STATE_RELOCATED ||
      unit->state == AK_CJIT_STATE_ERROR) {
    return AK_CJIT_ERROR_INVALID_STATE;
  }

  const char *fname = filename ? filename : "<string>";

  if (tcc_compile_string(unit->tcc, source) == -1) {
    unit->state = AK_CJIT_STATE_ERROR;
    return AK_CJIT_ERROR_COMPILE;
  }

  (void)fname;

  unit->state = AK_CJIT_STATE_COMPILED;
  return AK_CJIT_OK;
}

ak_cjit_error_e ak_cjit_add_file(ak_cjit_unit_t *unit, const char *filepath) {
  if (!unit || !filepath) {
    return AK_CJIT_ERROR_INVALID_STATE;
  }

  if (unit->state == AK_CJIT_STATE_RELOCATED ||
      unit->state == AK_CJIT_STATE_ERROR) {
    return AK_CJIT_ERROR_INVALID_STATE;
  }

  if (tcc_add_file(unit->tcc, filepath) == -1) {
    unit->state = AK_CJIT_STATE_ERROR;
    return AK_CJIT_ERROR_COMPILE;
  }

  unit->state = AK_CJIT_STATE_COMPILED;
  return AK_CJIT_OK;
}

ak_cjit_error_e ak_cjit_add_symbol(ak_cjit_unit_t *unit, const char *name,
                                   void *ptr) {
  if (!unit || !name || !ptr) {
    return AK_CJIT_ERROR_INVALID_STATE;
  }

  if (unit->state == AK_CJIT_STATE_RELOCATED ||
      unit->state == AK_CJIT_STATE_ERROR) {
    return AK_CJIT_ERROR_INVALID_STATE;
  }

  tcc_add_symbol(unit->tcc, name, ptr);
  return AK_CJIT_OK;
}

ak_cjit_error_e ak_cjit_relocate(ak_cjit_unit_t *unit) {
  if (!unit) {
    return AK_CJIT_ERROR_INVALID_STATE;
  }

  if (unit->state != AK_CJIT_STATE_COMPILED) {
    return AK_CJIT_ERROR_INVALID_STATE;
  }

  if (tcc_relocate(unit->tcc) < 0) {
    unit->state = AK_CJIT_STATE_ERROR;
    return AK_CJIT_ERROR_RELOCATE;
  }

  unit->state = AK_CJIT_STATE_RELOCATED;
  return AK_CJIT_OK;
}

void *ak_cjit_get_symbol(ak_cjit_unit_t *unit, const char *name) {
  if (!unit || !name) {
    return NULL;
  }

  if (unit->state != AK_CJIT_STATE_RELOCATED) {
    return NULL;
  }

  return tcc_get_symbol(unit->tcc, name);
}

ak_cjit_error_e ak_cjit_add_include_path(ak_cjit_unit_t *unit,
                                         const char *path) {
  if (!unit || !path) {
    return AK_CJIT_ERROR_INVALID_STATE;
  }

  if (unit->state == AK_CJIT_STATE_RELOCATED ||
      unit->state == AK_CJIT_STATE_ERROR) {
    return AK_CJIT_ERROR_INVALID_STATE;
  }

  tcc_add_include_path(unit->tcc, path);
  return AK_CJIT_OK;
}

ak_cjit_error_e ak_cjit_add_library_path(ak_cjit_unit_t *unit,
                                         const char *path) {
  if (!unit || !path) {
    return AK_CJIT_ERROR_INVALID_STATE;
  }

  if (unit->state == AK_CJIT_STATE_RELOCATED ||
      unit->state == AK_CJIT_STATE_ERROR) {
    return AK_CJIT_ERROR_INVALID_STATE;
  }

  tcc_add_library_path(unit->tcc, path);
  return AK_CJIT_OK;
}

ak_cjit_error_e ak_cjit_add_library(ak_cjit_unit_t *unit, const char *name) {
  if (!unit || !name) {
    return AK_CJIT_ERROR_INVALID_STATE;
  }

  if (unit->state == AK_CJIT_STATE_RELOCATED ||
      unit->state == AK_CJIT_STATE_ERROR) {
    return AK_CJIT_ERROR_INVALID_STATE;
  }

  tcc_add_library(unit->tcc, name);
  return AK_CJIT_OK;
}

ak_cjit_error_e ak_cjit_define(ak_cjit_unit_t *unit, const char *name,
                               const char *value) {
  if (!unit || !name) {
    return AK_CJIT_ERROR_INVALID_STATE;
  }

  if (unit->state == AK_CJIT_STATE_RELOCATED ||
      unit->state == AK_CJIT_STATE_ERROR) {
    return AK_CJIT_ERROR_INVALID_STATE;
  }

  tcc_define_symbol(unit->tcc, name, value);
  return AK_CJIT_OK;
}

const char *ak_cjit_error_string(ak_cjit_error_e error) {
  switch (error) {
  case AK_CJIT_OK:
    return "Success";
  case AK_CJIT_ERROR_ALLOC:
    return "Memory allocation failed";
  case AK_CJIT_ERROR_COMPILE:
    return "Compilation failed";
  case AK_CJIT_ERROR_RELOCATE:
    return "Relocation failed";
  case AK_CJIT_ERROR_SYMBOL_NOT_FOUND:
    return "Symbol not found";
  case AK_CJIT_ERROR_INVALID_STATE:
    return "Invalid state for operation";
  case AK_CJIT_ERROR_NOT_AVAILABLE:
    return "CJIT not available";
  default:
    return "Unknown error";
  }
}

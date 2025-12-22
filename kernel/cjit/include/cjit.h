/**
 * @file cjit.h
 * @brief C JIT compilation abstraction layer
 *
 * Provides runtime C compilation without exposing the underlying
 * implementation. Allows compiling C source to callable functions
 * using a defined contract.
 *
 * Key features:
 * - Compile C source from strings or files at runtime
 * - Retrieve function pointers by symbol name
 * - Add host symbols for JIT code to call back into
 * - Configurable include paths, libraries, and defines
 * - Error reporting via callbacks
 *
 * @note JIT units must remain allocated while symbols are in use
 * @note This module abstracts the underlying JIT implementation
 */

#ifndef AK24_KERNEL_CJIT_H
#define AK24_KERNEL_CJIT_H

#include <stdbool.h>
#include <stddef.h>

#define AK_CJIT_VERSION "0.0.1-dev"

typedef enum {
  AK_CJIT_STATE_INIT = 0,
  AK_CJIT_STATE_COMPILED,
  AK_CJIT_STATE_RELOCATED,
  AK_CJIT_STATE_ERROR
} ak_cjit_state_e;

typedef enum {
  AK_CJIT_OK = 0,
  AK_CJIT_ERROR_ALLOC,
  AK_CJIT_ERROR_COMPILE,
  AK_CJIT_ERROR_RELOCATE,
  AK_CJIT_ERROR_SYMBOL_NOT_FOUND,
  AK_CJIT_ERROR_INVALID_STATE,
  AK_CJIT_ERROR_NOT_AVAILABLE
} ak_cjit_error_e;

typedef struct ak_cjit_unit_s ak_cjit_unit_t;

typedef struct {
  const char **include_paths;
  size_t include_path_count;
  const char **library_paths;
  size_t library_path_count;
  const char **libraries;
  size_t library_count;
  const char **defines;
  size_t define_count;
  bool debug_symbols;
} ak_cjit_config_t;

typedef void (*ak_cjit_error_fn)(void *ctx, const char *message);

/**
 * @brief Check if CJIT is available
 *
 * @return true if CJIT support is compiled in
 */
bool ak_cjit_available(void);

/**
 * @brief Get CJIT backend name
 *
 * @return Backend identifier string (e.g., "tcc", "none")
 */
const char *ak_cjit_backend_name(void);

/**
 * @brief Create default configuration
 *
 * @return Configuration with sensible defaults
 */
ak_cjit_config_t ak_cjit_config_default(void);

/**
 * @brief Create a new JIT compilation unit
 *
 * @param config Configuration (NULL for defaults)
 * @param error_fn Error callback (NULL to suppress)
 * @param error_ctx Context passed to error callback
 * @return New unit or NULL on failure
 *
 * @notthreadsafe
 */
ak_cjit_unit_t *ak_cjit_unit_new(const ak_cjit_config_t *config,
                                 ak_cjit_error_fn error_fn, void *error_ctx);

/**
 * @brief Free a JIT compilation unit
 *
 * @param unit Unit to free (NULL-safe)
 *
 * @warning All symbols become invalid after this call
 *
 * @notthreadsafe
 */
void ak_cjit_unit_free(ak_cjit_unit_t *unit);

/**
 * @brief Get current unit state
 *
 * @param unit Compilation unit
 * @return Current state
 */
ak_cjit_state_e ak_cjit_unit_state(const ak_cjit_unit_t *unit);

/**
 * @brief Add source code from a string
 *
 * Can be called multiple times before relocate. Each call adds more
 * source code to the compilation unit.
 *
 * @param unit Compilation unit
 * @param source C source code (null-terminated)
 * @param filename Virtual filename for error messages
 * @return AK_CJIT_OK on success, error code otherwise
 *
 * @notthreadsafe
 */
ak_cjit_error_e ak_cjit_add_source(ak_cjit_unit_t *unit, const char *source,
                                   const char *filename);

/**
 * @brief Add source code from a file
 *
 * @param unit Compilation unit
 * @param filepath Path to C source file
 * @return AK_CJIT_OK on success, error code otherwise
 *
 * @notthreadsafe
 */
ak_cjit_error_e ak_cjit_add_file(ak_cjit_unit_t *unit, const char *filepath);

/**
 * @brief Add a host symbol for JIT code to reference
 *
 * Makes a symbol from the host application available to JIT-compiled
 * code. Must be called before relocate.
 *
 * @param unit Compilation unit
 * @param name Symbol name as it appears in JIT source
 * @param ptr Pointer to the symbol (function or data)
 * @return AK_CJIT_OK on success, error code otherwise
 *
 * @notthreadsafe
 */
ak_cjit_error_e ak_cjit_add_symbol(ak_cjit_unit_t *unit, const char *name,
                                   void *ptr);

/**
 * @brief Relocate the compilation unit
 *
 * Finalizes compilation and makes symbols available. Must be called
 * after all source is added and before retrieving symbols.
 *
 * @param unit Compilation unit
 * @return AK_CJIT_OK on success, error code otherwise
 *
 * @notthreadsafe
 */
ak_cjit_error_e ak_cjit_relocate(ak_cjit_unit_t *unit);

/**
 * @brief Get a symbol from the compiled unit
 *
 * Retrieves a function or data pointer by name. Unit must be in
 * AK_CJIT_STATE_RELOCATED state.
 *
 * @param unit Compilation unit
 * @param name Symbol name
 * @return Symbol pointer or NULL if not found
 *
 * @note Returned pointer is only valid while unit is alive
 *
 * @notthreadsafe
 */
void *ak_cjit_get_symbol(ak_cjit_unit_t *unit, const char *name);

/**
 * @brief Add an include path
 *
 * @param unit Compilation unit
 * @param path Include directory path
 * @return AK_CJIT_OK on success, error code otherwise
 *
 * @notthreadsafe
 */
ak_cjit_error_e ak_cjit_add_include_path(ak_cjit_unit_t *unit,
                                         const char *path);

/**
 * @brief Add a library path
 *
 * @param unit Compilation unit
 * @param path Library directory path
 * @return AK_CJIT_OK on success, error code otherwise
 *
 * @notthreadsafe
 */
ak_cjit_error_e ak_cjit_add_library_path(ak_cjit_unit_t *unit,
                                         const char *path);

/**
 * @brief Add a library to link
 *
 * @param unit Compilation unit
 * @param name Library name (without lib prefix or extension)
 * @return AK_CJIT_OK on success, error code otherwise
 *
 * @notthreadsafe
 */
ak_cjit_error_e ak_cjit_add_library(ak_cjit_unit_t *unit, const char *name);

/**
 * @brief Add a preprocessor define
 *
 * @param unit Compilation unit
 * @param name Define name
 * @param value Define value (NULL for empty define)
 * @return AK_CJIT_OK on success, error code otherwise
 *
 * @notthreadsafe
 */
ak_cjit_error_e ak_cjit_define(ak_cjit_unit_t *unit, const char *name,
                               const char *value);

/**
 * @brief Get error string for error code
 *
 * @param error Error code
 * @return Static error string
 */
const char *ak_cjit_error_string(ak_cjit_error_e error);

#endif

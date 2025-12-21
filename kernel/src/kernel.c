#include "kernel.h"

#if AK24_GC_ENABLED
#include <sched.h>
#include <unistd.h>
#define GC_THREADS 1
#include <gc.h>
#endif

// Forward declarations from platform-specific files
void ak_signal_handlers_init(void);
void ak_signal_handlers_deinit(void);

#ifdef AK24_PLATFORM_WINDOWS
void ak_print_warning(const char *format, ...);
#endif

static list_void_t shutdown_lambdas;
static int shutdown_lambdas_initialized = 0;
static time_t kernel_start_time;

#if AK24_GC_ENABLED

void ak_kernel_init(void) {
  kernel_start_time = time(NULL);
#ifdef AK24_PLATFORM_WINDOWS
  ak_print_warning("\n*** WARNING ***\n");
  ak_print_warning("AK24 Windows support is UNTESTED CODE\n");
  ak_print_warning("Please report any issues to the development team\n");
  ak_print_warning("***************\n\n");
#endif
  GC_set_warn_proc(GC_ignore_warn_proc);
  GC_INIT();
  list_init(&shutdown_lambdas);
  shutdown_lambdas_initialized = 1;
  ak_signal_handlers_init();
}

void ak_kernel_deinit(void) {
  if (shutdown_lambdas_initialized) {
    kernel_shutdown_info_t info;
    info.start_time = kernel_start_time;
#if AK24_BUILD_DEBUG_MEMORY
    info.memory_stats = ak_mem_get_stats();
#endif

    list_iter_t iter = list_iter(&shutdown_lambdas);
    void **lambda_ptr;
    while ((lambda_ptr = list_next(&shutdown_lambdas, &iter))) {
      ak_lambda_t *lambda = (ak_lambda_t *)*lambda_ptr;
      if (lambda) {
        ak_lambda_invoke(lambda, &info);
      }
    }
    list_deinit(&shutdown_lambdas);
    shutdown_lambdas_initialized = 0;
  }
  ak_signal_handlers_deinit();
  sched_yield();
  sched_yield();
}

#else

void ak_kernel_init(void) {
  kernel_start_time = time(NULL);
#ifdef AK24_PLATFORM_WINDOWS
  ak_print_warning("\n*** WARNING ***\n");
  ak_print_warning("AK24 Windows support is UNTESTED CODE\n");
  ak_print_warning("Please report any issues to the development team\n");
  ak_print_warning("***************\n\n");
#endif
  list_init(&shutdown_lambdas);
  shutdown_lambdas_initialized = 1;
  ak_signal_handlers_init();
}

void ak_kernel_deinit(void) {
  if (shutdown_lambdas_initialized) {
    kernel_shutdown_info_t info;
    info.start_time = kernel_start_time;
#if AK24_BUILD_DEBUG_MEMORY
    info.memory_stats = ak_mem_get_stats();
#endif

    list_iter_t iter = list_iter(&shutdown_lambdas);
    void **lambda_ptr;
    while ((lambda_ptr = list_next(&shutdown_lambdas, &iter))) {
      ak_lambda_t *lambda = (ak_lambda_t *)*lambda_ptr;
      if (lambda) {
        ak_lambda_invoke(lambda, &info);
      }
    }
    list_deinit(&shutdown_lambdas);
    shutdown_lambdas_initialized = 0;
  }
  ak_signal_handlers_deinit();
}

#endif

void ak_on_shutdown(ak_lambda_t *lambda) {
  if (!lambda || !shutdown_lambdas_initialized) {
    return;
  }
  list_push(&shutdown_lambdas, lambda);
}

list_str_t ak_args_to_list(int argc, char **argv) {
  list_str_t args;
  list_init(&args);
  for (int i = 0; i < argc; i++) {
    list_push(&args, argv[i]);
  }
  return args;
}

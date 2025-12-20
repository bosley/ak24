/**
 * @file lambda.h
 * @brief Function closures with captured context
 *
 * Provides a lambda/closure system allowing functions to capture and carry
 * context data. Supports custom cleanup functions for captured context and
 * lambda cloning for callback registration.
 *
 * Key features:
 * - Function pointers with captured context
 * - Custom context cleanup functions
 * - Lambda cloning for multiple registrations
 * - Context management and inspection
 * - Flexible invocation with runtime arguments
 *
 * @note Lambdas themselves are NOT thread-safe, but can be used in
 *       thread-safe contexts if the captured context is properly synchronized
 */

#ifndef AK24_LAMBDA_H
#define AK24_LAMBDA_H

/**
 * @def AK24_LAMBDA_VERSION
 * @brief Lambda module version string
 */
#define LAMBDA_VERSION "0.0.1-dev"

/**
 * @brief Lambda function signature
 *
 * @param captured_ctx Context captured at lambda creation
 * @param invoke_args Arguments passed during invocation
 */
typedef void (*ak_lambda_fn)(void *captured_ctx, void *invoke_args);

/**
 * @brief Context cleanup function signature
 *
 * Called when lambda is freed to clean up captured context.
 *
 * @param ctx Context to clean up
 */
typedef void (*ak_lambda_ctx_free_fn)(void *ctx);

/**
 * @brief Lambda structure with function and captured context
 *
 * Represents a function closure that carries context data.
 */
typedef struct ak_lambda_t {
  ak_lambda_fn fn;                /**< Function to execute */
  void *captured_ctx;             /**< Captured context data */
  ak_lambda_ctx_free_fn ctx_free; /**< Context cleanup function */
} ak_lambda_t;

/**
 * @brief Create a new lambda
 *
 * Allocates and initializes a lambda with the specified function and
 * captured context. The context cleanup function is optional.
 *
 * @param fn Function to execute when lambda is invoked
 * @param captured_ctx Context to capture (can be NULL)
 * @param ctx_free Cleanup function for context (can be NULL)
 * @return Pointer to new lambda, or NULL on allocation failure
 *
 * @notthreadsafe
 *
 * @note Caller must free with ak_lambda_free()
 *
 * @par Example:
 * @code
 * typedef struct {
 *   int count;
 * } counter_t;
 *
 * void count_fn(void *ctx, void *args) {
 *   counter_t *c = (counter_t *)ctx;
 *   c->count++;
 *   printf("Count: %d\n", c->count);
 * }
 *
 * counter_t *ctx = malloc(sizeof(counter_t));
 * ctx->count = 0;
 * ak_lambda_t *lambda = ak_lambda_new(count_fn, ctx, free);
 * @endcode
 */
ak_lambda_t *ak_lambda_new(ak_lambda_fn fn, void *captured_ctx,
                           ak_lambda_ctx_free_fn ctx_free);

/**
 * @brief Invoke a lambda with arguments
 *
 * Executes the lambda's function, passing both captured context and
 * invocation arguments.
 *
 * @param lambda Lambda to invoke
 * @param invoke_args Arguments to pass to function (can be NULL)
 *
 * @notthreadsafe
 *
 * @par Example:
 * @code
 * ak_lambda_invoke(lambda, NULL);
 * @endcode
 */
void ak_lambda_invoke(ak_lambda_t *lambda, void *invoke_args);

/**
 * @brief Free a lambda
 *
 * Releases lambda and calls context cleanup function if provided.
 *
 * @param lambda Lambda to free (NULL is safe)
 *
 * @notthreadsafe
 */
void ak_lambda_free(ak_lambda_t *lambda);

/**
 * @brief Create a shallow clone of lambda
 *
 * Creates a new lambda with the same function and context pointers.
 * The context is NOT deep copied - both lambdas share the same context.
 * The clone will NOT free the context when freed.
 *
 * @param lambda Lambda to clone
 * @return New lambda sharing context, or NULL on failure
 *
 * @notthreadsafe
 *
 * @warning Both lambdas share the same context pointer
 */
ak_lambda_t *ak_lambda_clone(ak_lambda_t *lambda);

/**
 * @brief Check if lambda has captured context
 *
 * @param lambda Lambda to check
 * @return 1 if context is non-NULL, 0 otherwise
 *
 * @notthreadsafe
 */
int ak_lambda_has_context(ak_lambda_t *lambda);

/**
 * @brief Get lambda's captured context
 *
 * Returns direct pointer to captured context.
 *
 * @param lambda Lambda to query
 * @return Pointer to context, or NULL if none
 *
 * @notthreadsafe
 */
void *ak_lambda_get_context(ak_lambda_t *lambda);

/**
 * @brief Replace lambda's captured context
 *
 * Updates the lambda's context and cleanup function. If the lambda
 * already has a context with a cleanup function, the old context
 * is freed first.
 *
 * @param lambda Lambda to modify
 * @param captured_ctx New context to capture
 * @param ctx_free New cleanup function (can be NULL)
 * @return 0 on success, -1 if lambda is NULL
 *
 * @notthreadsafe
 *
 * @warning Old context is freed if cleanup function was set
 */
int ak_lambda_set_context(ak_lambda_t *lambda, void *captured_ctx,
                          ak_lambda_ctx_free_fn ctx_free);

#endif

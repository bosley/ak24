# Context

The context provides a hierarchical scoped key-value store with parent-child relationships. It supports variable shadowing, hoisting values to parent scopes, and scope traversal.

## Core Concept

A context is a scoped environment that:
- Stores key-value pairs (string keys, void* values)
- Has optional parent context for scope chaining
- Supports variable shadowing (child values override parent)
- Can hoist values to parent scope on pop
- Tracks scope depth
- Uses map for storage and list for hoist queue

## Usage

```c
ak_context_t *ctx = ak_context_new();

int value = 42;
ak_context_set(ctx, "x", &value);

int *retrieved = (int *)ak_context_get(ctx, "x");
printf("x = %d\n", *retrieved);

ak_context_free(ctx);
```

## Scope Chaining

```c
ak_context_t *parent = ak_context_new();
ak_context_set(parent, "x", &value1);

ak_context_t *child = ak_context_push(parent);
ak_context_set(child, "y", &value2);

ak_context_get(child, "x");
ak_context_get(child, "y");

parent = ak_context_pop(child);
```

## Variable Shadowing

```c
ak_context_t *parent = ak_context_new();
int x = 10;
ak_context_set(parent, "x", &x);

ak_context_t *child = ak_context_push(parent);
int y = 20;
ak_context_set(child, "x", &y);

int *val = (int *)ak_context_get(child, "x");
```

## Hoisting

```c
ak_context_t *parent = ak_context_new();
ak_context_t *child = ak_context_push(parent);

int value = 42;
ak_context_set(child, "hoisted", &value);
ak_context_hoist(child, "hoisted");

parent = ak_context_pop(child);

int *val = (int *)ak_context_get(parent, "hoisted");
```

## Local vs Scoped Access

```c
ak_context_t *parent = ak_context_new();
ak_context_set(parent, "x", &value);

ak_context_t *child = ak_context_push(parent);

ak_context_get(child, "x");

ak_context_get_local(child, "x");
```

## Checking Existence

```c
if (ak_context_has(ctx, "key")) {
  void *val = ak_context_get(ctx, "key");
}

if (ak_context_has_local(ctx, "key")) {
  void *val = ak_context_get_local(ctx, "key");
}
```

## Finding Containing Context

```c
ak_context_t *containing = ak_context_get_containing_context(ctx, "key");
if (containing) {
  printf("Found at depth %u\n", ak_context_depth(containing));
}
```

## API

- `ak_context_new()` - Create new context
- `ak_context_push(ctx)` - Create child context
- `ak_context_pop(ctx)` - Pop context and hoist queued values
- `ak_context_set(ctx, key, value)` - Set value in current scope
- `ak_context_get(ctx, key)` - Get value (searches up scope chain)
- `ak_context_get_local(ctx, key)` - Get value from current scope only
- `ak_context_get_containing_context(ctx, key)` - Find context containing key
- `ak_context_hoist(ctx, key)` - Queue key to hoist to parent on pop
- `ak_context_has(ctx, key)` - Check if key exists (searches up chain)
- `ak_context_has_local(ctx, key)` - Check if key exists in current scope
- `ak_context_depth(ctx)` - Get scope depth
- `ak_context_free(ctx)` - Free context (does not free parent)

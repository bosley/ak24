# Forms System

The forms library provides structural type system infrastructure for compiler construction.

## Purpose

Forms are building blocks for implementing object systems in programming languages. They provide:

- **Structural types** - data layout without nominal identity
- **Affordances** - operations available on types
- **Affects** - method definitions with composition via `also`
- **Pattern matching** - structural compatibility checking

## Core Concepts

### Forms (Types)

Forms represent type structure at compile-time:

- **Primitive**: `bool`, `u8`-`u64`, `i8`-`i64`, `f32`, `f64`
- **Compound**: juxtaposition of forms (e.g., `i32 i32`)
- **Optional**: `none | form` - provides `is_none()`, `is_some()`
- **Repeatable**: `form...` - provides `append()`, `pop()`, `push()`, `rest()`
- **Struct**: named overlay on a pattern (compile-time field mapping)
- **List**: `list form` - provides `index()`, `length()`
- **Map**: `map key_type value_type` - provides `get()`, `set()`, `has()`
- **Named**: reference to another form

### Affects (Methods)

Affects are method definitions attached to forms:

- Stored on form objects (mutable - "pinning affects on the donkey")
- Contain lambdas that operate on instances
- Can be composed using `also` to include methods from other affects

### Affordances

Affordances are the operations available on a type:

- **Structural affordances**: automatically derived from form structure
- **Explicit affordances**: user-defined affects attached to forms

### Instances (Runtime Values)

Instances are runtime values that reference their form:

- Hold actual data
- Passed to affect lambdas for mutation
- Form pointer determines available affordances

## Ownership Model

Forms use deep-copy semantics with explicit ownership:

- **Form constructors deep-copy inputs**: When you create a form with `ak_form_new_*()`, it clones all child forms. The caller retains ownership of the inputs.
- **Caller owns returned forms**: The form returned by constructors must be freed by the caller with `ak_form_free()`.
- **Deep-free is safe**: Since each form owns its children, `ak_form_free()` recursively frees the entire form tree.
- **Contexts are lookup-only**: Registering a form in a context with `ak_form_register()` does not transfer ownership - the caller must still free the form.
- **root_form_ctx manages primitives**: Use `ak_root_form_ctx_new()` to get a context that owns and manages primitive forms. Call `ak_root_form_ctx_free()` to clean up.
- **Instances reference forms**: Runtime instances hold pointers to forms, which must outlive the instances.

## Structural Typing

Forms are matched by structure, not by name. Two forms are compatible if they have the same structure.

```c
ak_form_t *form1 = ak_form_new_compound(...); // i32 i32
ak_form_t *form2 = ak_form_new_compound(...); // i32 i32
ak_form_is_compatible(form1, form2); // true - same structure

ak_form_free(form1);
ak_form_free(form2);
```

## Usage Example

```c
root_form_ctx_t *root = ak_root_form_ctx_new();

ak_form_t *i32 = ak_root_form_ctx_get_i32(root);
ak_form_t *optional_i32 = ak_form_new_optional(i32);

ak_root_form_ctx_free(root);
ak_form_free(optional_i32);
```

## Struct Forms

Structs are compile-time metadata that overlay field names onto structural patterns:

```c
ak_form_t *pattern = ak_form_new_compound(...); // i32 str
ak_form_t *person = ak_form_new_struct(pattern, ["age", "name"]);
```

At runtime, instances use the same data layout as the underlying pattern. Struct forms provide field name affordances for compile-time name-to-index mapping.

## Design Philosophy

The forms system is intentionally minimal and language-agnostic. It provides primitives that can express various object systems:

- Classical OOP (classes, inheritance)
- Trait-based composition
- Prototype-based delegation
- Duck typing

The language designer decides how to map language constructs onto forms and affects.

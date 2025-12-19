#ifndef AK24_ATOM_H
#define AK24_ATOM_H

#include <stdatomic.h>
#include <stdint.h>

/*
atoms are:
     u8 u16 u32 u64 i8 i16 i32 i64 f32 f64 char byte

atomic-forms:
     any
     numeric         i f
     unsigned        u
     signed          i f
     real            f32 f64
     symbolic        char


The idea is this:

    in classic computing they do

          item -> item -> item -> item


    Pur atoms can do this if the atom dimentionality is 1

    Our atoms work in any N dimensions. so "next" actually returns a 1-dim of
all neighbors at the "next" level" emminating outwards from the atom




*/

typedef enum {
  AK24_ATOM_BYTE,
  AK24_ATOM_U8,
  AK24_ATOM_U16,
  AK24_ATOM_U32,
  AK24_ATOM_U64,
  AK24_ATOM_I8,
  AK24_ATOM_I16,
  AK24_ATOM_I32,
  AK24_ATOM_I64,
  AK24_ATOM_F32,
  AK24_ATOM_F64,
  AK24_ATOM_CHAR,    // 1 byte signed for c parity
  AK24_ATOM_CLUSTER, // a group of atoms
} ak_atom_type_e;

typedef enum {
  AK24_ANY,
  AK24_NUMERIC,
  AK24_UNSIGNED,
  AK24_SIGNED,
  AK24_REAL,
  AK24_SYMBOLIC,
} ak_atom_category_e;

typedef struct {
  ak_atom_type_e type;
  ak_atom_category_e categories[]; // array of categories
} ak_atom_meta_t;

typedef struct ak_atom_neighbor_t {
  _Atomic(struct ak_atom_t *) atom;
  _Atomic(struct ak_atom_neighbor_t *) next;
} ak_atom_neighbor_t;

typedef struct ak_atom_t {
  _Atomic(ak_atom_type_e) type;
  _Atomic size_t dimensionality;
  _Atomic(ak_atom_neighbor_t *) neighbors;
  union {
    _Atomic uint8_t u8;
    _Atomic uint16_t u16;
    _Atomic uint32_t u32;
    _Atomic uint64_t u64;
    _Atomic int8_t i8;
    _Atomic int16_t i16;
    _Atomic int32_t i32;
    _Atomic int64_t i64;
    _Atomic float f32;
    _Atomic double f64;
    _Atomic char c;
    _Atomic uint8_t byte;
    void *cluster;
  } value;
} ak_atom_t;

typedef struct ak_atom_cluster_t {
  ak_atom_t **atoms;
  size_t count;
  size_t capacity;
} ak_atom_cluster_t;

typedef union {
  uint8_t u8;
  uint16_t u16;
  uint32_t u32;
  uint64_t u64;
  int8_t i8;
  int16_t i16;
  int32_t i32;
  int64_t i64;
  float f32;
  double f64;
  char c;
  uint8_t byte;
  void *cluster;
} ak_atom_value_u;

ak_atom_t *ak_atom_new(ak_atom_type_e type, ak_atom_value_u value,
                       size_t dimensionality);

void ak_atom_free(ak_atom_t *atom);

ak_atom_value_u ak_atom_get_value(ak_atom_t *atom);

void ak_atom_set_value(ak_atom_t *atom, ak_atom_value_u value);

ak_atom_type_e ak_atom_get_type(ak_atom_t *atom);

size_t ak_atom_get_dimensionality(ak_atom_t *atom);

int ak_atom_bond(ak_atom_t *atom1, ak_atom_t *atom2);

int ak_atom_unbond(ak_atom_t *atom1, ak_atom_t *atom2);

ak_atom_cluster_t *ak_atom_neighbors(ak_atom_t *atom, size_t distance);

ak_atom_cluster_t *ak_atom_cluster_new(size_t initial_capacity);

void ak_atom_cluster_free(ak_atom_cluster_t *cluster);

int ak_atom_cluster_add(ak_atom_cluster_t *cluster, ak_atom_t *atom);

ak_atom_t *ak_atom_cluster_get(ak_atom_cluster_t *cluster, size_t index);

size_t ak_atom_cluster_count(ak_atom_cluster_t *cluster);

#endif

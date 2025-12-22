#ifndef SDL_EXAMPLE_ENTITY_H
#define SDL_EXAMPLE_ENTITY_H

#include <stddef.h>

typedef enum {
  ENTITY_DIRECTION_IN = 0,
  ENTITY_DIRECTION_NORTH,
  ENTITY_DIRECTION_NORTH_EAST,
  ENTITY_DIRECTION_EAST,
  ENTITY_DIRECTION_SOUTH_EAST,
  ENTITY_DIRECTION_SOUTH,
  ENTITY_DIRECTION_SOUTH_WEST,
  ENTITY_DIRECTION_WEST,
  ENTITY_DIRECTION_NORTH_WEST,
  ENTITY_DIRECTION_OUT,
} entity_direction_e;

// fed declare entity
typedef struct entity_t entity_t;

/*
  We will make a mapping of entitues 1 for each cell of a given HxW grid
  regardless of purpose. we will provide an api to get the data assoicted in an
  atomic manner, stored as a void* so the user can define what the entity IS or
  represents, the entity is just a means to represent a spacial mapping of data.
  the IN and OUT are use-case specific to help us represent a 3-dimensional
  entity grid, traversed via the "middle" entity plane. Mentally picture it like
  a towel. The mains structs we work with is the fabric that holds the towel
  together, and the UP and DOWN are the threads on the surface of the towel.

  Dont over think it, its just a grid of spacial dimensions like a sstandard
  cartesian. just know there is alo an up and down dimension in the mapping
  accessable from a given discrete x/y FIRST, then they can go up/down.
*/

typedef struct {
  size_t unique_id;
  entity_t
      *spatial_mapping[10]; // one for each direction. IN means "self" and OUT
                            // means "visual expression/ direction 'external'"
} entity_t;

// entity_get_entity_at(x, y) -> entity_t*
// the user can then access the *data themselves and we dont care
// they can then also navigate the entity structure as they see fit. we
// literally JUST help construct the grid and provide access to retrieve it from
// the main entity context

typedef struct {
  size_t width;
  size_t height;
  entity_t **entities; // array of entity_t pointers
} entity_grid_t;       // this is essentially the middle fabric of the towel

// We will use AK24 ALLOC and AK24_FREE, NOT malloc/free
// we just set data null on init

// offer functions such as :
/*





      iterate(x, y, ak24_lambda_t) ->        walks entity structure one by one
   from x to y calculated by bresenham line algo and executes a lambda for every
   entity found on "the main path"

      immediate_neigbors(x, y, lambda) ->    NORTH, EAST, SOUTH, WEST only
   immediate neighbors, no diagonals

      for_self(x, y, lambda) -> just the entity at x,y, UP, SELF, DOWN


      // distance 1 would be all the neighbores, NORTH, EAST, SOUTH, WEST,
   NORTH_EAST, SOUTH_EAST, SOUTH_WEST, NORTH_WEST
      // distance 2 would be the ring outside of that, etc
      // we will have SCALAR_RING, and SCALAR_FILL to determine if the
   counter/clockwise will iterate the RING scalar OR, if
      // it will iterate ALL ENTITUES within the scalar range. this way we can
   also do entity fills for deteciton, AOE, etc





      scalar_clockwise_neighbors(x, y, distance, is_filled, lambda) -> all
   neighbors at a given distance, e.g. distance 2 would be the ring of entities
   2 cells away from the center entity at (x,y)


      scalar_counter_clockwise_neighbors(x, y, distance, is_filled, lambda) ->
   all neighbors at a given distance, e.g. distance 2 would be the ring of
   entities 2 cells away from the center entity at (x,y)



      The lambda functions must all accept an entity_t. The entity sync for
   threads will be taken care of external to this so dont care about anything
   regarding that here




    Offering these iteration lambdas will help us significantly for updating
   screen regions and doing the computations cell-wise


      im thinking we can do something wherw when clicked, we get the 1 distance
   scalar neighbors, and then for each of them, inform them of the click and
   what distance it came from. a sort of "bump" whihc can then cascade into
   other events, like filling its neightbors opposite the direction of the click



*/
#endif

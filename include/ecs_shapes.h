
#ifndef ECS_SHAPES_H
#define ECS_SHAPES_H

#include <reflecs/reflecs.h>

typedef struct Context {
    EcsEntity
      * location,
      * speed,
      * size,
      * rotation,
      * ddswriter;
} Context;

#endif

#include <include/ecs_shapes.h>
#include <include/components.h>
#include <reflecs/reflecs.h>
#include "src/gen/ShapeType.h"
#include "src/gen/ShapeTypeSupport.h"

#define MAX_X (240)
#define MAX_Y (270)
#define MAX_SPEED (5)

void Move(EcsEntity *system, EcsEntity *e, void *data[]) {
    Location *location = data[0];
    Speed *speed = data[1];
    Size *size = data[2];

    uint32_t min = size->size / 2;

    uint32_t max_x = (MAX_X - min);
    if (location->x > max_x) {
        speed->x *= -1;
        location->x = max_x;
    } else
    if (location->x < min) {
        speed->x *= -1;
        location->x = min;
    }

    uint32_t max_y = (MAX_Y - min);
    if (location->y > max_y) {
        speed->y *= -1;
        location->y = max_y;
    } else
    if (location->y < min) {
        speed->y *= -1;
        location->y = min;
    }

    location->x += speed->x;
    location->y += speed->y;
}

void Bounce(EcsEntity *system, EcsEntity *e, void *data[]) {
    Speed *speed = data[0];

    speed->y += 0.2;

    if (speed->y > MAX_SPEED * 1.5) {
        speed->y = MAX_SPEED * 1.5;
    }
    if (speed->y < -MAX_SPEED * 1.5) {
        speed->y = -MAX_SPEED * 1.5;
    }
}

void Rotate(EcsEntity *system, EcsEntity *e, void *data[]) {
    Rotation *rotation = data[0];
    rotation->angle += 5;
}

void InitShape(EcsEntity *system, EcsEntity *e, void *data[]) {
    Location *location = data[0];
    Speed *speed = data[1];
    Size *size = data[2];
    speed->x = rand() % MAX_SPEED;
    speed->y = speed->x;
    size->size = 35;
    location->x = (rand() % (MAX_X - size->size)) + size->size / 2;
    location->y = (rand() % (MAX_Y - size->size)) + size->size / 2;
}

void SyncShape(EcsEntity *system, EcsEntity *e, void *data[]) {
    DDS_ReturnCode_t retcode;
    Location *location = data[0];
    Size *size = data[1];
    Rotation *rotation = data[2];

    EcsWorld *world = ecs_get_world(system);
    Context *ctx = ecs_world_get_context(world);
    DdsWriter *w = ecs_get(system, ctx->ddswriter);
    ShapeTypeExtendedDataWriter *dw = ShapeTypeExtendedDataWriter_narrow(w->dw);

    ShapeTypeExtended instance;
    instance.parent.color = (char*)ecs_get_id(e);
    instance.parent.x = location->x;
    instance.parent.y = location->y;
    instance.parent.shapesize = size->size;
    instance.angle = rotation->angle;
    instance.fillKind = 0;

    retcode = ShapeTypeExtendedDataWriter_write(dw, &instance, &DDS_HANDLE_NIL);
}

#include <include/bake_config.h>
#include <unistd.h>

#define FPS (100.0)
#define SHAPE_COUNT (8)
#define ANGULAR_SPEED (80)
#define MAX_X (240)
#define MAX_Y (270)
#define MAX_SPEED (130)
#define SIZE (35)

/* -- Custom components -- */

typedef bool Collision[2];

/* -- System implementations -- */

/* -- Utility to detect & handle collisions with window borders -- */
float Limit(float value, float min, float max, bool *collision) {
    float result = value;
    if (value > max) {
        if (collision) *collision = -1;
        result = max - (value - max);
    } else
    if (value < min) {
        if (collision) *collision = 1;
        result = min + (min - value);
    }
    return result;
}

/* -- Gravity system -- */
void Gravity(EcsRows *rows) {
    void *row;
    for (row = rows->first; row < rows->last; row = ecs_next(rows, row)) {
        EcsVelocity2D *v = ecs_data(rows, row, 0);
        v->y = Limit(v->y + 70 * rows->delta_time, -MAX_SPEED, MAX_SPEED, NULL);
    }
}

/* -- Drag system -- */
void Drag(EcsRows *rows) {
    void *row;
    for (row = rows->first; row < rows->last; row = ecs_next(rows, row)) {
        EcsVelocity2D *v = ecs_data(rows, row, 0);
        v->x *= 1.0 - (0.1 * rows->delta_time);
        v->y *= 1.0 - (0.1 * rows->delta_time);
    }
}

/* -- Bounce system -- */
void Bounce(EcsRows *rows) {
    void *row;
    for (row = rows->first; row < rows->last; row = ecs_next(rows, row)) {
        EcsPosition2D *p = ecs_data(rows, row, 0);
        EcsVelocity2D *v = ecs_data(rows, row, 1);
        Size *size = ecs_data(rows, row, 2);
        bool *collision = ecs_data(rows, row, 3);
        float min = *size / 2.0;

        bool x_collision = false, y_collision = false;
        p->x = Limit(p->x, min, MAX_X - min, &x_collision);
        p->y = Limit(p->y, min, MAX_Y - min, &y_collision);
        if (x_collision * v->x < 0) {
            v->x *= -1;
            collision[0] = true;
        } else {
            collision[0] = false;
        }
        if (y_collision * v->y < 0) {
            v->y *= -1;
            collision[1] = true;
        } else {
            collision[1] = false;
        }
    }
}

/* -- Explode system -- */
void Explode(EcsRows *rows) {
    void *row;
    EcsWorld *world = rows->world;
    EcsEntity Size_h = rows->components[1];
    EcsEntity EcsVelocity2D_h = rows->components[2];

    for (row = rows->first; row < rows->last; row = ecs_next(rows, row)) {
        EcsEntity entity = ecs_entity(rows, row, ECS_ROW_ENTITY);
        bool *c = ecs_data(rows, row, 0);
        Size *s = ecs_data(rows, row, 1);
        EcsVelocity2D *v = ecs_data(rows, row, 2);

        if (c[0] || c[1]) {
            Size new_size = *s *= 0.8;

            if (new_size >= (SIZE / 4.0)) {
                EcsEntity frag1 = ecs_clone(world, entity, true);
                EcsEntity frag2 = ecs_clone(world, entity, true);

                ecs_set(world, frag1, Size, {new_size});
                ecs_set(world, frag2, Size, {new_size});

                if (c[0]) {
                    ecs_set(world, frag2, EcsVelocity2D, {v->x, v->y * -1});
                } else {
                    ecs_set(world, frag2, EcsVelocity2D, {v->x * -1, v->y});
                }
            }

            ecs_delete(world, entity);
        }
    }
}

/* Create 100 entities per second */
void Create(EcsRows *rows) {
    EcsEntity Shape_h = ecs_component(rows, 0);
    ecs_new_w_count(rows->world, Shape_h, 100.0 * rows->delta_time, NULL);
}

/* -- System that initializes shapes -- */
void InitShape(EcsRows *rows) {
    void *row;

    for (row = rows->first; row < rows->last; row = ecs_next(rows, row)) {
        EcsPosition2D *p = ecs_data(rows, row, 0);
        EcsVelocity2D *v = ecs_data(rows, row, 1);
        EcsRotation2D *r = ecs_data(rows, row, 2);
        EcsAngularSpeed *as = ecs_data(rows, row, 3);
        Size *size = ecs_data(rows, row, 4);
        bool *collision = ecs_data(rows, row, 4);
        *size = SIZE;
        v->x = rand() % (MAX_SPEED / 2) + 1;
        v->y = v->x;
        p->x = (rand() % (int)(MAX_X - *size)) + *size / 2;
        p->y = (rand() % (int)(MAX_Y - *size)) + *size / 2;
        r->angle = rand() % 360;
        as->value = ANGULAR_SPEED;
        collision[0] = 0;
        collision[1] = 0;
    }
}

/* -- Main -- */

int main(int argc, char *argv[]) {
    EcsWorld *world = ecs_init();

    /* -- Import ECS modules -- */
    ECS_IMPORT(world, EcsComponentsTransform, ECS_2D);
    ECS_IMPORT(world, EcsComponentsPhysics, ECS_2D);
    ECS_IMPORT(world, EcsSystemsPhysics, ECS_2D);
    ECS_IMPORT(world, EcsSystemsCivetweb, 0);
    ECS_IMPORT(world, EcsSystemsAdmin, 0);
    ECS_IMPORT(world, EcsSystemsShapes, 0);

    /* -- Init components and component families -- */
    ECS_COMPONENT(world, Collision);
    ECS_FAMILY(world, Shape, EcsPosition2D, EcsVelocity2D, EcsRotation2D, EcsAngularSpeed, Size, Collision);

    /* -- Init systems -- */
    ECS_SYSTEM(world, InitShape, EcsOnAdd,   EcsPosition2D, EcsVelocity2D, EcsRotation2D, EcsAngularSpeed, Size, Collision);

    /* -- Frame systems -- */
    ECS_SYSTEM(world, Bounce,    EcsOnFrame, EcsPosition2D, EcsVelocity2D, Size, Collision);
    ECS_SYSTEM(world, Gravity,   EcsOnFrame, EcsVelocity2D);
    ECS_SYSTEM(world, Drag,      EcsOnFrame, EcsVelocity2D);
    ECS_SYSTEM(world, Explode,   EcsOnFrame, Collision, Size, EcsVelocity2D);
    ECS_SYSTEM(world, Create,    EcsOnFrame, ID.Shape);

    /* -- Create shape entities -- */
    ecs_new_w_count(world, Shape_h, SHAPE_COUNT, NULL);

    /* -- Enable/disable systems -- */
    ecs_enable(world, EcsRotate_h, true);
    ecs_enable(world, EcsMove_h, true);
    ecs_enable(world, Gravity_h, true);
    ecs_enable(world, Drag_h, false);
    ecs_enable(world, Explode_h, false);
    ecs_enable(world, Create_h, false);
    ecs_enable(world, EcsDdsSync_h, true);

    /* -- Start admin -- */
    ecs_set(world, 0, EcsAdmin, {.port = 9090});

    /* -- Enable profiling on startup -- */
    ecs_measure_frame_time(world, true);
    ecs_measure_system_time(world, true);

    /* Limit the frequency at which the main loop runs */
    ecs_set_target_fps(world, FPS);

    /* -- Main loop -- */
    while (true) {
        ecs_progress(world, 0);
    }

    return ecs_fini(world);
}

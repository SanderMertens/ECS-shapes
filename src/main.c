#include <reflecs/reflecs.h>
#include <reflecs/util/stats.h>
#include <reflecs/components/transform/transform.h>
#include <reflecs/components/physics/physics.h>
#include <reflecs/systems/physics/physics.h>
#include <reflecs/systems/civetweb/civetweb.h>
#include <reflecs/systems/admin/admin.h>
#include <reflecs/util/time.h>
#include "src/gen/ShapeType.h"
#include "src/gen/ShapeTypeSupport.h"
#include <unistd.h>

#define DOMAIN_ID (0)
#define TOPIC_NAME "Square"

#define FPS (100.0)
#define SHAPE_COUNT (8)
#define ANGULAR_SPEED (80)
#define MAX_X (240)
#define MAX_Y (270)
#define MAX_SPEED (130)
#define SIZE (35)

/* -- Custom components -- */

typedef float Size;

typedef bool Collision[2];

typedef struct DdsEntities {
    DDS_DomainParticipant *dp;
    DDS_Publisher *pub;
    DDS_Topic *topic;
    DDS_DataWriter *dw;
} DdsEntities;

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
        EcsVelocity2D *v = ecs_column(rows, row, 0);
        v->y = Limit(v->y + 70 * rows->delta_time, -MAX_SPEED, MAX_SPEED, NULL);
    }
}

/* -- Drag system -- */
void Drag(EcsRows *rows) {
    void *row;
    for (row = rows->first; row < rows->last; row = ecs_next(rows, row)) {
        EcsVelocity2D *v = ecs_column(rows, row, 0);
        v->x *= 1.0 - (0.1 * rows->delta_time);
        v->y *= 1.0 - (0.1 * rows->delta_time);
    }
}

/* -- Bounce system -- */
void Bounce(EcsRows *rows) {
    void *row;
    for (row = rows->first; row < rows->last; row = ecs_next(rows, row)) {
        EcsPosition2D *p = ecs_column(rows, row, 0);
        EcsVelocity2D *v = ecs_column(rows, row, 1);
        Size *size = ecs_column(rows, row, 2);
        bool *collision = ecs_column(rows, row, 3);
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
    EcsHandle Size_h = rows->components[1];
    EcsHandle EcsVelocity2D_h = rows->components[2];

    for (row = rows->first; row < rows->last; row = ecs_next(rows, row)) {
        EcsHandle entity = ecs_entity(row);
        bool *c = ecs_column(rows, row, 0);
        Size *s = ecs_column(rows, row, 1);
        EcsVelocity2D *v = ecs_column(rows, row, 2);

        if (c[0] || c[1]) {
            Size new_size = *s *= 0.8;

            if (new_size >= (SIZE / 4.0)) {
                EcsHandle frag1 = ecs_clone(world, entity, true);
                EcsHandle frag2 = ecs_clone(world, entity, true);

                ecs_set(world, frag1, Size, new_size);
                ecs_set(world, frag2, Size, new_size);

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
    EcsHandle Shape_h = ecs_handle(rows, 0);
    ecs_new_w_count(rows->world, Shape_h, 100.0 * rows->delta_time, NULL);
}

/* -- System that initializes shapes -- */
void InitShape(EcsRows *rows) {
    void *row;
    for (row = rows->first; row < rows->last; row = ecs_next(rows, row)) {
        EcsPosition2D *p = ecs_column(rows, row, 0);
        EcsVelocity2D *v = ecs_column(rows, row, 1);
        EcsRotation2D *r = ecs_column(rows, row, 2);
        EcsAngularSpeed *as = ecs_column(rows, row, 3);
        Size *size = ecs_column(rows, row, 4);
        bool *collision = ecs_column(rows, row, 4);
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

/* -- System that synchronizes entities (shapes) to DDS -- */
void DdsSync(EcsRows *rows) {
    DdsEntities *w = ecs_column(rows, NULL, 0);
    ShapeTypeExtendedDataWriter *dw = ShapeTypeExtendedDataWriter_narrow(w->dw);
    char *colors[] = {"PURPLE", "BLUE", "RED", "GREEN", "YELLOW", "CYAN", "MAGENTA", "ORANGE"};

    void *row;
    for (row = rows->first; row < rows->last; row = ecs_next(rows, row)) {
        EcsHandle entity = ecs_entity(row);
        EcsPosition2D *p = ecs_column(rows, row, 1);
        EcsRotation2D *r = ecs_column(rows, row, 2);
        Size *size = ecs_column(rows, row, 3);

        ShapeTypeExtended instance;
        instance.parent.color = colors[entity % (sizeof(colors) / sizeof(char*))];
        instance.parent.x = p->x;
        instance.parent.y = p->y;
        instance.parent.shapesize = *size;
        instance.angle = r->angle;
        instance.fillKind = 0;

        ShapeTypeExtendedDataWriter_write(dw, &instance, &DDS_HANDLE_NIL);
    }
}

/* -- DDS entity code -- */

void DdsDeinit(EcsRows *rows) {
    void *row;
    for (row = rows->first; row < rows->last; row = ecs_next(rows, row)) {
        DdsEntities *writer = ecs_column(rows, row, 0);
        if (writer->dp)
            DDS_DomainParticipant_delete_contained_entities(writer->dp);
    }
}

void DdsInit(EcsRows *rows) {
    void *row;
    for (row = rows->first; row < rows->last; row = ecs_next(rows, row)) {
        DdsEntities *writer = ecs_column(rows, row, 0);
        writer->dp = DDS_DomainParticipantFactory_create_participant(
            DDS_TheParticipantFactory, DOMAIN_ID, &DDS_PARTICIPANT_QOS_DEFAULT,
            NULL, DDS_STATUS_MASK_NONE);
        if (!writer->dp) break;

        writer->pub = DDS_DomainParticipant_create_publisher(
            writer->dp, &DDS_PUBLISHER_QOS_DEFAULT, NULL, DDS_STATUS_MASK_NONE);
        if (!writer->pub) break;

        const char *type_name = ShapeTypeExtendedTypeSupport_get_type_name();
        DDS_ReturnCode_t retcode = ShapeTypeExtendedTypeSupport_register_type(
            writer->dp, type_name);
        if (retcode != DDS_RETCODE_OK) break;

        writer->topic = DDS_DomainParticipant_create_topic(
            writer->dp, TOPIC_NAME, type_name, &DDS_TOPIC_QOS_DEFAULT, NULL,
            DDS_STATUS_MASK_NONE);
        if (writer->topic == NULL) break;

        writer->dw = DDS_Publisher_create_datawriter(
            writer->pub, writer->topic, &DDS_DATAWRITER_QOS_DEFAULT, NULL,
            DDS_STATUS_MASK_NONE);
        if (writer->dw == NULL) break;
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

    /* -- Init components and component families -- */
    ECS_COMPONENT(world, Size);
    ECS_COMPONENT(world, Collision);
    ECS_COMPONENT(world, DdsEntities);
    ECS_FAMILY(world, Shape, EcsPosition2D, EcsVelocity2D, EcsRotation2D, EcsAngularSpeed, Size, Collision);

    /* -- Init systems -- */
    ECS_SYSTEM(world, InitShape, EcsOnAdd,   EcsPosition2D, EcsVelocity2D, EcsRotation2D, EcsAngularSpeed, Size, Collision);
    ECS_SYSTEM(world, DdsInit,   EcsOnAdd,   DdsEntities);
    ECS_SYSTEM(world, DdsDeinit, EcsOnRemove, DdsEntities);

    /* -- Frame systems -- */
    ECS_SYSTEM(world, DdsSync,   EcsOnFrame, SYSTEM.DdsEntities, EcsPosition2D, EcsRotation2D, Size);
    ECS_SYSTEM(world, Bounce,    EcsOnFrame, EcsPosition2D, EcsVelocity2D, Size, Collision);
    ECS_SYSTEM(world, Gravity,   EcsOnFrame, EcsVelocity2D);
    ECS_SYSTEM(world, Drag,      EcsOnFrame, EcsVelocity2D);
    ECS_SYSTEM(world, Explode,   EcsOnFrame, Collision, Size, EcsVelocity2D);
    ECS_SYSTEM(world, Create,    EcsOnFrame, HANDLE.Shape);

    /* -- Create shape entities -- */
    ecs_new_w_count(world, Shape_h, SHAPE_COUNT, NULL);

    /* -- Enable/disable systems -- */
    ecs_enable(world, EcsRotate_h, true);
    ecs_enable(world, EcsMove_h, true);
    ecs_enable(world, Gravity_h, true);
    ecs_enable(world, Drag_h, false);
    ecs_enable(world, Explode_h, false);
    ecs_enable(world, Create_h, false);
    ecs_enable(world, DdsSync_h, true);

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

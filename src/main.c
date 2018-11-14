#include <reflecs/reflecs.h>
#include <reflecs/components/transform/transform.h>
#include <reflecs/components/physics/physics.h>
#include <reflecs/systems/physics/physics.h>
#include "src/gen/ShapeType.h"
#include "src/gen/ShapeTypeSupport.h"
#include <unistd.h>

#define DOMAIN_ID (0)
#define TOPIC_NAME "Square"
#define SHAPE_COUNT (8)

#define MAX_X (240)
#define MAX_Y (270)
#define MAX_SPEED (5)

/* -- Custom components -- */

typedef int32_t Size;

typedef struct DdsEntities {
    DDS_DomainParticipant *dp;
    DDS_Publisher *pub;
    DDS_Topic *topic;
    DDS_DataWriter *dw;
} DdsEntities;

/* -- System implementations -- */

float Limit(float value, float min, float max, float *v) {
    float result = value;
    if (value > max) {
        if (v) *v *= -1;
        result = max;
    } else
    if (value < min) {
        if (v) *v *= -1;
        result = min;
    }
    return result;
}

void Bounce(EcsRows *rows) {
    void *row;
    for (row = rows->first; row < rows->last; row = ecs_next(rows, row)) {
        EcsPosition2D *p = ecs_column(rows, row, 0);
        EcsVelocity2D *v = ecs_column(rows, row, 1);
        Size *size = ecs_column(rows, row, 2);
        uint32_t min = *size / 2;
        p->x = Limit(p->x, min, MAX_X - min, &v->x);
        p->y = Limit(p->y, min, MAX_Y - min, &v->y);
    }
}

void Gravity(EcsRows *rows) {
    void *row;
    for (row = rows->first; row < rows->last; row = ecs_next(rows, row)) {
        EcsVelocity2D *v = ecs_column(rows, row, 0);
        v->y = Limit(v->y + 0.2, -MAX_SPEED * 1.5, MAX_SPEED * 1.5, NULL);
    }
}

void InitShape(EcsRows *rows) {
    void *row;
    for (row = rows->first; row < rows->last; row = ecs_next(rows, row)) {
        EcsPosition2D *p = ecs_column(rows, row, 0);
        EcsVelocity2D *v = ecs_column(rows, row, 1);
        EcsRotation2D *r = ecs_column(rows, row, 2);
        EcsAngularSpeed *as = ecs_column(rows, row, 3);
        Size *size = ecs_column(rows, row, 4);
        *size = 35;
        v->x = rand() % MAX_SPEED + 1;
        v->y = v->x;
        p->x = (rand() % (MAX_X - *size)) + *size / 2;
        p->y = (rand() % (MAX_Y - *size)) + *size / 2;
        r->angle = 0;
        as->value = 5;
    }
}

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

void DeinitDDS(EcsRows *rows) {
    void *row;
    for (row = rows->first; row < rows->last; row = ecs_next(rows, row)) {
        DdsEntities *writer = ecs_column(rows, row, 0);
        if (writer->dp)
            DDS_DomainParticipant_delete_contained_entities(writer->dp);
    }
}

void InitDDS(EcsRows *rows) {
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

    /* -- Import ECS module definitions -- */
    ECS_IMPORT(world, EcsComponentsTransform, ECS_2D);
    ECS_IMPORT(world, EcsComponentsPhysics, ECS_2D);
    ECS_IMPORT(world, EcsSystemsPhysics, ECS_2D);

    /* -- Init components and component families -- */
    ECS_COMPONENT(world, Size);
    ECS_COMPONENT(world, DdsEntities);
    ECS_FAMILY(world, Shape, EcsPosition2D, EcsVelocity2D, EcsRotation2D, EcsAngularSpeed, Size);

    /* -- Init systems -- */
    ECS_SYSTEM(world, InitShape, EcsOnInit,   EcsPosition2D, EcsVelocity2D, EcsRotation2D, EcsAngularSpeed, Size);
    ECS_SYSTEM(world, InitDDS,   EcsOnInit,   DdsEntities);
    ECS_SYSTEM(world, DeinitDDS, EcsOnDeinit, DdsEntities);
    ECS_SYSTEM(world, DdsSync,   EcsPeriodic, SYSTEM.DdsEntities, EcsPosition2D, EcsRotation2D, Size);
    ECS_SYSTEM(world, Bounce,    EcsPeriodic, EcsPosition2D, EcsVelocity2D, Size);
    ECS_SYSTEM(world, Gravity,   EcsPeriodic, EcsVelocity2D);

    /* -- Create shape entities -- */
    ecs_new_w_count(world, Shape_h, SHAPE_COUNT, NULL);

    /* -- Enable/disable systems -- */
    ecs_enable(world, EcsRotate_h, true);
    ecs_enable(world, EcsMove_h, true);
    ecs_enable(world, Gravity_h, true);

    /* -- Main loop -- */
    struct DDS_Duration_t send_period = {0, 100000000};
    while (true) {
        ecs_progress(world);
        NDDS_Utility_sleep(&send_period);
    }

    return ecs_fini(world);
}

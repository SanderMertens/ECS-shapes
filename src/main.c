#include <reflecs/reflecs.h>
#include <reflecs/components/transform/transform.h>
#include <reflecs/components/physics/physics.h>
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

float limit(float value, float min, float max, float *v) {
    float result = value;
    if (value > max) {
        *v *= -1;
        result = max;
    } else
    if (value < min) {
        *v *= -1;
        result = min;
    }
    return result;
}

void Move(
    EcsRows *rows)
{
    void *row;
    for (row = rows->first; row < rows->last; row = ecs_next(rows, row)) {
        EcsPosition2D *p = ecs_column(rows, row, 0);
        EcsVelocity2D *v = ecs_column(rows, row, 1);
        Size *size = ecs_column(rows, row, 2);

        uint32_t min = *size / 2;
        p->x = limit(p->x, min, MAX_X - min, &v->x);
        p->y = limit(p->y, min, MAX_Y - min, &v->y);
        p->x += v->x;
        p->y += v->y;
    }
}

void Bounce(
    EcsRows *rows)
{
    void *row;
    for (row = rows->first; row < rows->last; row = ecs_next(rows, row)) {
        EcsVelocity2D *v = ecs_column(rows, row, 0);

        v->y += 0.2;

        if (v->y > MAX_SPEED * 1.5) {
            v->y = MAX_SPEED * 1.5;
        }
        if (v->y < -MAX_SPEED * 1.5) {
            v->y = -MAX_SPEED * 1.5;
        }
    }
}

void Rotate(
    EcsRows *rows)
{
    void *row;
    for (row = rows->first; row < rows->last; row = ecs_next(rows, row)) {
        EcsRotation2D *r = ecs_column(rows, row, 0);
        r->angle += 5;
    }
}

void InitShape(
    EcsRows *rows)
{
    void *row;
    for (row = rows->first; row < rows->last; row = ecs_next(rows, row)) {
        EcsPosition2D *p = ecs_column(rows, row, 0);
        EcsVelocity2D *v = ecs_column(rows, row, 1);
        EcsRotation2D *r = ecs_column(rows, row, 3);
        Size *size = ecs_column(rows, row, 2);
        v->x = rand() % MAX_SPEED + 1;
        v->y = v->x;
        *size = 35;
        p->x = (rand() % (MAX_X - *size)) + *size / 2;
        p->y = (rand() % (MAX_Y - *size)) + *size / 2;
        r->angle = 0;
    }
}

void DdsSync(
    EcsRows *rows)
{
    DdsEntities *w = ecs_column(rows, NULL, 0);
    ShapeTypeExtendedDataWriter *dw = ShapeTypeExtendedDataWriter_narrow(w->dw);
    char *colors[] = {"PURPLE", "BLUE", "RED", "GREEN", "YELLOW", "CYAN", "MAGENTA", "ORANGE"};

    void *row;
    for (row = rows->first; row < rows->last; row = ecs_next(rows, row)) {
        DDS_ReturnCode_t retcode;
        EcsHandle entity = ecs_entity(row);
        EcsPosition2D *p = ecs_column(rows, row, 1);
        Size *size = ecs_column(rows, row, 2);
        EcsRotation2D *r = ecs_column(rows, row, 3);

        ShapeTypeExtended instance;
        instance.parent.color = colors[entity % (sizeof(colors) / sizeof(char*))];
        instance.parent.x = p->x;
        instance.parent.y = p->y;
        instance.parent.shapesize = *size;
        instance.angle = r->angle;
        instance.fillKind = 0;

        retcode = ShapeTypeExtendedDataWriter_write(
            dw, &instance, &DDS_HANDLE_NIL);
    }
}

/* -- DDS entity code -- */

static
void deinit_dds_entities(
    DdsEntities *writer)
{
    if (writer->dp)
        DDS_DomainParticipant_delete_contained_entities(writer->dp);
}

static
int init_dds_entities(
    DdsEntities *writer)
{
    DDS_ReturnCode_t retcode;

    writer->dp = DDS_DomainParticipantFactory_create_participant(
        DDS_TheParticipantFactory, DOMAIN_ID, &DDS_PARTICIPANT_QOS_DEFAULT,
        NULL, DDS_STATUS_MASK_NONE);
    if (!writer->dp) goto error;

    writer->pub = DDS_DomainParticipant_create_publisher(
        writer->dp, &DDS_PUBLISHER_QOS_DEFAULT, NULL, DDS_STATUS_MASK_NONE);
    if (!writer->pub) goto error;

    const char *type_name = ShapeTypeExtendedTypeSupport_get_type_name();
    retcode = ShapeTypeExtendedTypeSupport_register_type(
        writer->dp, type_name);
    if (retcode != DDS_RETCODE_OK) goto error;

    writer->topic = DDS_DomainParticipant_create_topic(
        writer->dp, TOPIC_NAME, type_name, &DDS_TOPIC_QOS_DEFAULT, NULL,
        DDS_STATUS_MASK_NONE);
    if (writer->topic == NULL) goto error;

    writer->dw = DDS_Publisher_create_datawriter(
        writer->pub, writer->topic, &DDS_DATAWRITER_QOS_DEFAULT, NULL,
        DDS_STATUS_MASK_NONE);
    if (writer->dw == NULL) goto error;

    return 0;
error:
    fprintf(stderr, "failed to create DDS entities\n");
    deinit_dds_entities(writer);
    return -1;
}

/* -- Main -- */

int main(
    int argc,
    char *argv[])
{
    EcsWorld *world = ecs_init();

    /* -- Import ECS module definitions -- */
    ECS_IMPORT(world, EcsComponentsTransform, ECS_2D);
    ECS_IMPORT(world, EcsComponentsPhysics, ECS_2D);

    /* -- Init components and component families -- */
    ECS_COMPONENT(world, Size);
    ECS_COMPONENT(world, DdsEntities);
    ECS_FAMILY(world, Shape, EcsPosition2D, EcsVelocity2D, Size, EcsRotation2D);

    /* -- Init systems -- */
    ECS_SYSTEM(world, Move,      EcsPeriodic, EcsPosition2D, EcsVelocity2D, Size);
    ECS_SYSTEM(world, Bounce,    EcsPeriodic, EcsVelocity2D);
    ECS_SYSTEM(world, Rotate,    EcsPeriodic, EcsRotation2D);
    ECS_SYSTEM(world, DdsSync,   EcsPeriodic, SYSTEM.DdsEntities, EcsPosition2D, Size, EcsRotation2D);
    ECS_SYSTEM(world, InitShape, EcsOnInit,   EcsPosition2D, EcsVelocity2D, Size, EcsRotation2D);

    /* -- Register & initialize DDS entities with DdsSync system -- */
    ecs_add(world, DdsSync_h, DdsEntities_h);
    ecs_commit(world, DdsSync_h);
    if (init_dds_entities( ecs_get_ptr(world, DdsSync_h, DdsEntities_h)) != 0) {
        return -1;
    }

    /* -- Create shape entities -- */
    EcsHandle handles[SHAPE_COUNT];
    ecs_new_w_count(world, Shape_h, SHAPE_COUNT, handles);

    /* -- To enable systems, change to 'true' -- */
    ecs_enable(world, Rotate_h, true);
    ecs_enable(world, Bounce_h, true);

    /* -- Main loop -- */
    struct DDS_Duration_t send_period = {0, 50000000};
    while (true) {
        ecs_progress(world);
        NDDS_Utility_sleep(&send_period);
    }

    /* -- Cleanup -- */
    deinit_dds_entities( ecs_get_ptr(world, DdsSync_h, DdsEntities_h));

    return ecs_fini(world);
}

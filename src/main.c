#include <reflecs/reflecs.h>
#include "src/gen/ShapeType.h"
#include "src/gen/ShapeTypeSupport.h"
#include <unistd.h>

#define DOMAIN_ID (0)
#define TOPIC_NAME "Square"

#define MAX_X (240)
#define MAX_Y (270)
#define MAX_SPEED (5)

typedef struct Context {
    EcsHandle ddswriter;
} Context;

/* -- Custom components -- */

typedef struct Location {
    int32_t x;
    int32_t y;
} Location;

typedef struct Speed {
    float x;
    float y;
} Speed;

typedef int32_t Size;

typedef struct Rotation {
    float angle;
} Rotation;

typedef struct DdsWriter {
    DDS_DomainParticipant *dp;
    DDS_Publisher *pub;
    DDS_Topic *topic;
    DDS_DataWriter *dw;
} DdsWriter;

/* -- System implementations -- */

void Move(
    EcsRows *rows)
{
    void *row;
    for (row = rows->first; row < rows->last; row = ecs_next(rows, row)) {
        Location *location = ecs_column(rows, row, 0);
        Speed *speed = ecs_column(rows, row, 1);
        Size *size = ecs_column(rows, row, 2);

        uint32_t min = *size / 2;

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
}

void Bounce(
    EcsRows *rows)
{
    void *row;
    for (row = rows->first; row < rows->last; row = ecs_next(rows, row)) {
        Speed *speed = ecs_column(rows, row, 0);

        speed->y += 0.2;

        if (speed->y > MAX_SPEED * 1.5) {
            speed->y = MAX_SPEED * 1.5;
        }
        if (speed->y < -MAX_SPEED * 1.5) {
            speed->y = -MAX_SPEED * 1.5;
        }
    }
}

void Rotate(
    EcsRows *rows)
{
    void *row;
    for (row = rows->first; row < rows->last; row = ecs_next(rows, row)) {
        Rotation *rotation = ecs_column(rows, row, 0);
        rotation->angle += 5;
    }
}

void InitShape(
    EcsRows *rows)
{
    void *row;
    for (row = rows->first; row < rows->last; row = ecs_next(rows, row)) {
        Location *location = ecs_column(rows, row, 0);
        Speed *speed = ecs_column(rows, row, 1);
        Size *size = ecs_column(rows, row, 2);
        speed->x = rand() % MAX_SPEED;
        speed->y = speed->x;
        *size = 35;
        location->x = (rand() % (MAX_X - *size)) + *size / 2;
        location->y = (rand() % (MAX_Y - *size)) + *size / 2;
    }
}

void SyncShape(
    EcsRows *rows)
{
    EcsWorld *world = rows->world;
    Context *ctx = ecs_get_context(world);
    EcsHandle system = rows->system;
    DdsWriter *w = ecs_get_ptr(world, system, ctx->ddswriter);
    ShapeTypeExtendedDataWriter *dw = ShapeTypeExtendedDataWriter_narrow(w->dw);

    void *row;
    for (row = rows->first; row < rows->last; row = ecs_next(rows, row)) {
        DDS_ReturnCode_t retcode;
        Location *location = ecs_column(rows, row, 0);
        Size *size = ecs_column(rows, row, 1);
        Rotation *rotation = ecs_column(rows, row, 2);
        EcsId *id = ecs_column(rows, row, 3);

        ShapeTypeExtended instance;
        instance.parent.color = (DDS_Char*)id->id;
        instance.parent.x = location->x;
        instance.parent.y = location->y;
        instance.parent.shapesize = *size;
        instance.angle = rotation->angle;
        instance.fillKind = 0;

        retcode = ShapeTypeExtendedDataWriter_write(dw, &instance, &DDS_HANDLE_NIL);
    }
}

/* -- DDS entity code -- */

static
void deinit_dds_entities(
    DdsWriter *writer)
{
    if (writer->dp) {
        DDS_DomainParticipant_delete_contained_entities(writer->dp);
    }
}

static
int init_dds_entities(
    DdsWriter *writer)
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

    /* -- Init components -- */
    ECS_COMPONENT(world, Location);
    ECS_COMPONENT(world, Speed);
    ECS_COMPONENT(world, Size);
    ECS_COMPONENT(world, Rotation);
    ECS_COMPONENT(world, DdsWriter);

    /* -- Init systems -- */
    ECS_SYSTEM(world, Move,       EcsPeriodic, Location, Speed, Size);
    ECS_SYSTEM(world, Bounce,     EcsPeriodic, Speed);
    ECS_SYSTEM(world, Rotate,     EcsPeriodic, Rotation);
    ECS_SYSTEM(world, SyncShape,  EcsPeriodic, Location, Size, Rotation, EcsId);
    ECS_SYSTEM(world, InitShape,  EcsOnInit,   Location, Speed, Size);

    /* Create family for shapes */
    ECS_FAMILY(world, Shape, Location, Speed, Size, Rotation);

    /* -- Share DdsWriter handle with other parts of the code -- */
    Context ctx = { .ddswriter = DdsWriter_h };
    ecs_set_context(world, &ctx);

    /* -- Add DDS entities component to SyncShape system.  -- */
    ecs_add(world, SyncShape_h, DdsWriter_h);
    ecs_commit(world, SyncShape_h);
    if (init_dds_entities( ecs_get_ptr(world, SyncShape_h, DdsWriter_h)) != 0) {
        return -1;
    }

    /* -- Create 3 shape entities -- */
    EcsHandle handles[3];
    ecs_new_w_count(world, Shape_h, 3, handles);
    ecs_set(world, handles[0], EcsId, {.id = "RED"});
    ecs_set(world, handles[1], EcsId, {.id = "GREEN"});
    ecs_set(world, handles[2], EcsId, {.id = "BLUE"});

    /* -- Initially disable Bounce system -- */
    ecs_enable(world, Bounce_h, false);

    /* -- Main loop -- */
    struct DDS_Duration_t send_period = {0, 50000000};
    while (true) {
        ecs_progress(world);
        NDDS_Utility_sleep(&send_period);
    }

    /* -- Cleanup -- */
    deinit_dds_entities( ecs_get_ptr(world, SyncShape_h, DdsWriter_h));

    return ecs_fini(world);
}

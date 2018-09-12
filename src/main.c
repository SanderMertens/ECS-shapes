#include <include/ecs_shapes.h>
#include <include/components.h>
#include <reflecs/reflecs.h>
#include <reflecs/ui/ui.h>
#include "src/gen/ShapeType.h"
#include "src/gen/ShapeTypeSupport.h"

#include <unistd.h>

#define DOMAIN_ID (0)
#define TOPIC_NAME "Square"

static void deinit_dds_entities(DdsWriter *writer) {
    if (writer->dp) {
        DDS_DomainParticipant_delete_contained_entities(writer->dp);
    }
}

static int init_dds_entities(DdsWriter *writer) {
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

static void create_shape(EcsWorld *world, const char *id, Context *ctx) {
    EcsEntity *e = ecs_new(world, id);
    ecs_stage(e, ctx->location);
    ecs_stage(e, ctx->speed);
    ecs_stage(e, ctx->size);
    ecs_stage(e, ctx->rotation);
    ecs_commit(e);
}

static void create_entities(EcsWorld *world, Context *ctx) {
    create_shape(world, "RED", ctx);
    create_shape(world, "BLUE", ctx);
    create_shape(world, "PURPLE", ctx);
    create_shape(world, "GREEN", ctx);
    create_shape(world, "YELLOW", ctx);
    create_shape(world, "CYAN", ctx);
    create_shape(world, "GREY", ctx);
}

int main(int argc, char *argv[]) {

    EcsWorld *world = ecs_world_new();

    /* -- Init components -- */
    ECS_COMPONENT(world, Location);
    ECS_COMPONENT(world, Speed);
    ECS_COMPONENT(world, Size);
    ECS_COMPONENT(world, Rotation);
    ECS_COMPONENT(world, DdsWriter);

    /* -- Init systems -- */
    ECS_SYSTEM(world, Move,       EcsPeriodic,  Location, Speed, Size);
    ECS_SYSTEM(world, Bounce,     EcsPeriodic,  Speed);
    ECS_SYSTEM(world, Rotate,     EcsPeriodic,  Rotation);
    ECS_SYSTEM(world, SyncShape,  EcsPeriodic,  Location, Size, Rotation);
    ECS_SYSTEM(world, InitShape,  EcsOnInit,    Location, Speed, Size);

    /* -- Share component handles with other parts of the code -- */
    Context ctx = {
        .location = Location_e,
        .speed = Speed_e,
        .size = Size_e,
        .rotation = Rotation_e,
        .ddswriter = DdsWriter_e
    };
    ecs_world_set_context(world, &ctx);

    /* -- Init DDS entities -- */
    DdsWriter *writer = ecs_add(SyncShape_e, DdsWriter_e);
    if (init_dds_entities(writer) != 0) {
        return -1;
    }

    /* -- Create shape entities -- */
    create_entities(world, &ctx);

    /* -- Create web UI -- */
    if (!ecs_ui_new(world)) {
        printf("failed to load UI\n");
        return -1;
    }

    ecs_system_enable(Bounce_e, false);

    /* -- Main loop -- */
    struct DDS_Duration_t send_period = {0, 50000000};
    while (true) {
        ecs_world_progress(world);
        NDDS_Utility_sleep(&send_period);
    }

    /* -- Cleanup -- */
    deinit_dds_entities(writer);

    return 0;
}

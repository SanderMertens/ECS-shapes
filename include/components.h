#ifndef ECS_SHAPES_COMPONENTS_H
#define ECS_SHAPES_COMPONENTS_H

#include <stdint.h>
#include "ndds/ndds_c.h"

typedef struct Location {
    int32_t x;
    int32_t y;
} Location;

typedef struct Speed {
    float x;
    float y;
} Speed;

typedef struct Size {
    int32_t size;
} Size;

typedef struct Rotation {
    float angle;
} Rotation;

typedef struct DdsWriter {
    DDS_DomainParticipant *dp;
    DDS_Publisher *pub;
    DDS_Topic *topic;
    DDS_DataWriter *dw;
} DdsWriter;

#endif

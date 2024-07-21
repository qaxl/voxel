#pragma once

#include <cglm/vec3.h>

#pragma push(pack, 1)
typedef struct Vertex {
    // XXXXXX YYYYYY ZZZZZZZZ RRRR GGGG BBBB
    //   6   +   6  +   8    + 4  + 4  + 4
    // = 32 bits
    uint32_t data;
} Vertex;

typedef struct Voxel {
    Vertex vertices[8];
} Voxel;

typedef struct Index {
    uint32_t indices[36];
} Index;

typedef struct Chunk {
    Voxel voxels[64][256][64];
    Index indices[64][256][64];
} Chunk;
#pragma pop(pack)

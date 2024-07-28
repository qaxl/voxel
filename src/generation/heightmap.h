#pragma once

#define HEIGHT_MAP_SIZE 64

typedef struct GicHeightMap {
    // XZ-coordinates
    float map[HEIGHT_MAP_SIZE][HEIGHT_MAP_SIZE];
    void* _renderer_internal;
} GicHeightMap;

GicHeightMap* gic_height_map_generate_using_perlin(float scale);
GicHeightMap* gic_height_map_generate_with_seed(int seed);

void gic_height_map_destroy(GicHeightMap* hm);

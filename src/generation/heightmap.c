#include "heightmap.h"
#include "perlin.h"

#include <stdio.h>
#include <stdlib.h>

GicHeightMap* gic_height_map_generate_using_perlin(float scale) {
    GicHeightMap* hm = malloc(sizeof(*hm));

    for (int x = 0; x < HEIGHT_MAP_SIZE; ++x) {
        for (int z = 0; z < HEIGHT_MAP_SIZE; ++z) {
            hm->map[x][z] = octave_perlin((float)x * scale, 0, (float)z * scale, 6, 0.8) * 100.f;
        }
    }

#ifndef NDEBUG
    printf("DEBUG HEIGHTMAP OUTPUT:\n\n");
    for (int x = 0; x < HEIGHT_MAP_SIZE; ++x) {
        for (int z = 0; z < HEIGHT_MAP_SIZE; ++z) {
            printf("%d:%d: %f ", x, z, hm->map[x][z]);
        }
    }
    printf("\n\n");
#endif

    hm->_renderer_internal = NULL;
    return hm;
}

void gic_height_map_destroy(GicHeightMap* hm) {
    if (hm->_renderer_internal != NULL) {
        free(hm->_renderer_internal);
    }

    free(hm);
}

#include "heightmap.h"
#include "perlin.h"

#include <FastNoiseLite.h>

#include <stdio.h>
#include <stdlib.h>

GicHeightMap* gic_height_map_generate_using_perlin(float scale) {
    GicHeightMap* hm = malloc(sizeof(*hm));

    fnl_state st = fnlCreateState();
    // st.seed = rand();
    st.noise_type = FNL_NOISE_OPENSIMPLEX2;
    st.fractal_type = FNL_FRACTAL_FBM;
    st.octaves = 8;
    st.frequency = 0.025f;
    printf("Generating terrain with seed of %d\n", st.seed);

    for (int x = 0; x < HEIGHT_MAP_SIZE; ++x) {
        for (int z = 0; z < HEIGHT_MAP_SIZE; ++z) {
            float noise = hm->map[x][z] = fnlGetNoise2D(&st, (float)x, (float)z) / 2.0 + 0.5;
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

GicHeightMap* gic_height_map_generate_with_seed(int seed) {
    GicHeightMap* hm = malloc(sizeof(*hm));

    fnl_state st = fnlCreateState();
    st.seed = seed;
    st.noise_type = FNL_NOISE_OPENSIMPLEX2;
    st.fractal_type = FNL_FRACTAL_FBM;
    st.octaves = 8;
    st.frequency = 0.025f;
    printf("Generating terrain with seed of %d\n", st.seed);

    for (int x = 0; x < HEIGHT_MAP_SIZE; ++x) {
        for (int z = 0; z < HEIGHT_MAP_SIZE; ++z) {
            float noise = hm->map[x][z] = fnlGetNoise2D(&st, (float)x, (float)z) / 2.0 + 0.5;
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

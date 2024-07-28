#include "renderer.h"
#include "shader.h"
#include "camera.h"
#include "vertex.h"
#include "util.h"
#include "platform/sdl_window.h"
#include "generation/heightmap.h"

#include <glad/gl.h>
#include <glad/wgl.h>

#include <cglm/cglm.h>
#include <cglm/cam.h>
#include <cglm/mat4.h>
#include <cglm/project.h>

#include <SDL3/SDL.h>

#include <Windows.h>

#include <minwindef.h>
#include <stdio.h>
#include <stdlib.h>

#include <nuklear.h>
#include <nuklear_sdl_gl3.h>

enum {
    SHADER_VOXEL,
    SHADER_HM,
    NUM_OF_SHADERS,
};

static const char** shader_file_lookup[] = {
    [SHADER_VOXEL] = (const char*[]){ "shaders/voxel.vert", "shaders/voxel.frag" },
};

#define EXPAND(x, y) x##y

#define LOAD_SHADER(S) do { \
    char* vert = gic_io_load_file_to_string(shader_file_lookup[S][0]); \
    char* frag = gic_io_load_file_to_string(shader_file_lookup[S][1]); \
    printf("%s\n%s\n", vert, frag); \
    renderer->shaders[S] = gic_load_shader_from_sources(vert, frag); \
    free(vert); \
    free(frag); \
} while(0)

struct GicGlRenderer {
    GicWindow* window;
    int width;
    int height;

    SDL_GLContext ctx;

    mat4 projection;
    mat4 view;

    Shader shaders[NUM_OF_SHADERS];
    // Chunk chunk;

    uint32_t vao;
    uint32_t vbo;
    uint32_t ibo;

    Camera camera;
    float delta;

    bool first_mouse_move;
    int mouse_last_x;
    int mouse_last_y;
    int face_count;

    int old_seed;
    int new_seed;

    struct nk_context* nk_ctx;
};

static void gic_gl__window_resize(void* user, int width, int height) {
    GicGlRenderer* renderer = user;

    glViewport(0, 0, width, height);

    renderer->width = width;
    renderer->height = height;
}

static void gic_gl__window_mouse_move(void* user, int x, int y) {
    GicGlRenderer* renderer = user;

    if (renderer->first_mouse_move) {
        renderer->first_mouse_move = false;
        renderer->mouse_last_x = x;
        renderer->mouse_last_y = y;
    }

    //int x_offset = x - renderer->mouse_last_x;
    //int y_offset = y - renderer->mouse_last_y;
    //renderer->mouse_last_x = x;
    //renderer->mouse_last_y = y;

    gic_camera_process_mouse_movement(&renderer->camera, x, y, true);
}

static void gic_gl__window_mouse_scroll(void* user, float scroll) {
    GicGlRenderer* renderer = user;

    gic_camera_process_mouse_scroll(&renderer->camera, scroll);
}

GicGlRenderer* gic_create_gl_renderer(GicWindow* window) {
    GicGlRenderer* renderer = malloc(sizeof(GicGlRenderer));
    renderer->window = window;
    renderer->delta = gic_get_ticks();
    renderer->first_mouse_move = true;

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    // SDL_WarpMouseInWindow(window->window, (float)window->width / 2.0f, (float)window->height / 2.0f);
    SDL_SetRelativeMouseMode(true);

    renderer->ctx = SDL_GL_CreateContext(window->window);
    SDL_GL_MakeCurrent(window->window, renderer->ctx);
    SDL_GL_SetSwapInterval(-1);

    if (gladLoadGL(SDL_GL_GetProcAddress) == 0) {
        fprintf(stderr, "[gic-renderer] [fatal] could not load opengl functions? are they available? is the driver available?\n");
        free(renderer);
        return NULL;
    }


    int width, height;
    // gic_get_window_size(window, &width, &height);
    SDL_GetWindowSize(window->window, &width, &height);

    // gic_set_user_ptr(window, renderer);
    // gic_on_window_resize(window, gic_gl__window_resize);
    // gic_on_window_mouse_move(window, gic_gl__window_mouse_move);
    // gic_on_window_mouse_scroll(window, gic_gl__window_mouse_scroll);

    // call once.
    gic_gl__window_resize(renderer, width, height);

    window->renderer_cbs.ptr = renderer;
    window->renderer_cbs.resize = gic_gl__window_resize;
    window->renderer_cbs.motion = gic_gl__window_mouse_move;
    window->renderer_cbs.wheel = gic_gl__window_mouse_scroll;

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    /*glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    glFrontFace(GL_CCW);
*/

    printf("s\n");
    LOAD_SHADER(SHADER_VOXEL);
    printf("s\n");

    // memset(&renderer->chunk, 0, sizeof(renderer->chunk));

    glCreateVertexArrays(1, &renderer->vao);
    // not a mistake, simply vbo and ibo are next to each other so they can be seen as an array.
    glGenBuffers(2, &renderer->vbo);

    glBindVertexArray(renderer->vao);
    glBindBuffer(GL_ARRAY_BUFFER, renderer->vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, renderer->ibo);

    // glBufferData(GL_ARRAY_BUFFER, sizeof(renderer->chunk.voxels), &renderer->chunk.voxels[0][0][0].vertices[0], GL_STATIC_DRAW);
    // glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Index) * (renderer->face_count / 36), renderer->chunk.indices, GL_STATIC_DRAW);

    glVertexAttribIPointer(0, 1, GL_UNSIGNED_INT, 0, NULL);
    glEnableVertexAttribArray(0);

    glUseProgram(renderer->shaders[SHADER_VOXEL]);

    renderer->camera = gic_create_camera_default();
    renderer->nk_ctx = nk_sdl_init(window->window);

    renderer->new_seed = renderer->old_seed = 1337;

    struct nk_font_atlas* k;
    nk_sdl_font_stash_begin(&k);
    nk_sdl_font_stash_end();

    return renderer;
}

void gic_destroy_gl_renderer(GicGlRenderer* renderer) {
    SDL_GL_DestroyContext(renderer->ctx);
    free(renderer);
}

void gic_gl_renderer_swap_buffers(GicGlRenderer* renderer) {
    if (!renderer->window->is_mouse_grabbed) {
        nk_input_end(renderer->nk_ctx);

        struct nk_context* ctx = renderer->nk_ctx;
        if (nk_begin(ctx, "Generator Settings", nk_rect(300, 300, 200, 200), NK_WINDOW_MOVABLE | NK_WINDOW_TITLE)) {
            nk_layout_row_dynamic(ctx, 30, 1);
            nk_slider_int(ctx, INT_MIN, &renderer->new_seed, INT_MAX, 1);

            // TODO: 
            static char seed_str[64] = { 0 };
            int seed_len = strlen(seed_str);
            SDL_itoa(renderer->new_seed, seed_str, 10);
            nk_layout_row_dynamic(ctx, 30, 2);
            nk_edit_string(ctx, NK_EDIT_FIELD | NK_TEXT_EDIT_MODE_INSERT, seed_str, &seed_len, 64, nk_filter_default);
            nk_edit_string_zero_terminated(ctx, NK_EDIT_FIELD, seed_str, 64, nk_filter_decimal);
            // printf("%s\n", seed_str);
            renderer->new_seed = atoi(seed_str);
        } nk_end(ctx);

        
        nk_sdl_render(NK_ANTI_ALIASING_ON, 512 * 1024, 128 * 1024);
    }

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    glFrontFace(GL_CCW);

    SDL_GL_SwapWindow(renderer->window->window);
}

#define PROCESS_KEY(key_code, direction) if (renderer->window->is_key_pressed[SDL_SCANCODE_##key_code]) { \
        gic_camera_process_keyboard(&renderer->camera, direction, delta); \
    } 

void gic_gl_clear(GicGlRenderer* renderer, float r, float g, float b) {
    glClearColor(r, g, b, 1.0f);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

    float time = gic_get_ticks();
    float delta = time - renderer->delta;
    renderer->delta = time;

    PROCESS_KEY(W, CAM_FORWARD)
    PROCESS_KEY(S, CAM_BACKWARDS)
    PROCESS_KEY(A, CAM_LEFT)
    PROCESS_KEY(D, CAM_RIGHT)
    
    glm_perspective(glm_rad(renderer->camera.zoom), (float)renderer->width / (float)renderer->height, 0.01f, 100.0f, renderer->projection);
    gic_camera_get_view_matrix(&renderer->camera, renderer->view);

    mat4 mvp;
    glm_mat4_mul(renderer->projection, renderer->view, mvp);
    glUniformMatrix4fv(1, 1, GL_FALSE, &mvp[0][0]);

    if (!renderer->window->is_mouse_grabbed) {
        nk_input_begin(renderer->nk_ctx);
    }

    // glDrawElements(GL_TRIANGLES, renderer->face_count, GL_UNSIGNED_INT, NULL);
}

typedef struct Gic__HeightMapData {
    uint32_t va;
    uint32_t vb;
    uint32_t ib;
} Gic__HeightMapData;

typedef struct Gic__HMVertex {
    uint32_t packed_data;
} Gic__HMVertex;

typedef struct Gic__HMIndex {
    uint32_t index;
} Gic__HMIndex;

#define PACK_DATA(x, y, z) v2d->vtx[k++].packed_data = (x & 0x3F) | ((int)y & 0xFF) << 6 | (z & 0x3F) << 14 | color
#define PACK_INDEX
#define MAKE_PACKED_COLOR(r, g, b) ((int)(r) & 0x3F) << 20 | ((int)(g) & 0x3F) << 24 | ((int)(b) & 0x3F) << 28


typedef struct Vec2D {
    Gic__HMVertex vtx[(HEIGHT_MAP_SIZE + 100) * (HEIGHT_MAP_SIZE + 100)];
} Vec2D;

typedef struct VecI2D {
    int i[HEIGHT_MAP_SIZE + 10][HEIGHT_MAP_SIZE + 10];
} VecI2D;

static void gic_gl__initialize_height_map_data(GicHeightMap* hm, Gic__HeightMapData* hmd) {
    glGenVertexArrays(1, &hmd->va);
    glBindVertexArray(hmd->va);
    
    glGenBuffers(2, &hmd->vb);
    glBindBuffer(GL_ARRAY_BUFFER, hmd->vb);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, hmd->ib);

    glVertexAttribIPointer(0, 1, GL_INT, 0, NULL);
    glEnableVertexAttribArray(0);

    Vec2D* v2d = malloc(sizeof(Vec2D));
    Gic__HMIndex* itx = malloc(sizeof(*itx) * (HEIGHT_MAP_SIZE + 1) * (HEIGHT_MAP_SIZE + 1) * 36);
    VecI2D* vi2d = malloc(sizeof(VecI2D));

    int i = 0;
    int j = 0;
    int k = 0;
    for (int x = 0; x < HEIGHT_MAP_SIZE; ++x) {
        for (int z = 0; z < HEIGHT_MAP_SIZE; ++z) {
            float height = hm->map[x][z] * 45.f;
            int color = 0;
            if (height > 40.f) {
                color = MAKE_PACKED_COLOR(0, 10, 0);
            } else if (height > 30.f) {
                color = MAKE_PACKED_COLOR(6, 6, 0);
            } else if (height > 15.f) {
                color = MAKE_PACKED_COLOR(4, 6, 0);
            } else {
                color = MAKE_PACKED_COLOR(0, 0, 12);
            }

            PACK_DATA(x, height, z);
            PACK_DATA(x + 1, height, z);
            PACK_DATA(x, height, z + 1);
            PACK_DATA(x + 1, height, z + 1);
            PACK_DATA(x, height + 1, z);
            PACK_DATA(x + 1, height + 1, z);
            PACK_DATA(x + 1, height + 1, z + 1);
            PACK_DATA(x, height + 1, z + 1);

            itx[i++].index = j + 0;  // Bottom-left
            itx[i++].index = j + 1;  // Bottom-right
            itx[i++].index = j + 2;  // Top-left
            itx[i++].index = j + 1;  // Bottom-right
            itx[i++].index = j + 3;  // Top-right
            itx[i++].index = j + 2;  // Top-left
            itx[i++].index = j + 4;  // Bottom-left
            itx[i++].index = j + 6;  // Top-left
            itx[i++].index = j + 5;  // Bottom-right
            itx[i++].index = j + 5;  // Bottom-right
            itx[i++].index = j + 6; // Top-left
            itx[i++].index = j + 7; // Top-right
            itx[i++].index = j + 0;  // Bottom-left
            itx[i++].index = j + 2;  // Top-left
            itx[i++].index = j + 6;  // Top-right
            itx[i++].index = j + 0;  // Bottom-left
            itx[i++].index = j + 6;  // Top-right
            itx[i++].index = j + 4;  // Bottom-right
            itx[i++].index = j + 1;  // Bottom-left
            itx[i++].index = j + 5;  // Bottom-right
            itx[i++].index = j + 3;  // Top-left
            itx[i++].index = j + 3;  // Top-left
            itx[i++].index = j + 5;  // Bottom-right
            itx[i++].index = j + 7;  // Top-right
            itx[i++].index = j + 2;  // Bottom-left
            itx[i++].index = j + 3;  // Bottom-right
            itx[i++].index = j + 6;  // Top-left
            itx[i++].index = j + 3;  // Bottom-right
            itx[i++].index = j + 7;  // Top-right
            itx[i++].index = j + 6;  // Top-left
            itx[i++].index = j + 0;  // Bottom-left
            itx[i++].index = j + 4;  // Top-left
            itx[i++].index = j + 1;  // Bottom-right
            itx[i++].index = j + 1;  // Bottom-right
            itx[i++].index = j + 4;  // Top-left
            itx[i++].index = j + 5;  // Top-right

            j += 8;
        }
    }

    glBufferData(GL_ARRAY_BUFFER, sizeof(v2d->vtx), v2d->vtx, GL_STATIC_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, i * sizeof(Gic__HMIndex), itx, GL_STATIC_DRAW);
    free(v2d);
    free(itx);

    glBindVertexArray(0);
}

void gic_gl_render_height_map(GicGlRenderer* renderer, GicHeightMap** hm) {
    if (renderer->old_seed != renderer->new_seed) {
        free(*hm);
        *hm = gic_height_map_generate_with_seed(renderer->new_seed);
        renderer->old_seed = renderer->new_seed;
    }

    if ((*hm)->_renderer_internal == NULL) {
        (*hm)->_renderer_internal = malloc(sizeof(Gic__HeightMapData));
        gic_gl__initialize_height_map_data(*hm, (*hm)->_renderer_internal);
    }

    Gic__HeightMapData* hmd = (*hm)->_renderer_internal;
    glBindVertexArray(hmd->va);
    glUseProgram(renderer->shaders[SHADER_VOXEL]);
    
    // glDrawArrays(GL_POINTS, 0, HEIGHT_MAP_SIZE * HEIGHT_MAP_SIZE);
    glDrawElements(GL_TRIANGLES, 36 * HEIGHT_MAP_SIZE * HEIGHT_MAP_SIZE, GL_UNSIGNED_INT, NULL);
}

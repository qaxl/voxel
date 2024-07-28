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
    Chunk chunk;

    uint32_t vao;
    uint32_t vbo;
    uint32_t ibo;

    Camera camera;
    float delta;

    bool first_mouse_move;
    int mouse_last_x;
    int mouse_last_y;
    int face_count;
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

    memset(&renderer->chunk, 0, sizeof(renderer->chunk));

    glCreateVertexArrays(1, &renderer->vao);
    // not a mistake, simply vbo and ibo are next to each other so they can be seen as an array.
    glGenBuffers(2, &renderer->vbo);

    glBindVertexArray(renderer->vao);
    glBindBuffer(GL_ARRAY_BUFFER, renderer->vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, renderer->ibo);

    glBufferData(GL_ARRAY_BUFFER, sizeof(renderer->chunk.voxels), &renderer->chunk.voxels[0][0][0].vertices[0], GL_STATIC_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Index) * (renderer->face_count / 36), renderer->chunk.indices, GL_STATIC_DRAW);

    glVertexAttribIPointer(0, 1, GL_UNSIGNED_INT, 0, NULL);
    glEnableVertexAttribArray(0);

    glUseProgram(renderer->shaders[SHADER_VOXEL]);

    renderer->camera = gic_create_camera_default();

    return renderer;
}

void gic_destroy_gl_renderer(GicGlRenderer* renderer) {
    SDL_GL_DestroyContext(renderer->ctx);
    free(renderer);
}

void gic_gl_renderer_swap_buffers(GicGlRenderer* renderer) {
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

#define PACK_DATA(x, z) vtx[i++].packed_data = x & 0x3F | ((int)hm->map[x][z] & 0xFF) << 6 | (z & 0x3F) << 14 | color
#define PACK_INDEX

static void gic_gl__initialize_height_map_data(GicHeightMap* hm, Gic__HeightMapData* hmd) {
    glGenVertexArrays(1, &hmd->va);
    glBindVertexArray(hmd->va);
    
    glGenBuffers(2, &hmd->vb);
    glBindBuffer(GL_ARRAY_BUFFER, hmd->vb);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, hmd->ib);

    glVertexAttribIPointer(0, 1, GL_INT, 0, NULL);
    glEnableVertexAttribArray(0);

    Gic__HMVertex* vtx = malloc(sizeof(*vtx) * HEIGHT_MAP_SIZE * HEIGHT_MAP_SIZE);
    Gic__HMIndex* itx = malloc(sizeof(*itx) * HEIGHT_MAP_SIZE * HEIGHT_MAP_SIZE * 6);

    int i = 0;
    for (int x = 0; x < HEIGHT_MAP_SIZE; ++x) {
        for (int z = 0; z < HEIGHT_MAP_SIZE; ++z) {
            float height = hm->map[x][z];
            int color = ((int)(height * 15.f) & 0x3F) << 20 | ((int)(height * 15.f) & 0x3F) << 24 | ((int)(height * 15.f) & 0x3F) << 28;
            // int color = 15 << 20 | 15 << 24 | 15 << 28;
            PACK_DATA(x, z);
        }
    }

    int j = 0;
    for (int x = 0; x < HEIGHT_MAP_SIZE - 1; ++x) {
        for (int z = 0; z < HEIGHT_MAP_SIZE - 1; ++z) {
            int l = x * HEIGHT_MAP_SIZE + z;
            itx[j++].index = l;
            itx[j++].index = l + HEIGHT_MAP_SIZE;
            itx[j++].index = l + 1;

            itx[j++].index = l + 1;
            itx[j++].index = l + HEIGHT_MAP_SIZE;
            itx[j++].index = l + HEIGHT_MAP_SIZE + 1;
        }
    }

    glBufferData(GL_ARRAY_BUFFER, i * sizeof(Gic__HMVertex), vtx, GL_STATIC_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, j * sizeof(Gic__HMIndex), itx, GL_STATIC_DRAW);
    free(vtx);
    free(itx);

    glBindVertexArray(0);
}

void gic_gl_render_height_map(GicGlRenderer* renderer, GicHeightMap* hm) {
    if (hm->_renderer_internal == NULL) {
        hm->_renderer_internal = malloc(sizeof(Gic__HeightMapData));
        gic_gl__initialize_height_map_data(hm, hm->_renderer_internal);
    }

    Gic__HeightMapData* hmd = hm->_renderer_internal;
    glBindVertexArray(hmd->va);
    glUseProgram(renderer->shaders[SHADER_VOXEL]);
    
    // glDrawArrays(GL_POINTS, 0, HEIGHT_MAP_SIZE * HEIGHT_MAP_SIZE);
    glDrawElements(GL_TRIANGLES, 6 * HEIGHT_MAP_SIZE * HEIGHT_MAP_SIZE, GL_UNSIGNED_INT, NULL);
}

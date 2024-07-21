#include "renderer.h"
#include "shader.h"
#include "camera.h"
#include "vertex.h"
#include "util.h"
#include "platform/sdl_window.h"


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

    bool (*active)[256][64] = malloc(64 * 256 * 64);
    memset(active, 0, 64 * 256 * 64);

    int i = 0;
    int total_face_count = 0;
    for (int x = 0; x < 64; ++x) {
        for (int y = 0; y < 256; ++y) {
            for (int z = 0; z < 64; ++z) {
                // printf("Processing voxel at (%d, %d, %d)\n", x, y, z);

                Voxel* voxel = &renderer->chunk.voxels[x][y][z];
                Index* index = &renderer->chunk.indices[x][y][z];

                voxel->vertices[0].data = ((x + 0) & 0x3F) | ((((y + 0) & 0xFF) << 6) ) | ((((z + 0) & 0x3F) << 14) ) | ((((x & 0x3F) << 20) | ((y & 0x3F) << 24) | ((z & 0x3F) << 28)));
                voxel->vertices[1].data = ((x + 1) & 0x3F) | ((((y + 0) & 0xFF) << 6) ) | ((((z + 0) & 0x3F) << 14) ) | ((((x & 0x3F) << 20) | ((y & 0x3F) << 24) | ((z & 0x3F) << 28)));
                voxel->vertices[2].data = ((x + 0) & 0x3F) | ((((y + 0) & 0xFF) << 6) ) | ((((z + 1) & 0x3F) << 14) ) | ((((x & 0x3F) << 20) | ((y & 0x3F) << 24) | ((z & 0x3F) << 28)));
                voxel->vertices[3].data = ((x + 1) & 0x3F) | ((((y + 0) & 0xFF) << 6) ) | ((((z + 1) & 0x3F) << 14) ) | ((((x & 0x3F) << 20) | ((y & 0x3F) << 24) | ((z & 0x3F) << 28)));
                voxel->vertices[4].data = ((x + 0) & 0x3F) | ((((y + 1) & 0xFF) << 6) ) | ((((z + 0) & 0x3F) << 14) ) | ((((x & 0x3F) << 20) | ((y & 0x3F) << 24) | ((z & 0x3F) << 28)));
                voxel->vertices[5].data = ((x + 1) & 0x3F) | ((((y + 1) & 0xFF) << 6) ) | ((((z + 0) & 0x3F) << 14) ) | ((((x & 0x3F) << 20) | ((y & 0x3F) << 24) | ((z & 0x3F) << 28)));
                voxel->vertices[6].data = ((x + 0) & 0x3F) | ((((y + 1) & 0xFF) << 6) ) | ((((z + 1) & 0x3F) << 14) ) | ((((x & 0x3F) << 20) | ((y & 0x3F) << 24) | ((z & 0x3F) << 28)));
                voxel->vertices[7].data = ((x + 1) & 0x3F) | ((((y + 1) & 0xFF) << 6) ) | ((((z + 1) & 0x3F) << 14) ) | ((((x & 0x3F) << 20) | ((y & 0x3F) << 24) | ((z & 0x3F) << 28)));

                // printf("%d\n%d\n--\n", voxel->vertices[0].data, voxel->vertices[1].data);
                int face_count = 0;

                if (x == 0 || x == 64 || y == 0 || y == 255 || z == 0 || z == 63) {
                index->indices[0] = i + 0;  // Bottom-left
                index->indices[1] = i + 1;  // Bottom-right
                index->indices[2] = i + 2;  // Top-left
                index->indices[3] = i + 1;  // Bottom-right
                index->indices[4] = i + 3;  // Top-right
                index->indices[5] = i + 2;  // Top-left

                // Back Face
                index->indices[6] = i + 4;  // Bottom-left
                index->indices[7] = i + 6;  // Top-left
                index->indices[8] = i + 5;  // Bottom-right
                index->indices[9] = i + 5;  // Bottom-right
                index->indices[10] = i + 6; // Top-left
                index->indices[11] = i + 7; // Top-right

                // Left Face
                index->indices[12] = i + 0;  // Bottom-left
                index->indices[13] = i + 2;  // Top-left
                index->indices[14] = i + 6;  // Top-right
                index->indices[15] = i + 0;  // Bottom-left
                index->indices[16] = i + 6;  // Top-right
                index->indices[17] = i + 4;  // Bottom-right

                // Right Face
                index->indices[18] = i + 1;  // Bottom-left
                index->indices[19] = i + 5;  // Bottom-right
                index->indices[20] = i + 3;  // Top-left
                index->indices[21] = i + 3;  // Top-left
                index->indices[22] = i + 5;  // Bottom-right
                index->indices[23] = i + 7;  // Top-right

                // Top Face
                index->indices[24] = i + 2;  // Bottom-left
                index->indices[25] = i + 3;  // Bottom-right
                index->indices[26] = i + 6;  // Top-left
                index->indices[27] = i + 3;  // Bottom-right
                index->indices[28] = i + 7;  // Top-right
                index->indices[29] = i + 6;  // Top-left

                // Bottom Face
                index->indices[30] = i + 0;  // Bottom-left
                index->indices[31] = i + 4;  // Top-left
                index->indices[32] = i + 1;  // Bottom-right
                index->indices[33] = i + 1;  // Bottom-right
                index->indices[34] = i + 4;  // Top-left
                index->indices[35] = i + 5;  // Top-right

                face_count = 36;
                }
                i += 8;
                total_face_count += face_count;
 
                // Bottom face (y - 1)
                // if (y == 0) {
                //     index->indices[face_count++] = i + 0;
                //     index->indices[face_count++] = i + 4;
                //     index->indices[face_count++] = i + 1;
                //     index->indices[face_count++] = i + 1;
                //     index->indices[face_count++] = i + 4;
                //     index->indices[face_count++] = i + 5;
                // }
    
                // // Top face (y + 1)
                // if (y == 255) {
                //     index->indices[face_count++] = i + 2;
                //     index->indices[face_count++] = i + 3;
                //     index->indices[face_count++] = i + 6;
                //     index->indices[face_count++] = i + 3;
                //     index->indices[face_count++] = i + 7;
                //     index->indices[face_count++] = i + 6;
                // }
    
                // // Front face (z + 1)
                // if (z == 63) {
                //     index->indices[face_count++] = i + 1;
                //     index->indices[face_count++] = i + 5;
                //     index->indices[face_count++] = i + 3;
                //     index->indices[face_count++] = i + 3;
                //     index->indices[face_count++] = i + 5;
                //     index->indices[face_count++] = i + 7;
                // }
    
                // // Back face (z - 1)
                // if (z == 0) {
                //     index->indices[face_count++] = i + 0;
                //     index->indices[face_count++] = i + 2;
                //     index->indices[face_count++] = i + 4;
                //     index->indices[face_count++] = i + 4;
                //     index->indices[face_count++] = i + 2;
                //     index->indices[face_count++] = i + 6;
                // }
    
                // // Right face (x + 1)
                // if (x == 63) {
                //     index->indices[face_count++] = i + 1;
                //     index->indices[face_count++] = i + 5;
                //     index->indices[face_count++] = i + 3;
                //     index->indices[face_count++] = i + 3;
                //     index->indices[face_count++] = i + 5;
                //     index->indices[face_count++] = i + 7;
                // }
    
                // // Left face (x - 1)
                // if (x == 0) {
                //     index->indices[face_count++] = i + 0;
                //     index->indices[face_count++] = i + 4;
                //     index->indices[face_count++] = i + 2;
                //     index->indices[face_count++] = i + 2;
                //     index->indices[face_count++] = i + 4;
                //     index->indices[face_count++] = i + 6;
                // }
            }
        }
    }

    renderer->face_count = total_face_count;

    glCreateVertexArrays(1, &renderer->vao);
    // not a mistake, simply vbo and ibo are next to each other so they can be seen as an array.
    glGenBuffers(2, &renderer->vbo);

    glBindVertexArray(renderer->vao);
    glBindBuffer(GL_ARRAY_BUFFER, renderer->vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, renderer->ibo);

    glBufferData(GL_ARRAY_BUFFER, sizeof(renderer->chunk.voxels), &renderer->chunk.voxels[0][0][0].vertices[0], GL_STATIC_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(renderer->chunk.indices), &renderer->chunk.indices[0][0][0].indices[0], GL_STATIC_DRAW);

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
    
    glm_perspective(glm_rad(renderer->camera.zoom), (float)renderer->width / (float)renderer->height, 0.1f, 100.0f, renderer->projection);
    gic_camera_get_view_matrix(&renderer->camera, renderer->view);

    mat4 mvp;
    glm_mat4_mul(renderer->projection, renderer->view, mvp);
    glUniformMatrix4fv(1, 1, GL_FALSE, &mvp[0][0]);

    glDrawElements(GL_TRIANGLES, renderer->face_count, GL_UNSIGNED_INT, NULL);
}


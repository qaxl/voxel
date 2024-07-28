#include "generation/heightmap.h"
#include "graphics/renderer.h"
#include "platform/sdl_window.h"

// temporary
#include <glad/gl.h>

#include <stdio.h>

int main(int argc, char** argv) {
    GicWindow* window = gic_create_window("schizophrenia", 1024, 768);
    GicGlRenderer* renderer = gic_create_gl_renderer(window);
    if (!window || !renderer) {
        fprintf(stderr, "creating window failed!\n");
        return 1;
    }

    GicHeightMap* hm = gic_height_map_generate_using_perlin(0.1f);

    while (gic_window_is_open(window)) {
        gic_gl_clear(renderer, 0.33f, 0.33f, 0.33f);
        gic_gl_render_height_map(renderer, &hm);

        gic_window_poll_events(window);

        if (window->is_key_pressed[SDL_SCANCODE_F1]) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        }

        if (window->is_key_pressed[SDL_SCANCODE_F2]) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }

        if (window->is_key_pressed[SDL_SCANCODE_TAB]) {
            window->is_mouse_grabbed = !window->is_mouse_grabbed;
            SDL_SetRelativeMouseMode(window->is_mouse_grabbed);
        }

        gic_gl_renderer_swap_buffers(renderer);
    }

    gic_height_map_destroy(hm);
    gic_destroy_gl_renderer(renderer);
    gic_destroy_window(window);
}

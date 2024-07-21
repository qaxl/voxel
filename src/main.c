#include "graphics/renderer.h"
#include "platform/sdl_window.h"

#include <stdio.h>

int main(int argc, char** argv) {
    GicWindow* window = gic_create_window("schizophrenia", 1024, 768);
    GicGlRenderer* renderer = gic_create_gl_renderer(window);
    if (!window || !renderer) {
        fprintf(stderr, "creating window failed!\n");
        return 1;
    }

    while (gic_window_is_open(window)) {
        gic_gl_clear(renderer, 0.33f, 0.33f, 0.33f);
        gic_gl_renderer_swap_buffers(renderer);

        gic_window_poll_events(window);
    }

    gic_destroy_gl_renderer(renderer);
    gic_destroy_window(window);
}

#include "sdl_window.h"
#include "SDL3/SDL_events.h"
#include "util.h"

GicWindow* gic_create_window(const char* title, int width, int height) {
    static bool initialized = false;
    if (!initialized) {
        initialized = true;

        if (SDL_Init(SDL_INIT_VIDEO) != 0) {
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "SDL Initialization Failure", SDL_GetError(), NULL);
            return NULL;
        }
    }

    GicWindow* window = malloc(sizeof(GicWindow));

    window->window = SDL_CreateWindow(title, width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_MOUSE_GRABBED);
    if (!window->window) {
        free(window->window);
        return NULL;
    }

    window->is_open = true;
    window->width = width;
    window->height = height;

    memset(&window->renderer_cbs, 0, sizeof(window->renderer_cbs));
    memset(&window->is_key_pressed, 0, sizeof(window->is_key_pressed));

    return window;
}

void gic_destroy_window(GicWindow* window) {
    SDL_DestroyWindow(window->window);
    free(window);
}

void gic_window_poll_events(GicWindow* window) {
    static SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                window->is_open = false;
                break;

            case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
                window->width = event.window.data1;
                window->height = event.window.data2;

                if (window->renderer_cbs.resize)
                    window->renderer_cbs.resize(window->renderer_cbs.ptr, window->width, window->height);                
                break;

            case SDL_EVENT_KEY_DOWN:
                if (event.key.scancode == SDL_SCANCODE_ESCAPE)
                    window->is_open = false;
                window->is_key_pressed[event.key.scancode] = true;
                break;

            case SDL_EVENT_KEY_UP:
                window->is_key_pressed[event.key.scancode] = false;
                break;

            case SDL_EVENT_MOUSE_MOTION:
                if (window->renderer_cbs.motion)
                    window->renderer_cbs.motion(window->renderer_cbs.ptr, event.motion.xrel, event.motion.yrel);
                break;

            case SDL_EVENT_MOUSE_WHEEL:
                if (window->renderer_cbs.wheel)
                    window->renderer_cbs.wheel(window->renderer_cbs.ptr, event.wheel.y);
                break;
        }
    }
}

bool gic_window_is_open(GicWindow* window) {
    return window->is_open;
}

#pragma once

#include <SDL3/SDL.h>

#include "graphics/renderer.h"
#include "util.h"

typedef void (*GicRendererResizeCallback)(void*, int, int);
typedef void (*GicRendererMouseMotionCallback)(void*, int, int);
typedef void (*GicRendererMouseWheelCallback)(void*, float);

typedef struct GicRendererCallbacks {
    void* ptr;

    GicRendererResizeCallback resize;
    GicRendererMouseMotionCallback motion;
    GicRendererMouseWheelCallback wheel;
} GicRendererCallbacks;

typedef struct GicWindow {
    SDL_Window* window;
    GicRendererCallbacks renderer_cbs;

    int width;
    int height;
    bool is_open;

    bool is_key_pressed[SDL_NUM_SCANCODES];
} GicWindow;

GicWindow* gic_create_window(const char* title, int width, int height);
void gic_destroy_window(GicWindow* window);

void gic_window_poll_events(GicWindow* window);
bool gic_window_is_open(GicWindow* window);


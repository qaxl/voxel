#pragma once

typedef struct GicWindow GicWindow;
typedef struct GicHeightMap GicHeightMap;

// private structure, inside renderer.c
typedef struct GicGlRenderer GicGlRenderer;

GicGlRenderer* gic_create_gl_renderer(GicWindow* window);
void gic_destroy_gl_renderer(GicGlRenderer* renderer);

void gic_gl_renderer_swap_buffers(GicGlRenderer* renderer);
void gic_gl_clear(GicGlRenderer* renderer, float r, float g, float b);
void gic_gl_render_height_map(GicGlRenderer* renderer, GicHeightMap* hm);

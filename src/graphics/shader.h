#pragma once

#include <stdint.h>

typedef uint32_t Shader;

Shader gic_load_shader_from_sources(const char* vertex, const char* fragment);
void gic_destroy_shader(Shader shader);


#include "shader.h"
#include "util.h"

#include <glad/gl.h>

#include <stdio.h>
#include <stdlib.h>

static uint32_t gic__load_shader(GLenum type, const char* source) {
    uint32_t id = glCreateShader(type);
    
    glShaderSource(id, 1, &source, NULL);
    glCompileShader(id);

    GLint success;
    glGetShaderiv(id, GL_COMPILE_STATUS, &success);

    if (success == GL_FALSE) {
        GLsizei size = 0;
        glGetShaderInfoLog(id, 0, &size, NULL);

        char* info_log = malloc(size + 1);
        info_log[0] = 0;

        glGetShaderInfoLog(id, size, NULL, info_log);
        fprintf(stderr, "[gic-renderer] [fatal] %s shader compilation failure at: %s\n", 
            type == GL_VERTEX_SHADER ? 
                "vertex" : 
            type == GL_FRAGMENT_SHADER ? 
                "fragment" : 
            "unknown",

            info_log);

        free(info_log);
        glDeleteShader(id);
        return -1;
    }

    return id;
}

static int gic__check_program_status_and_print_if_not_successful(Shader shader, GLenum type) {
    GLint success;
    glGetProgramiv(shader, type, &success);

    if (success == GL_FALSE) {
        GLsizei size = 0;
        glGetProgramInfoLog(shader, 0, &size, NULL);

        char* info_log = malloc(size + 1);
        info_log[size] = 0;

        glGetProgramInfoLog(shader, size, NULL, info_log);
        fprintf(stderr, "[gic-renderer] [fatal] program compilation failure: %s\n", info_log);

        free(info_log);
        glDeleteProgram(shader);
    }

    return success;
}

Shader gic_load_shader_from_sources(const char* vertex, const char* fragment) {
    uint32_t vertex_id = gic__load_shader(GL_VERTEX_SHADER, vertex);
    uint32_t fragment_id = gic__load_shader(GL_FRAGMENT_SHADER, fragment);

    if (vertex_id == -1 || fragment_id == -1) {
        return -1;
    }

    Shader shader = glCreateProgram();
    glAttachShader(shader, vertex_id);
    glAttachShader(shader, fragment_id);

    glLinkProgram(shader);
    if (!gic__check_program_status_and_print_if_not_successful(shader, GL_LINK_STATUS)) {
        glDeleteProgram(shader);
        glDeleteShader(vertex_id);
        glDeleteShader(fragment_id);
    }

    glValidateProgram(shader);
    if (!gic__check_program_status_and_print_if_not_successful(shader, GL_VALIDATE_STATUS)) {
        glDeleteProgram(shader);
        glDeleteShader(vertex_id);
        glDeleteShader(fragment_id);
    }

    glDeleteShader(vertex_id);
    glDeleteShader(fragment_id);

    return shader;
}

void gic_destroy_shader(Shader shader) {
    glDeleteProgram(shader);
}

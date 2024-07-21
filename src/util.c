#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>

#include <Windows.h>

char* gic_io_load_file_to_string(const char* filename) {
    FILE* f = fopen(filename, "rb");
    if (!f) {
        return NULL;
    }

    struct stat st;
    stat(filename, &st);

#ifndef NDEBUG
    printf("[gic-util] [debug] loading file \"%s\" of %ld bytes\n", filename, st.st_size);
#endif

    char* text = malloc(st.st_size + 1);
    text[st.st_size] = 0;

    fread(text, 1, st.st_size, f);
    fclose(f);

    return text;
}

float gic_get_ticks()
{
    ULONGLONG ticks_ms = GetTickCount64();
    return (float)ticks_ms / 1000.f;
}

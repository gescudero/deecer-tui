//utils.h
#ifndef UTILS_H
#define UTILS_H
#include "models.h"
#include <stdbool.h>
#include <string.h>

// Estructura basica para textos
// con control de lineas
// puede contener track en cada linea individualmente
// también podrá contener playlist
struct content_t {
    char **text;
    size_t numlines;
    size_t maxlines;
    track_t **tracks;
};

content_t* content_create(size_t content_size);
void content_init(content_t *cont, size_t content_size);
void content_add_line(content_t *cont, const char *texto);
void content_add_char(content_t *cont, int line_index, const char c);
void content_add_track(content_t *cont, track_t *track);
bool content_line_is_track(const content_t *cont, int line_index);
void content_add(content_t *dest, const content_t *addition);
void content_clear(content_t *cont);
void content_free(content_t *cont);

#endif

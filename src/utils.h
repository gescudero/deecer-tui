#ifndef UTILS_H
#define UTILS_H

#include <string.h>

// Estructura basica para textos
// con control de lineas
typedef struct {
    char **text;
    size_t numlines;
    size_t maxlines;
} content_t;

content_t* content_create(size_t content_size);
void content_init(content_t *cont, size_t content_size);
void content_add_line(content_t *cont, const char *texto);
void content_add_char(content_t *cont, int line_index, const char c);
void content_copy(content_t *dest, const content_t *origin);
void content_add(content_t *dest, const content_t *addition);
void content_clear(content_t *cont);
void content_free(content_t *cont);

#endif

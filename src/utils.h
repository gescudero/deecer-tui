//utils.h

#ifndef UTILS_H
#define UTILS_H

#include "models.h"

#define LOG(msg, ...) { \
  fprintf(stderr, "[%s][Line %d] ", __FILE__, __LINE__); \
  fprintf(stderr, (msg), ##__VA_ARGS__); \
}

/**
 * Helper to remove extension from a filename string
 * changing the dot with '\0'
 */
void remove_extension(char *filename);

#endif

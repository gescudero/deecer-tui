//utils.h

#ifndef UTILS_H
#define UTILS_H

#include "models.h"
#include "config.h"
#include <stdio.h>

#define LOG(msg, ...) do{ \
    if (log_enabled && log_file) { \
        fprintf(log_file, "[%s][Line %d] ", __FILE__, __LINE__); \
        fprintf(log_file, (msg), ##__VA_ARGS__); \
        fflush(log_file); \
    } \
}while(0)

extern bool log_enabled;
extern FILE *log_file;

/**
 * Helper to remove extension from a filename string
 * changing the dot with '\0'
 */
void remove_extension(char *filename);

#endif

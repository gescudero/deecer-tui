#include "utils.h"
#include <string.h>

void remove_extension(char *filename) {
    // Limpiar todo el path hasta quedarnos con el
    // filename limpio
    char *slash_ptr = strrchr(filename, '/');
    if (slash_ptr) {
        memmove(filename, slash_ptr + 1, strlen(slash_ptr + 1) + 1);
    }
    
    for (int i=0;i<strlen(filename); i++) {
        if (filename[i] == '.') {
            filename[i] = '\0';
            return;
        }
    }
}



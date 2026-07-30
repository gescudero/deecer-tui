#include "utils.h"
#include <sched.h>
#include <stddef.h>
#include <string.h>

void remove_extension(char *filename) {
    for (int i=0;i<strlen(filename); i++) {
        if (filename[i] == '.') {
            filename[i] = '\0';
            return;
        }
    }
}



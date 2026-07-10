// config.h
#ifndef CONFIG_H
#define CONFIG_H

#include "models.h"
#include "stdbool.h"

struct config_t {
    bool is_debug;
    bool deezer_active;
    char *arl;
    char *theme;
};

config_t* config_init();

#endif

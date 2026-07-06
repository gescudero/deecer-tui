// config.h
#ifndef CONFIG_H
#define CONFIG_H

#include "models.h"
#include "stdbool.h"

struct config_t {
    bool is_debug;
    char *arl;
    char *user_id;
};

config_t* config_init();

#endif

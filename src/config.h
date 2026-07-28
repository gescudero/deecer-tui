// config.h
#ifndef CONFIG_H
#define CONFIG_H

#include "models.h"
#include "stdbool.h"

struct config_t {
    bool is_debug;
    bool deezer_active;
    bool keep_downloads;
    char *arl;
    char *theme;
    char *download_path;
};

extern config_t *config;

config_t* config_init();


#endif

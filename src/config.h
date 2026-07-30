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

// global config object
extern config_t *config;

/**
 * Initialize config object
 * allocating memory and reading
 * config file from disk
 *
 * @return: a pointer to the configured config_t object
 */
config_t* config_init();

#endif

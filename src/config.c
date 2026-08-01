//config.c 
#include "config.h"
#include "models.h"
#include "utils.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/**
 * Reads config file and parse it 
 * converting text in a list of 
 * key value pairs and calling config_set_key
 * for each pair
 *
 * @return error code 
 */
static deecer_result_t config_read_file(char *path_to_file);

/**
 * Write each value in each key of the
 * global config_t object if are valid
 *
 * @params key/value pair
 */
static void config_set_key(char *key, char *value);

// config global, definicion inicial
config_t *config = NULL;


// PUBLIC FUNCTIONS 
config_t* config_init() {
    config = calloc(1, sizeof(config_t));
    if (!config) {
        return NULL;
    }
    config->is_debug = false;
    config->deezer_active = false;
    config->keep_downloads = false;

    // Order to try to find the config file
    // 1.$XDG_CONFIG_HOME/deecer-tui/config
    // 2.$HOME/.config/deecer-tui/config 
    // Si no encuentra config sale mostrando error
    char *xdg_config_home_path = getenv("XDG_CONFIG_HOME");
    char *config_file_path = NULL;
    asprintf(&config_file_path, "%s/deecer-tui/config", xdg_config_home_path);
    deecer_result_t err = config_read_file(config_file_path);
    if (DC_SUCCESS == err) {
        free(config_file_path);
        return config;
    }
    char *home_path = getenv("HOME");
    asprintf(&config_file_path, "%s/.config/deecer-tui/config", home_path);
    err = config_read_file(config_file_path);
    if (DC_SUCCESS == err) {
        free(config_file_path);
        return config;
    }
    free(config_file_path);

    printf("ERROR: Can't find config file in this paths or ARL is too short:\n");
    printf("${XDG_CONFIG_HOME}/deecer-tui/config\n");
    printf("${HOME}/.config/deecer-tui/config\n");
    printf("### File content ###\n");
    printf("DEEZER_ARL={your_arl}\n");
    printf("IS_DEBUG=false\n");
    printf("KEEP_DOWNLOADS=false\n");
    printf("DOWNLOAD_PATH=/only/if/keep_downloads/true\n");

    exit(0);
}


// PRIVATE FUNCTIONS
static deecer_result_t config_read_file(char *path_to_file) {
    // check if file exists
    LOG("Intentando acceder a %s\n", path_to_file);
    if (access(path_to_file, F_OK) != 0) {
        return DC_ERROR_FILE_ACCESS;
    }
    FILE *fptr;
    char textline[1024];
    fptr = fopen(path_to_file, "r");
    if (fptr == NULL) {
        return DC_ERROR_FILE_ACCESS;
    }
    while (fgets(textline, 1024, fptr)) {
        char key[256] = {0}; // contenedor para la clave
        char value[512] = {0}; // contenedor para el valor
        bool is_value = false; // switcher entre clave/valor
        int i = 0; // contador para el escacio en file_content 
        int j = 0; // contador para el espacio en value[]
        // Añadimos caracter a caracter para evitar saltos de linea extras
        // y para poder detectar '='
        for (i=0; i < strlen(textline); i++) {
            // detectamos si es un comentario, salto de linea y
            // cualquier caracter no imprimible. Si lo encontramos
            // salimos del for y pasamos a la siguiente linea
            if (31 > textline[i] || '#' == textline[i]) {
                break;
            }
            // si el caracter es = cambiamos de contenedor
            if (textline[i] == '=') {
                is_value = true;
                key[i] = '\0';
                j = 0;
            } else if (32 != textline[i]) {
                // evitamos espacios
                 if (is_value) {
                    value[j] = textline[i];
                    j++;
                } else {
                    key[i] = textline[i];
                }
            }
        }
        config_set_key(key, value);
    }
    fclose(fptr);
    if (strlen(config->arl) < 10) {
        return DC_ERROR_FILE_ACCESS;
    }
    return DC_SUCCESS;
}

static void config_set_key(char *key, char *value) {
    LOG("%s - %s\n", key, value);
    if (strcmp(key, "IS_DEBUG") == 0) {
        if (strcmp(value, "true") == 0) {
            config->is_debug = true;
        } else {
            config->is_debug = false;
        }
    }
    if (strcmp(key, "DEEZER_ARL") == 0) {
        config->arl = strdup(value);
    }
    if (strcmp(key, "THEME") == 0) {
        config->theme = strdup(value);
    }
    if (strcmp(key, "KEEP_DOWNLOADS") == 0) {
        if (strcmp(value, "true") == 0) {
            config->keep_downloads = true;
        } else {
            config->keep_downloads = false;
        }
    }
    if (strcmp(key, "DOWNLOAD_PATH") == 0) {
        if (access(value, F_OK) == 0) {
            config->download_path = strdup(value);
        } else {
            config->download_path = strdup("/tmp");
            config->keep_downloads = false;
        }
    }
}

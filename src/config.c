//config.c 
#include "config.h"
#include "models.h"
#include "utils.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int config_read_file(char *path_to_file);
static void config_set_key(char *key, char *value);

// config global, definicion inicial
config_t *config = NULL;

config_t* config_init() {
    config = calloc(1, sizeof(config_t));
    if (!config) {
        return NULL;
    }
    config->is_debug = false;
    config->deezer_active = false;
    config->keep_downloads = false;
    config_read_file("/home/guille/.deezer/config");
    return config;
}

int config_read_file(char *path_to_file) {
    FILE *fptr;
    char textline[1024];
    fptr = fopen(path_to_file, "r");
    if (fptr == NULL) {
        return 1;
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
    return 0;
}

void config_set_key(char *key, char *value) {
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

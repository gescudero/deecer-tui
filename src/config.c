//config.c 
#include "config.h"
#include "models.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

static config_t app_config;
static int config_read_file(char *path_to_file);
static void config_set_key(char *key, char *value);

config_t* config_init() {
    app_config.is_debug = false;
    config_read_file("/home/guille/.deezer/config");
    return &app_config;
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
            if (31 > textline[i] || '#' == textline[i]) {
                break;
            }
            if (textline[i] == '=') {
                // si el caracter es = cambiamos de contenedor
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
    return 0;
}

void config_set_key(char *key, char *value) {
    if (strcmp(key, "IS_DEBUG") == 0) {
        if (strcmp(value, "true") == 0) {
            app_config.is_debug = true;
        } else {
            app_config.is_debug = false;
        }
    } else if (strcmp(key, "DEEZER_ARL") == 0) {
        app_config.arl = strdup(value);
    } else if (strcmp(key, "USER_ID") == 0) {
        app_config.user_id = strdup(value);
    }  
}

// main.c
#include "config.h"
#include "utils.h"
#include "ui.h"
#include "player.h"
#include "deezer_api.h"
#include <cjson/cJSON.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

void* thread_player_openurl(void *arg); 

int main() {
    bool running = true;
    ui_action_t action;
    char ui_response[256];
    pthread_t player_thread;

    // read config
    config_t *config = config_init(); 
    // init de la api y libcurl
    deezer_init();

    fprintf(stderr, "Config cargada. User id=%s\n", config->user_id);
    // init de curses y la ui
    if (!ui_init()) {
        fprintf(stderr, "Error creado las ventanas.\n");
        return 1;
    }

    // inicializamos el player (libmpv)
    if (!player_init()) {
        fprintf(stderr, "Error creando el player.\n");
        return 1;
    }
    if (config->is_debug) {
        fprintf(stderr, "Estamos en modo debug\n");
    } else {
        fprintf(stderr, "No estamos en modo debug\n");
    }
    while(running) {
        action = ui_handle_input(ui_response); // bloquea esperando tecla
        switch (action) {
            case UI_ACTION_SELECT:
                // ya veremos a ver que hay que hacer
                break;
            case UI_ACTION_SEARCH: {
                // hacer llamadas a la api
                // hacemos la consulta pasandole el texto
                // que hemos recibido del campo de busqueda
                // y esperamos la respuesta
                content_t *resp = deezer_search(ui_response);
                
                if (resp != NULL) {
                    // seteamos el contenido
                    center_set_content(resp);
                }
                break;
            }
            case UI_ACTION_PLAY: {
                // Nos piden reproducir un track
                // para ello conseguimos el content completo del center,
                // Creamos un puntero y lo pasamos para que nos escriban ahi
                // la direccion de memoria al contenido y nos den la linea seleccionada
                content_t *center_content;
                // Ahora mismo pedimos el content y la linea seleccionada
                int selected_line = center_get_selected_line_content(&center_content);
                // comprobamos que la linea seleccionada realmente sea un track,
                // podria ser un texto cualquiera
                if (content_line_is_track(center_content, selected_line-1)) {
                    /***
                     *
                     * MODO PLAYLIST 
                     *
                     * **/
                    // Creamos el fichero.
                    char playlist_path[] = "/tmp/playlist-deezer";
                    FILE *fptr;
                    fptr = fopen(playlist_path,"w");
                    // escribimos en cada linea del fichero una url
                    for (int i=0; i < center_content->numlines; i++) {
                        fprintf(fptr, "%s\n", center_content->tracks[i]->preview);
                    }
                    //cerramos el fichero
                    fclose(fptr);
                    // luego le pasamos la ruta a la funcion que lanza el player en un thread separado
                   if (pthread_create(&player_thread, NULL, thread_player_openurl, (void*)playlist_path) != 0) {
                            fprintf(stderr, "Error creando el thread\n");
                    }


                    /****
                     *
                     * MODO UN SOLO FILE / URL
                     * Reproducimos un solo track no mas
                     *
                     *
                    // ejecutamos la reproduccion en un thread aparte (parece que funciona!!)    
                    if (pthread_create(&player_thread, NULL, thread_player_openurl, (void*)center_content->tracks[selected_line-1]->preview) != 0) {
                        fprintf(stderr, "Error creando el thread\n");
                    }
                    **/
                }
                break;
            }
            case UI_ACTION_QUIT:
                running = false;
                break;
            case UI_ACTION_CHANGE_FOCUS:
            case UI_ACTION_NONE:
            default:
                break;
        }
    }
    // Rutinas de cerrado de la aplicacion 
    pthread_cancel(player_thread);
    deezer_cleanup();
    player_end();
    ui_end();
    return 0;
}

//funcion para el thread del player
void* thread_player_openurl(void *arg) {
    pthread_detach(pthread_self()); // el thread se limpia

    char *url = (char*)arg; // casteamos el argumento
    fprintf(stderr, "[thread_player_openurl] - Pedimos reproducir\n%s\n", url);
    //player_openurl(url);
    player_openplaylist(url);
    return NULL;
}

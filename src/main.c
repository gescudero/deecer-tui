// main.c
#include "config.h"
#include "ui.h"
#include "player.h"
#include "deezer_api.h"
#include <stdlib.h>
#include <stdio.h>
#include <cjson/cJSON.h>
#include <pthread.h>

void* thread_player_openurl(void *arg); 
void* thread_player_openplaylist(void *arg); 

int main() {
    bool running = true;
    ui_action_t action;
    char ui_response[256];
    pthread_t player_thread;

    // read config
    config_t *config = config_init(); 

    LOG("Config cargada. User arl=%s\n", config->arl);
    
    // init de la api y libcurl
    deezer_init(config);
    LOG("Api inicializada.\n");
/***
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
                    if (deezer_playlist_is_valid(resp->playlists[0])) {
                        fprintf(stderr, "Primera playlist: %s\n", resp->playlists[0]->title );
                        fprintf(stderr, "Primera track de la playlist: %s\n", resp->playlists[0]->tracks[1]->title);
                    }
                }
                break;
            }
            case UI_ACTION_LOAD_TRACK: {
                //
                // 
                //  MODO UN SOLO FILE / URL
                //  Reproducimos un solo track no mas
                // 
                // 
                content_t *center_content;
                int selected_line = center_get_selected_line_content(&center_content);
                
                // comprobamos que la linea seleccionada sea un track
                if (content_line_is_track(center_content, selected_line - 1)) {
                    // ejecutamos la reproduccion en un thread aparte (parece que funciona!!)    
                    if (pthread_create(&player_thread, NULL, thread_player_openurl, (void*)center_content->tracks[selected_line-1]->preview) != 0) {
                        fprintf(stderr, "Error creando el thread\n");
                    }
                }


                break;
            }
            case UI_ACTION_LOAD_PLAYLIST: {
                // Nos piden reproducir una playlist
                // para ello conseguimos el content completo del center,
                // Creamos un puntero y lo pasamos para que nos escriban ahi
                // la direccion de memoria al contenido y nos den la linea seleccionada
                content_t *center_content;
                // Ahora mismo pedimos el content y la linea seleccionada
                int selected_line = center_get_selected_line_content(&center_content);
                // comprobamos que la linea seleccionada realmente sea una playlist,
                // podria ser un texto cualquiera
                if (content_line_is_playlist(center_content, selected_line-1)) {
                    //
                    //
                    // MODO PLAYLIST 
                    //
                    // 
                    fprintf(stderr, "[main] Vamos a crear el fichero para la playlist\n");
                    // Creamos el fichero.
                    char *playlist_path = strdup("/tmp/playlist-deezer");
                    FILE *fptr;
                    fptr = fopen(playlist_path,"w");
                    if (fptr == NULL) {
                        fprintf(stderr, "[main] Oh oh, hay problemas con el fichero\n");
                    }
                    fprintf(stderr, "selected line: %d\n", selected_line);
                    fprintf(stderr, "primer track: %s\n", center_content->playlists[0]->tracks[0]->title);
                    // escribimos en cada linea del fichero una url
                    for (int i=0; i < center_content->playlists[selected_line-1]->nb_tracks; i++) {
                        if (deezer_track_is_valid(center_content->playlists[selected_line-1]->tracks[i])) {
                            fprintf(fptr, "%s\n", center_content->playlists[selected_line-1]->tracks[i]->preview);
                            fprintf(stderr, "%s\n", center_content->playlists[selected_line-1]->tracks[i]->preview);
                        }
                    }
                    fprintf(stderr, "[main] Fichero creado\n");
                    // for (int i=0; i < center_content->numlines; i++) {
                    //     fprintf(fptr, "%s\n", center_content->tracks[i]->preview);
                    // }
                    //cerramos el fichero
                    fclose(fptr);
                    // luego le pasamos la ruta a la funcion que lanza el player en un thread separado
                   if (pthread_create(&player_thread, NULL, thread_player_openplaylist, (void*)playlist_path) != 0) {
                            fprintf(stderr, "Error creando el thread\n");
                    }


                }

                break;
            }
            case UI_ACTION_PLAY: {
                player_play();
                break;
            }
            case UI_ACTION_STOP:
                player_stop();
                break;
            case UI_ACTION_PAUSE:
                player_pause();
                break;
            case UI_ACTION_BACK:
                player_back();
                break;
            case UI_ACTION_FORWARD:
                player_forward();
                break;
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
    ***/
    player_end();
    ui_end();
    deezer_cleanup();
    return 0;
}

//funcion para el thread del player
void* thread_player_openurl(void *arg) {
    pthread_detach(pthread_self()); // el thread se limpia

    char *url = (char*)arg; // casteamos el argumento
    fprintf(stderr, "[thread_player_openurl] - Pedimos reproducir\n%s\n", url);
    player_openurl(url);
    free(url);
    return NULL;
}
void* thread_player_openplaylist(void *arg) {
    pthread_detach(pthread_self()); // el thread se limpia

    char *url = (char*)arg; // casteamos el argumento
    fprintf(stderr, "[thread_player_openurl] - Pedimos reproducir\n%s\n", url);
    player_openplaylist(url);
    free(url);
    return NULL;
}

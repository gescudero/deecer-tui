// main.c

#include "config.h"
#include "content.h"
#include "models.h"
#include "ui.h"
#include "player.h"
#include "deezer_api.h"
#include "utils.h"
#include <stdlib.h>
#include <stdio.h>
#include <cjson/cJSON.h>
#include <pthread.h>


/**
 * Reproducir una url o un file mediante un path
 * en un thread independiente
 *
 * @param: recibe la url o el path en char*
 */
void* thread_player_openurl(void *arg); 

/**
 * Reproduce una lista de urls o de files
 * Se le pasa el path a un fichero de texto que debe contener 
 * un path/url en cada linea
 *
 * @param path to file
 */
void* thread_player_openplaylist(void *arg); 

/**
 * MAIN FUNCTION
 */
int main() {
    bool running = true;
    ui_action_t action;
    char ui_response[256];
    pthread_t player_thread = 0;

    // read config
    config = config_init(); 

    LOG("Config cargada. User arl=%s\n", config->arl);
    LOG("keep_downloads: %d\n", config->keep_downloads);
    LOG("download_dir: %s\n", config->download_path);
    
    if (deezer_arl_is_valid(config->arl)) {
        // init de la api y libcurl
        int err = deezer_init(config);
        if (err == DC_SUCCESS) {
            config->deezer_active = true;
            LOG("Api inicializada.\n");
        } else {
            LOG("Ha ocurrido un error iniciando la api de deezer. ERROR CODE: %d\n", err);
        }
    } else {
        LOG("No existe una clave ARL válida.\n");
    }
    // init de curses y la ui
    if (!ui_init()) {
        LOG("Error creado las ventanas.\n");
        return 1;
    }

    // inicializamos el player (libmpv)
    if (!player_init()) {
        LOG("Error creando el player.\n");
        return 1;
    }
    if (config->is_debug) {
        LOG("Estamos en modo debug\n");
    } else {
        LOG("No estamos en modo debug\n");
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
                    LOG("Hemos recibido los resultados de la busqueda.\n");
                    if (resp->numlines > 0) {
                        LOG("linea 1: %s\n", resp->text[0]);
                    } else {
                        LOG("No hay texto.\n");
                    }
                    // seteamos el contenido
                    center_set_content(resp);
                    // if (deezer_playlist_is_valid(resp->playlists[0])) {
                    //     fprintf(stderr, "Primera playlist: %s\n", resp->playlists[0]->title );
                    //     fprintf(stderr, "Primera track de la playlist: %s\n", resp->playlists[0]->tracks[1]->title);
                    // }
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
                LOG("Reproduccion seleccionada\n");
                // comprobamos que la linea seleccionada sea un track
                if (content_line_is_track(center_content, selected_line - 1)) {
                    // hay que descargar el fichero, descodificarlo y reproducirlo
                    // ya veremos quien hace cada cosa 
                    track_t *track = center_content->tracks[selected_line - 1];
                    if (track != NULL) {
                        LOG("url de descarga: %s\n", track->media_url);
                    }
                    LOG("Has seleccionado reproducir %s\n", track->title);
                    LOG("Comenzando descarga...\n");
                    char *filename = NULL;
                    int err = deezer_get_media(track, &filename);
                    if (DC_ERROR_CURL_DOWNLOADING == err) {
                        LOG("No hemos podido descargar el fichero.\n");
                        break;
                    }
                    if (DC_ERROR_DECRYPT == err) {
                        LOG("Hemos tenido problemas con el desencriptado.\n");
                        break;
                    }
                    player_stop();
                    if (player_thread != 0) {
                        pthread_join(player_thread, NULL);
                        player_thread = 0;
                    }
                    // ejecutamos la reproduccion en un thread aparte (parece que funciona!!)    
                    if (pthread_create(&player_thread, NULL, thread_player_openurl, (void*)filename) != 0) {
                        LOG("Error creando el thread\n");
                    }
                    char *nowplaying_text = NULL;
                    asprintf(&nowplaying_text, "%s - %s", track->artists[0]->name, track->title);
                    now_playing_set_content(nowplaying_text);
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
                    LOG("[main] Vamos a crear el fichero para la playlist\n");
                    // Creamos el fichero.
                    char *playlist_path = strdup("/tmp/playlist-deezer");
                    FILE *fptr;
                    fptr = fopen(playlist_path,"w");
                    if (fptr == NULL) {
                        LOG("[main] Oh oh, hay problemas con el fichero\n");
                    }
                    LOG("selected line: %d\n", selected_line);
                    // escribimos en cada linea del fichero una url
                    for (int i=0; i < center_content->playlists[selected_line-1]->nb_tracks; i++) {
                        if (deezer_track_is_valid(center_content->playlists[selected_line-1]->tracks[i])) {
                            // Debemos escribir la ruta al fichero de cada tema (aunque no exista todavia)
                            char *filepath = deezer_get_filepath(center_content->playlists[selected_line - 1]->tracks[i]);
                            if (filepath) {
                                fprintf(fptr, "%s\n", filepath);
                                LOG("Añadido %s a la playlist.\n", filepath);
                                free(filepath);
                            }
                            // voy a forzar a descargar cada fichero de momento es temporal
                            // aunque para listas de 10 o 15 canciones funciona sorprendentemente bien
                            char *filename = NULL;
                            deezer_get_media(center_content->playlists[selected_line-1]->tracks[i], &filename);
                        }
                    }
                    LOG("[main] Fichero creado\n");
                    fclose(fptr);
                    
                    player_stop();
                    if (player_thread != 0) {
                        pthread_join(player_thread, NULL);
                        player_thread = 0;
                    }
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
    LOG("Vamos cerrando:\n");
    player_stop();
    //primero le decimos a mpv que vaya saliendo de su bucle
    //seteando la variable global player_running a 0,
    //esperamos a que el hilo se acabe y lo seteamos a 0
    if (player_thread != 0) {
        player_running = 0;
        pthread_join(player_thread, NULL);
        player_thread = 0;
    }
    LOG("hilos,\n");
    // con los hilos ya en su sitio, podemos cerrar el player
    player_end();
    LOG("player_end(),\n");
    deezer_cleanup();
    LOG("deezer,\n");
    ui_end();
    LOG("curses, adeu.\n")

    return 0;
}

void* thread_player_openurl(void *arg) {
    char *url = (char*)arg; // casteamos el argumento
    LOG("[thread_player_openurl] - Pedimos reproducir\n%s\n", url);
    player_openurl(url);
    free(url);
    return NULL;
}
void* thread_player_openplaylist(void *arg) {
    char *url = (char*)arg; // casteamos el argumento
    LOG("[thread_player_openurl] - Pedimos reproducir\n%s\n", url);
    player_openplaylist(url);
    free(url);
    return NULL;
}

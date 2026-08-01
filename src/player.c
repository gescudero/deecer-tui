#include "player.h"
#include "deezer_api.h"
#include "models.h"
#include "ui.h"
#include "utils.h"
#include <sched.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <mpv/client.h>

// EL HANDLE
static mpv_handle *mpv;
int player_running = 0;

/**
 * Notifica a la ui la cancion que esta sonando actualmente
 * Le envia un string en formato track_name (artist_name)
 *
 */
static void player_notify_now_playing(char *filename); 

/**
 * funcion para gestionar errores, esta copiada del 
 * ejemplo simple.c de mpv-player/mpv-examples
 * debo ajustarla a mis necesidades
 */
static inline void check_error(int status);

/**
 * configura el handle de mpv con los settings que necesitamos
 * para la app
 */
static void player_configure(mpv_handle *handle); 

// ================
// PUBLIC FUNCTIONS
// ================
bool player_init() {
    mpv = mpv_create();
    if (!mpv) {
        LOG("Error creando el contexto de mpv.\n");
        player_end();
        return false;
    }
    check_error(mpv_initialize(mpv));
    
    //aplicamos configuraciones
    player_configure(mpv);
    //comprobamos que todo haya salido bien
    if (mpv_initialize(mpv) < 0) {
        LOG("Error inicializando mpv\n");
        player_end();
        return false;
    }
    return true;
}

void player_end() {
    mpv_terminate_destroy(mpv);
}

void player_openurl(char *url){
    const char *cmd[] = {"loadfile", url, NULL};
    LOG("antes de mpv_command...\n");
    check_error(mpv_command(mpv, cmd));
    LOG("[player] Loadfile command...\n");
    player_running = 1;
    while (player_running) {
        mpv_event *event = mpv_wait_event(mpv, 100);
        LOG("event: %s\n", mpv_event_name(event->event_id));
        if (event->event_id == MPV_EVENT_SHUTDOWN) {
            player_running = 0;
            break;
        }
    }
    player_running = 0;
}

void player_openplaylist(char *file_path) {
    const char *cmd[] = {"loadlist", file_path, NULL};
    check_error(mpv_command(mpv,cmd));
    LOG("[player] Loadlist command...\n");
    
    // esta opcion fuerza a que se pare cada vez que acabe de reproducir un 
    // fichero. En el evento end-of-file, yo pongo en pausa, me aseguro
    // de que el fichero este descargado y continuo la reproduccion
    mpv_set_option_string(mpv, "keep-open", "yes");
    
    player_running = 1;
    while (player_running) {
        mpv_event *event = mpv_wait_event(mpv, 100);
        if (event->event_id != MPV_EVENT_NONE) {
            // LOG("[playlist] event: %s\n", mpv_event_name(event->event_id));
        }
        if (event->event_id == MPV_EVENT_SHUTDOWN) {
            player_running = 0;
            break;
        }
        if (event->event_id == MPV_EVENT_END_FILE) {
            LOG("MPV_EVENT_END_FILE.\n");
            player_pause();
            LOG("Reproductor pausado.\n");
            // esperamos hasta que nos confirmen que estamos pausados
            int paused = 0;
            while (!paused) {
                mpv_get_property(mpv, "pause", MPV_FORMAT_FLAG, &paused);
                if (!paused) {
                    mpv_wait_event(mpv, 0.1);
                }
            }
            /**
             * Averiguamos en que posicion estamos
             */
            int64_t playlist_pos = -1;
            mpv_get_property(mpv, "playlist-pos", MPV_FORMAT_INT64, &playlist_pos);
            LOG("playlist_pos: %lu\n", playlist_pos);
            /**
             * la property playlist nos devuelve una lista de nodos
             * en la que cada nodo es un track, que incluye 
             * - filename (con path completo)
             * - id 
             * - playlist-path
             */
            mpv_node playlist_node = {0};
            mpv_get_property(mpv, "playlist", MPV_FORMAT_NODE, &playlist_node);
            mpv_node_list *playlist_list = playlist_node.u.list;
            for (int i=0;i<playlist_list->num;i++) {
                mpv_node *item = &(playlist_list->values[i]);
                // todo lo que se recibe son NODE_MAP
                if (MPV_FORMAT_NODE_MAP == item->format) {
                    mpv_node_list *itemlist = item->u.list;
                    for (int j=0; j<itemlist->num; j++) {
                        if (i == playlist_pos) {
                            if (strcmp(itemlist->keys[j], "filename") == 0) {
                                char *filename = strdup(itemlist->values[j].u.string);
                                remove_extension(filename);
                                track_t *track = deezer_get_track(strtoul(filename, NULL, 10));
                                LOG("NEXT TRACK: %s\n", track->title);
                                // Ahora es cuando descargaríamos el tema
                                LOG("downloading (o no si ya lo tenemos)...\n");
                                deezer_get_media(track, &filename);
                                LOG("fichero %s descargado.\n", filename);
                                free(filename);
                            }
                        } else {
                            if (strcmp(itemlist->keys[j], "filename") == 0) {
                                char *filename = strdup(itemlist->values[j].u.string);
                                remove_extension(filename);
                                track_t *track = deezer_get_track(strtoul(filename, NULL, 10));
                                LOG("%d: %s\n", i, track->title);
                                // Ahora es cuando descargaríamos el tema
                                free(filename);
                            }

                        }
                    }
                }
            }
            LOG("Reproduciendo de nuevo\n");
            player_play();
        }
        if (event->event_id == MPV_EVENT_PROPERTY_CHANGE) {
            // LOG("MPV_EVENT_PROPERTY_CHANGE\n");
            mpv_event_property *prop = (mpv_event_property*)event->data;
            // LOG("prop->name: %s\n", prop->name);
            // LOG("prop->format: %d\n", prop->format);
            if (strcmp(prop->name, "media-title") == 0 && prop->format == MPV_FORMAT_STRING) {
                char *title = *(char**)prop->data;
                // ponemos la reproduccion en pausa con player_pause()
                // compruebo si el fichero esta descargado
                // si esta descargado reproducimos con player_play() y notificamos now_playing
                // si no esta descargado, notificamos "downloading"
                // y comenzamos la descarga, y cuando acabe la descarga continuamos
                // con player_play()
                player_notify_now_playing(title);
            }
        }
    }
    player_running = 0;
    LOG("[playlist] - Hemos salido del bucle de notificaciones ------\n");
}

void player_play() {
    // Reading a flag property
    int pausa;
    mpv_get_property(mpv, "pause", MPV_FORMAT_FLAG, &pausa);
    if (pausa > 0) {
        pausa = 0;
        check_error(mpv_set_property(mpv, "pause", MPV_FORMAT_FLAG, &pausa));
        LOG("[player] Saliendo de la pausa. \n");
    }
}

void player_stop() {
    player_running = 0;
    const char *cmd[] = {"stop", "", NULL};
    check_error(mpv_command(mpv, cmd));
    LOG("[player] Comando stop\n");
}

void player_pause() {
    // Set a property to a string value
    int pausa = 1;
    check_error(mpv_set_property(mpv, "pause", MPV_FORMAT_FLAG, &pausa));
    LOG("[player] Pause set property. \n");
}

void player_forward() {
    LOG("[player] Solicitada proxima cancion en la lista\n");
    const char *cmd[] = {"playlist-next", "weak", NULL};
    mpv_command_async(mpv, 0, cmd);  // asíncrono, no bloquea
}

void player_back() {
    LOG("[player] Solicitada anterior cancion en la lista\n");
    const char *cmd[] = {"playlist-prev", "weak", NULL};
    check_error(mpv_command(mpv, cmd));
}

void player_shuffle() {
    LOG("Shuffle activo.\n");
    const char *cmd[] = {"playlist-shuffle", NULL};
    check_error(mpv_command(mpv, cmd));
    const char *cmd_pos[] = {"playlist-play-index", "1", NULL};
    mpv_command(mpv, cmd_pos);
}

void player_unshuffle() {
    LOG("Unshuffle activo.\n");
    const char *cmd[] = {"playlist-unshuffle", NULL};
    check_error(mpv_command(mpv, cmd));
}

// =================
// PRIVATE FUNCTIONS
// =================

static inline void check_error(int status) {
    if (status < 0) {
        LOG("mpv API error: %s\n", mpv_error_string(status));
    }
}

static void player_configure(mpv_handle *handle) {
    mpv_set_option_string(handle, "terminal", "no");
    mpv_set_option_string(handle, "msg-level", "all=error");
    mpv_set_option_string(handle, "no-video", "yes");
    mpv_set_option_string(handle, "audio-display", "no");
    mpv_set_option_string(handle, "input-default-bindings", "yes");
    // nos suscribimos al evento de cambio de media-title (cambio de cancion)
    mpv_observe_property(mpv, 0, "media-title", MPV_FORMAT_STRING);
}

static void player_notify_now_playing(char *filename) {
    if (filename) {
        LOG("filename: %s\n", filename);
        remove_extension(filename);
        unsigned long track_id = strtoul(filename, NULL, 10);
        track_t *track = deezer_get_track(track_id);
        if (!track || !track->title) {
            LOG("No hemos podido notificar lo que esta sonando.\n");
            return;
        }
        char *real_title = NULL;
        if (!track->artists[0] || !track->artists[0]->name) {
            asprintf(&real_title, "%s (unknown)", track->title);
        } else {
            asprintf(&real_title, "%s (%s)", track->title, track->artists[0]->name);
        }
        LOG("Nueva canción: %s\n",real_title);
        now_playing_set_content(real_title);
        free(real_title);
    }
}


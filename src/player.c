#include "player.h"
#include "utils.h"
#include <sched.h>
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>
#include <mpv/client.h>
// EL HANDLE
static mpv_handle *mpv;

// funcion para gestionar errores, esta copiada del 
// ejemplo simple.c de mpv-player/mpv-examples
// debo ajustarla a mis necesidades
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
}

// Inicializacion del player, lo conservaremos durante la vida
// del programa
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
// video killed the radio star
void player_end() {
    mpv_terminate_destroy(mpv);
}
// esta funcion es capaz de reproducir una url si no esta encriptada
void player_openurl(char *url){
    player_stop();
    const char *cmd[] = {"loadfile", url, NULL};
    check_error(mpv_command(mpv, cmd));
    LOG("[player] Loadfile command...\n");
    while (1) {
        mpv_event *event = mpv_wait_event(mpv, 10000);
        LOG("event: %s\n", mpv_event_name(event->event_id));
        if (event->event_id == MPV_EVENT_SHUTDOWN) {
            break;
        }
    }
}
// igual que la funcion anterior pero que recibe una lista
void player_openplaylist(char *url) {
    player_stop();
    const char *cmd[] = {"loadlist", url, NULL};
    check_error(mpv_command(mpv,cmd));
    LOG("[player] Loadlist command...\n");
    while (1) {
        mpv_event *event = mpv_wait_event(mpv, 10000);
        LOG("[playlist] event: %s\n", mpv_event_name(event->event_id));
        if (event->event_id == MPV_EVENT_SHUTDOWN) {
            break;
        } else if (event->event_id == MPV_EVENT_END_FILE) {
            // Salta cada vez que acaba una cancion,
            // asi que aqui deberiamos avisar de que hemos
            // cambiado de track
            // NO ESTOY SEGURO
            LOG("MPV_EVENT_END_FILE");

            break;
        }
    }
}
void player_stop() {
    const char *cmd[] = {"stop", "", NULL};
    check_error(mpv_command(mpv, cmd));
    LOG("[player] Comando stop\n");
}
// Resume play 
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

// Pause play
void player_pause() {
    // Set a property to a string value
    int pausa = 1;
    check_error(mpv_set_property(mpv, "pause", MPV_FORMAT_FLAG, &pausa));
    LOG("[player] Pause set property. \n");
}
// Next song on playlist
void player_forward() {
    LOG("[player] Solicitada proxima cancion en la lista\n");
    const char *cmd[] = {"playlist-next", "weak", NULL};
    check_error(mpv_command(mpv, cmd));
}
// Previous song on playlist
void player_back() {
    LOG("[player] Solicitada anterior cancion en la lista\n");
    const char *cmd[] = {"playlist-prev", "weak", NULL};
    check_error(mpv_command(mpv, cmd));
}
// option 
int player_get_time_pos(double *pos) {
    mpv_get_property(mpv, "time-pos", MPV_FORMAT_DOUBLE, pos);
    return 0;
}

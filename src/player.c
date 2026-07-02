#include "player.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <mpv/client.h>
// funcion para gestionar errores, esta copiada del 
// ejemplo simple.c de mpv-player/mpv-examples
// debo ajustarla a mis necesidades
static inline void check_error(int status) {
    if (status < 0) {
        printf("mpv API error: %s\n", mpv_error_string(status));
        exit(1);
    }
}

mpv_handle *ctx;

bool player_init() {
    ctx = mpv_create();
    if (!ctx) {
        fprintf(stderr, "Error creando el contexto de mpv.\n");
        return false;
    }
    check_error(mpv_initialize(ctx));
    return true;
}
void player_end() {
    mpv_terminate_destroy(ctx);
}

void player_openurl(char *url){
    const char *cmd[] = {"loadfile", url, NULL};
    check_error(mpv_command(ctx, cmd));
    while (1) {
        mpv_event *event = mpv_wait_event(ctx, 10000);
        fprintf(stderr, "event: %s\n", mpv_event_name(event->event_id));
        if (event->event_id == MPV_EVENT_SHUTDOWN) {
            break;
        }
    }
}


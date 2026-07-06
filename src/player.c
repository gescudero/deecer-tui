#include "player.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <mpv/client.h>
// EL HANDLE
static mpv_handle *mpv;

// funcion para gestionar errores, esta copiada del 
// ejemplo simple.c de mpv-player/mpv-examples
// debo ajustarla a mis necesidades
static inline void check_error(int status) {
    if (status < 0) {
        printf("mpv API error: %s\n", mpv_error_string(status));
        exit(1);
    }
}
static void player_configure(mpv_handle *handle) {
    mpv_set_option_string(handle, "terminal", "yes");
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
        fprintf(stderr, "Error creando el contexto de mpv.\n");
        player_end();
        return false;
    }
    check_error(mpv_initialize(mpv));
    
    //aplicamos configuraciones
    player_configure(mpv);
    //comprobamos que todo haya salido bien
    if (mpv_initialize(mpv) < 0) {
        fprintf(stderr, "Error inicializando mpv\n");
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
    const char *cmd[] = {"loadfile", url, NULL};
    check_error(mpv_command(mpv, cmd));
    while (1) {
        mpv_event *event = mpv_wait_event(mpv, 10000);
        fprintf(stderr, "event: %s\n", mpv_event_name(event->event_id));
        if (event->event_id == MPV_EVENT_SHUTDOWN) {
            break;
        }
    }
}
// igual que la funcion anterior pero que recibe una lista
void player_openplaylist(char *url) {
    const char *cmd[] = {"loadlist", url, NULL};
    check_error(mpv_command(mpv,cmd));
    while (1) {
        mpv_event *event = mpv_wait_event(mpv, 10000);
        fprintf(stderr, "[playlist] event: %s\n", mpv_event_name(event->event_id));
        if (event->event_id == MPV_EVENT_SHUTDOWN) {
            break;
        } else if (event->event_id == MPV_EVENT_END_FILE) {
            fprintf(stderr, "MPV_EVENT_END_FILE");
            break;
        }
    }
}
void player_playfile(char *audio_data, size_t audio_size) {
    // Comprobaciones del audio recibido
    if (audio_size < 100) {
        fprintf(stderr, "Error: Archivo demasiado pequeño");
        return;
    }
    // Creamos archivo temporal (no se porque todavia)
    char temp_path[] = "/tmp/deezer_audio_XXXXXX.mp3";
    int fd = mkstemps(temp_path, 4);
    if (fd == -1) {
        perror("Error creando archivo temporal");
        return;
    }
    // Escribimos en el archivo temporal
    ssize_t written = write(fd, audio_data, audio_size);
    close(fd);

    if (written != (ssize_t)audio_size) {
        fprintf(stderr, "Error escribiendo archivo temporal\n");
        unlink(temp_path);
        return;
    }
    // Cargar archivo
    const char *cmd[] = {"loadfile", temp_path, NULL};
    check_error(mpv_command(mpv, cmd));
    fprintf(stderr, "Reproduciendo %s\n", temp_path);
    while (1) {
        mpv_event *event = mpv_wait_event(mpv, 10000);
        if (event->event_id == MPV_EVENT_SHUTDOWN) {
            break;
        }
    }
}

unsigned char* player_download_encrypted_data() {
    unsigned char* encrypted_data = NULL;

    return encrypted_data;
}



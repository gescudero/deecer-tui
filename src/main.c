#include "utils.h"
#include "ui.h"
#include "player.h"
#include "deezer_api.h"
#include <cjson/cJSON.h>
#include <ncurses.h>
#include <stdio.h>
#include <pthread.h>

//funcion para el thread del player
void* thread_player_openurl(void *arg) {
    pthread_detach(pthread_self()); // el thread se limpia

    char *url = (char*)arg; // casteamos el argumento
    player_openurl(url);
    return NULL;
}


int main() {
    bool running = true;
    ui_action_t action;
    char ui_response[256];
    pthread_t player_thread;

    deezer_init();

    // Seccion window curses
    // init
    if (!ui_init()) {
        fprintf(stderr, "Error creado las ventanas.");
        return 1;
    }

    //Inicializamos el player
    if (!player_init()) {
        fprintf(stderr, "Error creando el player.");
        return 1;
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
                    // liberamos la memoria de la respuesta y de nuestro buffer
                    content_free(resp);
                }
                break;
                                   }
            case UI_ACTION_PLAY: {
                // Nos piden reproducir un track
                // ahora mismo recibimos una url para
                // reproducir el preview de la api publica
                content_t *preview_url = content_create(1);
                content_add_line(preview_url, ui_response);
                center_set_content(preview_url);
                //test pthread basico
                if (pthread_create(&player_thread, NULL, thread_player_openurl, (void*)preview_url->text[0]) != 0) {
                    fprintf(stderr, "Error creando el thread\n");
                }
                //player_openurl(preview_url->text[0]);
                // damos un tiempo antes de liberar la memoria del content
                napms(100);
                content_free(preview_url);
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
    deezer_cleanup();
    player_end();
    ui_end();
    return 0;
}

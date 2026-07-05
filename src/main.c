#include "utils.h"
#include "ui.h"
#include "player.h"
#include "deezer_api.h"
#include <cjson/cJSON.h>
#include <stdio.h>
#include <pthread.h>

void* thread_player_openurl(void *arg); 

int main() {
    bool running = true;
    ui_action_t action;
    char ui_response[256];
    pthread_t player_thread;
    
    // init de la api y libcurl
    deezer_init();

    // init de curses y la ui
    if (!ui_init()) {
        fprintf(stderr, "Error creado las ventanas.");
        return 1;
    }

    // inicializamos el player (libmpv)
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
                }
                break;
            }
            case UI_ACTION_PLAY: {
                // Nos piden reproducir un track
                // para ello conseguimos el content completo del center,
                // aunque quizas seria mejor tener una variable global
                // con el contenido y no necesitar pedirla (valorar)
                content_t *center_content = content_create(1);
                if (center_content == NULL) {
                    fprintf(stderr, "Error creando center_content\n");
                }
                // Ahora mismo pedimos el content y la linea seleccionada
                int selected_line = center_get_selected_line_content(center_content);
                // comprobamos que la linea seleccionada realmente sea un track,
                // podria ser un texto cualquiera
                if (content_line_is_track(center_content, selected_line-1)) {
                    // ejecutamos la reproduccion en un thread aparte (parece que funciona!!)    
                    if (pthread_create(&player_thread, NULL, thread_player_openurl, (void*)center_content->tracks[selected_line-1]->preview) != 0) {
                        fprintf(stderr, "Error creando el thread\n");
                    }
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
    fprintf(stderr, "[thread_player_openurl] - Pedimos reproducir %s\n", url);
    player_openurl(url);
    return NULL;
}

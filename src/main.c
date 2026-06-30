#include "utils.h"
#include "ui.h"
#include "deezer_api.h"
#include <cjson/cJSON.h>
#include <stdio.h>

int main() {
    bool running = true;
    ui_action_t action;
    char search_query[256];

    deezer_init();

    // Seccion window curses
    // init
    if (!ui_init()) {
        fprintf(stderr, "Error creado las ventanas.");
        return 1;
    }

    while(running) {
        action = ui_handle_input(search_query); // bloquea esperando tecla

        switch (action) {
            case UI_ACTION_SELECT:
                // ya veremos a ver que hay que hacer
                break;
            case UI_ACTION_SEARCH: {
                // hacer llamadas a la api
                // hacemos la consulta pasandole el texto
                // que hemos recibido del campo de busqueda
                // y esperamos la respuesta
                content_t *resp = deezer_search(search_query);
                
                if (resp != NULL) {
                    // seteamos el contenido
                    center_set_content(resp);
                    // liberamos la memoria de la respuesta y de nuestro buffer
                    content_free(resp);
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
    deezer_cleanup();
    ui_end();
    return 0;
}

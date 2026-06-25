#include "utils.h"
#include "ui.h"
#include "deezer_api.h"
#include <stdio.h>

int main() {
    bool running = true;
    content_t content;
    ui_action_t action;
    char *api_resp;

    content_init(&content, 1);
    // Seccion window curses
    // init
    if (!ui_init()) {
        fprintf(stderr, "Error creado las ventanas.");
        return 1;
    }

    while(running) {
        action = ui_handle_input(); // bloquea esperando tecla

        switch (action) {
            case UI_ACTION_SELECT:
                // ya veremos a ver que hay que hacer
                break;
            case UI_ACTION_SEARCH:
                // hacer llamadas a la api
                // imaginemos que la api nos ha devuelto este string
                // y despues de tratarlo lo hemos convertido a 
                // un array de strings linea a linea 
                api_resp = deezer_search("query");
                content_add_line(&content, api_resp);
                center_set_content(&content);
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
    return 0;
}

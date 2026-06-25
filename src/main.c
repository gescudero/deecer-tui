#include <stdio.h>
#include "ui.h"

int main() {
    bool running = true;
    // Seccion window curses
    // init
    if (!ui_init()) {
        fprintf(stderr, "Error creado las ventanas.");
        return 1;
    }

    while(running) {
        ui_action_t action = ui_handle_input(); // bloquea esperando tecla
        char *resp[6] = {
            "Linea 1",
            "Linea 2",
            "Linea 3",
            "Linea 4",
            "Linea 5\tHe puesto un tab",
            "Linea 6"
        };

        switch (action) {
            case UI_ACTION_SELECT:
                // ya veremos a ver que hay que hacer
                break;
            case UI_ACTION_SEARCH:
                // hacer llamadas a la api
                // imaginemos que la api nos ha devuelto este string
                // y despues de tratarlo lo hemos convertido a 
                // un array de strings linea a linea 
                center_set_content(resp, 6);
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

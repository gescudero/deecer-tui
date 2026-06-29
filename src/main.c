#include "utils.h"
#include "ui.h"
#include "deezer_api.h"
#include <stdio.h>

int main() {
    bool running = true;
    content_t content;
    ui_action_t action;
    char *api_resp;
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
            case UI_ACTION_SEARCH:
                // hacer llamadas a la api
                
                // inicializamos el buffer
                content_init(&content, 1);
                // limpiamos la ventana mientras hacemos
                // la busqueda, aqui pondriamos una barra 
                // de progreso si lo creemos necesario
                center_set_content(&content);
                // hacemos la consulta pasandole el texto
                // que hemos recibido del campo de busqueda
                // y esperamos la respuesta
                api_resp = deezer_search(search_query);
                // ahora mismo recibimos un string largo
                // en formato json, pero probablemente debamos
                // recibir algo ya formateado como un objeto personalizado
                // que contenga una lista de items del tipo que 
                // estemos buscando
                //
                //Comprobamos que no recibamos respuesta NULL
                if (api_resp == NULL) {
                    break;
                }
                //añadimos una linea al contenido con la respuesta
                content_add_line(&content, api_resp);
                // seteamos el contenido
                center_set_content(&content);
                // liberamos la memoria de la respuesta y de nuestro buffer
                content_clear(&content);
                if (api_resp != NULL) {
                    free(api_resp);
                    api_resp = NULL;
                }
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
    deezer_cleanup();
    ui_end();
    return 0;
}

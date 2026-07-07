//ui.h
#ifndef UI_H
#define UI_H

#include "utils.h"
#include <ncursesw/curses.h>

#define MARGIN 1 //Valor margenes entre ventanas y con el borde de la ventana

typedef struct {
    char* name; // nombre identificativo
    WINDOW *win; // la ventana
    int height; // numero de filas 
    int width; // numero de columnas
    int starty; // fila inicial
    int startx; // columna inicial
    bool has_focus; // si tiene el foco actualmente
    content_t *content; // el contenido
    int selected_line; // la linea seleccionada actualmente
}section_t;

typedef enum {
    UI_ACTION_NONE,
    UI_ACTION_QUIT,
    UI_ACTION_SEARCH,
    UI_ACTION_SELECT,
    UI_ACTION_LOAD_TRACK,
    UI_ACTION_LOAD_PLAYLIST,
    UI_ACTION_PLAY,
    UI_ACTION_PAUSE,
    UI_ACTION_STOP,
    UI_ACTION_FORWARD,
    UI_ACTION_BACK,
    UI_ACTION_CHANGE_FOCUS
} ui_action_t;

typedef enum {
    UI_PLAYER_NONE,
    UI_PLAYER_BACK,
    UI_PLAYER_STOP,
    UI_PLAYER_PLAY,
    UI_PLAYER_PAUSE,
    UI_PLAYER_FORWARD,
} ui_player_button_t;

bool ui_init();
void ui_end();
ui_action_t ui_handle_input(char *return_value);

void center_set_content(content_t *content);
int center_get_selected_line_content(content_t **content);

#endif

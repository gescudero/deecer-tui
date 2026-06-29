#ifndef UI_H
#define UI_H

#include "utils.h"
#include <ncurses.h>

#define MARGIN 1 //Valor margenes entre ventanas y con el borde de la ventana

typedef struct {
    char* name; // nombre identificativo
    WINDOW *win; // la ventana
    int height; // numero de filas 
    int width; // numero de columnas
    int starty; // fila inicial
    int startx; // columna inicial
    bool has_focus; // si tiene el foco actualmente
    content_t content; // el contenido
    int selected_line; // la linea seleccionada actualmente
}section_t;

typedef enum {
    UI_ACTION_NONE,
    UI_ACTION_QUIT,
    UI_ACTION_SEARCH,
    UI_ACTION_SELECT,
    UI_ACTION_CHANGE_FOCUS
} ui_action_t;

void section_print(section_t *sec);
int section_getch(section_t *sec);
void section_delwin(section_t *sec);
void section_next_option(section_t *sec);
void section_prev_option(section_t *sec);
void section_set_focus(section_t *sec);
void section_unset_focus(section_t *sec);
const char* section_get_selected_value(section_t *sec);

bool ui_init();
void ui_end();
void ui_start_colors();
void ui_change_focus();
ui_action_t ui_handle_input(char *return_value);

int menu_create_window();

int search_create_window();
void search_init_text();
void search_set_focus();

int center_create_window();
void center_set_content(content_t *content);

#endif

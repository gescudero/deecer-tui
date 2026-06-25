#ifndef UI_H
#define UI_H

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
    char **content; // contenido por filas 
    int numlines; // numero de lineas de contenido
    int maxlines; // memoria reservada para el contenido
    int selected_line; // la linea seleccionada actualmente
}Section;

typedef enum {
    UI_ACTION_NONE,
    UI_ACTION_QUIT,
    UI_ACTION_SEARCH,
    UI_ACTION_SELECT,
    UI_ACTION_CHANGE_FOCUS
} ui_action_t;

void section_init_content(Section *sec, int content_size);
void section_add_line(Section *sec, const char *texto);
void section_add_char(Section *sec, int line_index, const char c);
void section_clear(Section *sec); 
void section_print(Section *sec);
int section_getch(Section *sec);
void section_delwin(Section *sec);
void section_next_option(Section *sec);
void section_prev_option(Section *sec);
void section_set_focus(Section *sec);
void section_unset_focus(Section *sec);
const char* section_get_selected_value(Section *sec);

bool ui_init();
void ui_end();
void ui_start_colors();
void ui_change_focus();
ui_action_t ui_handle_input();

int menu_create_window();

int search_create_window();
void search_init_text();
void search_set_focus();

int center_create_window();
void center_set_content(char *content[], int numlines);

#endif

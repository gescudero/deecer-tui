//ui.h

#ifndef UI_H
#define UI_H

#include "models.h"
#include <ncursesw/curses.h>

#define MARGIN 1 //Valor margenes entre ventanas y con el borde de la ventana

struct section_t {
    char* name; // nombre identificativo
    WINDOW *win; // la ventana
    int height; // numero de filas 
    int width; // numero de columnas
    int starty; // fila inicial
    int startx; // columna inicial
    bool has_focus; // si tiene el foco actualmente
    content_t *content; // el contenido
    int selected_line; // la linea seleccionada actualmente
};

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

/**
 * UI init. Prepare ncurses and create WINDOW
 * and the different sections and contents
 */
bool ui_init();

/**
 * Clear windows and free memory of curses and contents
 */
void ui_end();

/**
 * Funcion que se ejecuta en el bucle principal y que espera 
 * la accion del usuario. Una vez el usuario pulsa alguna tecla
 * ejecutamos lo necesario y devolvemos la accion realizada
 * excepto en el caso de la busqueda, que nos quedamos en un bucle
 * para capturar lo que el usuario escriba por teclado
 * hasta que el usuario pulse ENTER o TAB para salir
 *
 * @param return_value puntero para devolver 
 * el termino de la busqueda del usuario. 
 *
 * @return devuelve el tipo de accion realizada
 */
ui_action_t ui_handle_input(char *return_value); 

/**
 * Put content in center section. Usefull to fill the list
 * of tracks of a playlist or a search or whatever
 *
 * @param content: :/
 */
void center_set_content(content_t *content);

/**
 * This function is for getting content of center and
 * the selected line.
 * @param returns the content of center section
 *
 * @return the number of selected line
 */
int center_get_selected_line_content(content_t **content);

/**
 * Put text in now_playing section
 */
void now_playing_set_content(char *text);

#endif

#include "ui.h"
#include "content.h"
#include "deezer_api.h"
#include "models.h"
#include "utils.h"
#include <locale.h>
#include <ncursesw/curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <wchar.h>

/****************
 *
 * private vars declaration
 *
 ****************/ 
// dimensiones de la pantalla 
static int screen_height, screen_width;
// Las secciones de la UI
static section_t menu = {0};
static section_t search = {0};
static section_t center = {0};
static section_t playerui = {0};
static section_t now_playing = {0};
static section_t user_playlists = {0};

/****************
* 
* private functions declaration
*
*****************/

/**
 * Init content of sections 
 */
static void ui_init_content();

/**
 * Init ncurses and windows of sections
 */
static bool ui_init_windows();

/**
 * Frees windows and curses
 */
static void ui_end_windows();

/**
 * Frees contents 
 */
static void ui_end_content();

/**
 * Set up for color pairs
 */
static void ui_start_colors();

/**
 * Half refresh. Just kidding, full refresh
 */
static void ui_full_refresh();

/**
 * Controls the focus cycle when user press 
 * TAB key 
 */
static void ui_change_focus();

/**
 * Returns true if dimensions of the window has changed
 */
static bool ui_console_changed_size();

/**
 * Refresh section with the current content 
 * Cleans the window and print again all the
 * content.
 *
 * @param sec: the section_t to update
 */
static void section_print(section_t *sec);

/**
 * wgetch wrapper for the section_t
 *
 * @param sec: the section_t 
 * @return: the char value
 */
static int section_getch(section_t *sec); 

/**
 * Delete window of section_t
 */
static void section_delwin(section_t *sec); 

/**
 * Select next line of the content of the section
 * usually select the line below
 *
 * @param sec: the section_t
 */
static void section_next_option(section_t *sec); 

/**
 * Select the previous line of the content of the 
 * section_t
 */
static void section_prev_option(section_t *sec); 

/**
 * Set focus the section_t passed as argument
 * and refresh content in window
 */
static void section_set_focus(section_t *sec); 

/**
 * Unset as focused the section_t passed as argument
 * ans refresh content in window
 */
static void section_unset_focus(section_t *sec); 

/**
 * Returns the text of the selected line of the section_t
 *
 * @param sec: the section_t
 * @return text from the selected line
 */
static const char* section_get_selected_value(section_t *sec); 

/**
 * returns a pointer of the section_t that is current 
 * focused by the user
 */
static section_t *section_get_focused();

/**
 * Ejecuta la accion correspondiente a cada seccion 
 * cuando el usuario pulsa enter. Se encarga de
 * mirar que tiene seleccionado el usuario y devolver
 * lo que toque
 *
 * @return ui_action_t
 */
static ui_action_t section_exec_action(section_t *sec);

/**
 * Create the first content of 
 * the menu section
 */
static void menu_create_content();

/**
 * Create the window of the menu with its 
 * dimensions and style and print its content
 */
static deecer_result_t menu_create_window();

/**
 *
 */
static void search_create_content();

/**
 *
 */
static deecer_result_t search_create_window();

/**
 *
 */
static void search_init_text();

/**
 *
 */
static deecer_result_t center_create_window();

/**
 *
 */
static void center_create_content();

/**
 *
 */
static deecer_result_t playerui_create_window();

/**
 *
 */
static void playerui_create_content();

/**
 *
 */
static ui_action_t playerui_get_selected_action();

/**
 *
 */
static int now_playing_create_window();

/**
 *
 */
static void now_playing_create_content();

/**
 *
 */
static int user_playlists_create_window();

/**
 *
 */
static void user_playlists_create_content();

// ================
// PUBLIC FUNCTIONS
// ================

bool ui_init() {
    ui_init_content();
    return ui_init_windows();
}
// Destruccion de la UI
void ui_end() {
    ui_end_windows();
    ui_end_content();
}

ui_action_t ui_handle_input(char *return_value) {
    // Primero comprobamos si la terminal ha cambiado de tamaño.
    if (ui_console_changed_size()) {
        //redraw everything
        ui_full_refresh();
    }
    int pressed_key = 0;

    if (search.has_focus) {
        // Caso mas diferente al resto de secciones
        // Hasta que no pulsen ENTER o TAB, cada tecla la escribimos en el campo
        // de texto. Desde la funcion content_add_char controlamos si le pasamos
        // pulsaciones de teclas no imprimibles o Suprimir/Backspace
        while (pressed_key != 10 && pressed_key != 9) {
            pressed_key = wgetch(search.win);
            content_add_char(search.content, 0, pressed_key); // la añadimos a nuestro content
            section_print(&search);
        }
        // una vez fuera del bucle comprobamos si era ENTER o TAB
        if (pressed_key == 9) {
            // con TAB cambiamos foco
            ui_change_focus();
            return UI_ACTION_CHANGE_FOCUS;
        } else if (pressed_key == 10) {
            // con enter cambiamos foco Y devolvemos accion de SEARCH
            ui_change_focus();
            strcpy(return_value, search.content->text[0]);
            return UI_ACTION_SEARCH;
        }
        return UI_ACTION_NONE;
    }

    // el resto de secciones se comportan mas o menos igual 
    section_t *focused_section = section_get_focused();

    pressed_key = section_getch(focused_section);
    if (!pressed_key) {
        return UI_ACTION_NONE;
    }
    switch (pressed_key) {
        case KEY_UP:
            // en todos los casos menos con el playerui que mueve en 
            // horizontal <- ->
            if (strcmp(focused_section->name, "playerui") != 0) {
                section_prev_option(focused_section);
            }
            break;
        case KEY_DOWN:
            // en itodos los casos menos con el playerui que mueve en 
            // horizontal <- ->
            if (strcmp(focused_section->name, "playerui") != 0) {
                section_next_option(focused_section);
            }
            break;
        case KEY_LEFT:
            // soloi en el caso de player ui 
            if (strcmp(focused_section->name, "playerui") == 0) {
                section_prev_option(focused_section);
            }
            break;
        case KEY_RIGHT:
            // solo ien el caso de player ui 
            if (strcmp(focused_section->name, "playerui") == 0) {
                section_next_option(focused_section);
            }
            break;
        case 9:
            ui_change_focus();
            return UI_ACTION_CHANGE_FOCUS;
            break;
        case 10:
            return section_exec_action(focused_section);
            break;
        case 'q':
        case 'Q':
            return UI_ACTION_QUIT;
            break;
        default:
            break;
    }
    section_print(focused_section);

    return UI_ACTION_NONE;
}

void center_set_content(content_t *content) {
    // Liberamos el espacio del contenido anterior
    if (center.content != NULL) {
        content_free(center.content);
        center.content = NULL;
    }
    // Apuntamos hacia el nuevo content
    center.content = content;
    // refrescamos la pantalla escribiendo el nuevo contenido.
    section_print(&center);
}

int center_get_selected_line_content(content_t **content) {
    // igualamos un puntero que nos pasan como parametro
    // a la direccion de memoria del content de center
    // no ocupa nueva memoria, solo apunta al mismo lugar
    // una vez liberado center.content, el puntero que
    // nos han pasado deja de tener contenido.
    *content = center.content;
    // devolvemos la linea seleccionada porque en content no 
    // se sabe que linea hay seleccionada
    // ¿deberiamos pasar puntero a section_t?
    return center.selected_line;
}

void now_playing_set_content(char *text) {
    now_playing.content->text[0] = strdup(text);
    section_print(&now_playing);
}

// =================
// PRIVATE FUNCTIONS
// =================

static void ui_init_content() {
    menu_create_content();
    search_create_content();
    center_create_content();
    playerui_create_content();
    now_playing_create_content();
    user_playlists_create_content();
}

static bool ui_init_windows() {
    // funciones de inicializacion de ncurses
    setlocale(LC_ALL, "");
    setlocale(LC_NUMERIC, "C");
    initscr();
    cbreak();
    noecho();
    curs_set(0);
    ui_start_colors();
    keypad(stdscr, FALSE);
    werase(stdscr);
    refresh();
    // dimensiones de la pantalla
    getmaxyx(stdscr, screen_height, screen_width);

    // creamos nuestras secciones
    if (DC_SUCCESS != menu_create_window()) {
        ui_end();
        return false;
    }
    if (DC_SUCCESS != search_create_window()) {
        ui_end();
        return false;
    }
    if (DC_SUCCESS != !center_create_window()) {
        ui_end();
        return false;
    }
    if (DC_SUCCESS != !playerui_create_window()) {
        return false;
    }
    if (DC_SUCCESS != !now_playing_create_window()) {
        return false;
    }
    if (DC_SUCCESS != user_playlists_create_window()) {
        return false;
    }

    return true;
}

static void ui_end_windows() {
    section_delwin(&menu);
    section_delwin(&user_playlists);
    section_delwin(&search);
    section_delwin(&center);
    section_delwin(&playerui);
    clrtoeol();
    endwin();
}

static void ui_end_content() {
    content_free(menu.content);
    content_free(user_playlists.content);
    content_free(search.content);
    content_free(center.content);
    content_free(playerui.content);
}

static void ui_start_colors() {
    start_color();
    init_pair(1, COLOR_CYAN, COLOR_BLACK);
    init_pair(2, COLOR_YELLOW, COLOR_BLACK);
}

static void ui_full_refresh() {
    ui_end_windows();
    ui_init_windows();
}

static void ui_change_focus() {
    if (menu.has_focus) {
        // si teniamos foco en menu, pasamos a user_playlists 
        section_unset_focus(&menu);
        section_set_focus(&user_playlists);
    } else if (user_playlists.has_focus) {
        section_unset_focus(&user_playlists);
        section_set_focus(&search);
    } else if (search.has_focus) {
        // si teniamos foco en search pasamos a center
        section_unset_focus(&search);
        section_set_focus(&center);
    } else if (center.has_focus) {
        // si teniamos foco en center, volvemos a menu
        section_unset_focus(&center);
        section_set_focus(&playerui);
    } else if (playerui.has_focus) {
        section_unset_focus(&playerui);
        section_set_focus(&menu);
    }
}

static bool ui_console_changed_size() {
    int new_height;
    int new_width;
    
    getmaxyx(stdscr, new_height, new_width);

    if (new_height != screen_height || new_width != screen_width) {
        LOG("Han cambiado las dimensiones %dx%d\n", new_width, new_height);
        screen_width = new_width;
        screen_height = new_height;
        return true;
    }
    return false;
}

static void section_print(section_t *sec) {
    // borramos el contenido que hubiera en pantalla
    werase(sec->win);
    
    // si tenemos el foco, cambiamos el color del borde 
    if (sec->has_focus) {
        wattron(sec->win, COLOR_PAIR(1));
        box(sec->win, 0, 0);
        wattroff(sec->win, COLOR_PAIR(1));
    } else {
        box(sec->win, 0, 0);
    }

    // padding del texto
    int x = 2;
    int y = 1;
    // contador de lineas (del content_t)
    int i = 0;
    // maximo numero de caracteres
    // ancho de la seccion menos dos columnas por
    // los bordes y menos 1 por el indice base 0
    int max_char = sec->width - 3;

    // hemos seleccionado una linea que no cabe, entonces vamos moviendo 
    // la primera linea que imprimimos, de forma que creamos el scroll
    if (strcmp(sec->name, "playerui") != 0 &&sec->selected_line > sec->height -2) {
        i = (sec->selected_line - (sec->height - 2));
    }
    // imprimimos linea a linea y pintamos diferente 
    // la linea seleccionada
    for (; i<sec->content->numlines; ++i) {
        if (y < (sec->height - 1)) {
            // texto que imprimiremos
            char *printed_text = strdup(sec->content->text[i]);
            if (strlen(printed_text) > max_char) {
                printed_text[max_char] = '\0';
            }

            if (sec->selected_line == i+1) {
                wattron(sec->win, A_REVERSE);
                // tenemos que cambiar mvwprintw que imprime una linea y si no cabe
                // pasa a escribir en la siguiente, por ujna funcion que escriba 
                // solo lo que que quepa en el espacio que tiene
                mvwprintw(sec->win, y, x, "%s", printed_text);
                wattroff(sec->win, A_REVERSE);
            } else {
                mvwprintw(sec->win, y, x, "%s", printed_text);
            }
            free(printed_text);
            printed_text = NULL;
        }
       // playerui escribe en horizontal, el resto en vertical
        if (strcmp(sec->name, "playerui") == 0 ) {
            x += strlen(sec->content->text[i]) + 2;
        } else {
            ++y;
        }
    }
    // En el caso de la ventana de now_playing volvemos a dibujar
    // el borde para eliminar posibles caracteres que se hayan salido
    // tambien aprovechamos para cambiar el color.
    if (strcmp(sec->name, "now_playing") == 0) {
        wattron(sec->win, COLOR_PAIR(2));
        box(sec->win, 0, 0);
        wattron(sec->win, COLOR_PAIR(2));
    }

    // refrescamos la ventana
    wrefresh(sec->win);
}

static int section_getch(section_t *sec) {
    return wgetch(sec->win);
}

static void section_delwin(section_t *sec) {
    if (sec->win != NULL) {
        werase(sec->win);
        delwin(sec->win);
    }
}

static void section_next_option(section_t *sec) {
    if (sec->selected_line == sec->content->numlines) {
        sec->selected_line = 1;
    } else {
        ++sec->selected_line;
    }
}

static void section_prev_option(section_t *sec) {
    if (sec->selected_line == 1) {
        sec->selected_line = sec->content->numlines;
    } else {
        --sec->selected_line;
    }
}

static void section_set_focus(section_t *sec) {
    // excepcion para la ventana de busqueda
    if (strcmp(sec->name, "search") == 0) {
        content_clear(sec->content);
        content_init(sec->content, 2);
    }
    // marcamos la variable a true
    sec->has_focus = true;
    // activamos la entrada del teclado en la ventana
    keypad(sec->win, TRUE);
    // seleccionamos la primera linea por defecto
    // excepto si es playerui
    if (strcmp(sec->name, "playerui") == 0) {
        sec->selected_line = UI_PLAYER_PLAY;
    } else {
        sec->selected_line = 1;
    }
    // repintamos la pantalla
    section_print(sec);
}

static void section_unset_focus(section_t *sec) {
    // guardamos el estado actual
    sec->has_focus = false;
    // desactivamos la entrada por teclado
    keypad(sec->win, FALSE);
    // eliminamos la seleccion excepto en center
    if (strcmp(sec->name, "center") != 0) {
        sec->selected_line = 0;
    }
    // refrescamos la pantalla
    section_print(sec);
}

static const char* section_get_selected_value(section_t *sec) {
    return sec->content->text[sec->selected_line - 1];
}

static section_t *section_get_focused() {
    if (menu.has_focus) {
        return &menu;
    }
    if (user_playlists.has_focus) {
        return &user_playlists;
    }
    if (search.has_focus) {
        return &search;
    }
    if (center.has_focus) {
        return &center;
    }
    if (playerui.has_focus) {
        return &playerui;
    }
    return NULL;
}

static ui_action_t section_exec_action(section_t *sec) {
    if (strcmp(sec->name, "menu") == 0) {
        // seleccionada opcion
        // de momento solo imprimimos
        // la seleccion en la ventana central
        content_add_line(menu.content, section_get_selected_value(&menu));
        section_print(&menu);
        return UI_ACTION_SELECT;
    }
    
    if (strcmp(sec->name, "user_playlists") == 0) {
        LOG("[ui] Has seleccionado una playlist del usuario.\n");
        if (content_line_is_playlist(user_playlists.content, user_playlists.selected_line - 1)) {
            //load playlist tracks on center
            // get content from playlist id 
            unsigned long selected_id = user_playlists.content->playlists[user_playlists.selected_line - 1]->id;
            content_fill_with_playlists(center.content, selected_id);
            section_print(&center);
            section_unset_focus(sec);
            section_set_focus(&center);
        }
    }
    
    if (strcmp(sec->name, "center") == 0) {
        // hay que reproducir la seleccion
        // debemos diferenciar si ha seleccionado un track 
        // o una playlist
        //
        // pasamos playerui el estado para reflejarlo en la ui y preseleccionar el play
        // pero no cambiamos el foco, simplemente iluminamos el play
        LOG("[ui] Has seleccionado reproducir.\n");
        playerui.selected_line = UI_PLAYER_PLAY;
        section_print(&playerui);
        if (content_line_is_track(center.content, center.selected_line -1)) {
            return UI_ACTION_LOAD_TRACK;
        } else if(content_line_is_playlist(center.content, center.selected_line -1)) {
            return UI_ACTION_LOAD_PLAYLIST;
        }
        return UI_ACTION_NONE;
    }
    
    if (strcmp(sec->name, "playerui") == 0) {
        return playerui_get_selected_action();
    }
    return UI_ACTION_NONE;
}

static void menu_create_content() {
     // contenido
    menu.has_focus = true; // el foco al iniciar la app es en el menu
    menu.selected_line = 1; // preseleccionamos la primera linea
    menu.content = content_create(5); // inicializamos el contenido y le reservamos espacio para 5 lineas
    content_add_line(menu.content, "Home");
    content_add_line(menu.content, "Explore");
    content_add_line(menu.content, "Library");
    content_add_line(menu.content, "Settings");
}

static deecer_result_t menu_create_window() {
    menu.name = strdup("menu");
    menu.height = 6;
    menu.width = 16;
    menu.starty = MARGIN;
    menu.startx = 0;
    menu.win = newwin(menu.height, menu.width, menu.starty, menu.startx);

    if (menu.win == NULL) {
        return DC_ERROR_UI_INIT; // devolvemos fallo
    }
    box(menu.win, 0, 0);
    keypad(menu.win, TRUE);

    section_print(&menu);
    return DC_SUCCESS; // devolvemos OK
}



static void search_create_content() {
     // Contenido 
    search.has_focus = false; // no tenemos el foco al inicial la app
    search.selected_line = 0; // en esta ventana no se usa el campo selected_line
    search.content = content_create(2); // inicializamos la memoria con 2 lineas de maximo.
}
static deecer_result_t search_create_window() {
    search.name = strdup("search");
    search.height = 3;
    search.width = (screen_width - (menu.width + (MARGIN*2)))/2;
    search.starty = MARGIN;
    search.startx = menu.width + (MARGIN*2);

    search.win = newwin(search.height, search.width, search.starty, search.startx);

    if (search.win == NULL) {
        return 0; // devolvemos error
    }
    box(search.win, 0, 0);
    wrefresh(search.win);

    search_init_text(); // inicializamos el texto
    return 1;
}
static void search_init_text() {
    content_add_line(search.content, "Search ..."); // añadimos la linea al contenido.
    wattron(search.win, COLOR_PAIR(1)); // set color
    mvwprintw(search.win, 1, 1, "%s", search.content->text[search.content->numlines - 1]); // imprimimos la linea
    wattroff(search.win, COLOR_PAIR(1)); // unset color
    wrefresh(search.win);
}

static deecer_result_t center_create_window() {
    center.name = strdup("center");
    center.height = screen_height - (search.height + MARGIN + 3);
    center.width = screen_width - (menu.width + (MARGIN*2));
    center.starty = search.height + (MARGIN);
    center.startx = menu.width + (MARGIN*2);

    center.win = newwin(center.height, center.width, center.starty, center.startx);

    if (center.win == NULL) {
        return DC_ERROR_UI_INIT;
    }

    box(center.win, 0, 0);

    section_print(&center);

    return DC_SUCCESS;
}
void center_create_content() {
     // contenido
    center.has_focus = false;
    center.selected_line = 0; // numero de linea seleccionada
    center.content = content_create(center.height);
    content_add_line(center.content, "Ábaco ábaco Bienvenido a deecer <3");
    char *tmp_str;
    asprintf(&tmp_str, "Tamaño de panel. width:%d ; height:%d", center.width, center.height);
    content_add_line(center.content, tmp_str);
    free(tmp_str);
}

static deecer_result_t playerui_create_window() {
    playerui.name = strdup("playerui");
    playerui.height = screen_height - (search.height + center.height + MARGIN);
    playerui.width = 2;
    for (int i=0; i<playerui.content->numlines; i++) {
        playerui.width += strlen(playerui.content->text[i]) + 2;
    }
    playerui.starty = center.starty + center.height;
    playerui.startx = (center.width / 2) - (playerui.width / 2) + menu.width;

    playerui.win = newwin(playerui.height, playerui.width, playerui.starty, playerui.startx);

    if (playerui.win == NULL) {
        return DC_ERROR_UI_INIT;
    }

    playerui.has_focus = false;
    playerui.selected_line = 0;
    
    section_print(&playerui);

    return DC_SUCCESS;
}
static void playerui_create_content() {
    playerui.content = content_create(5);
    content_add_line(playerui.content, "[BACK]");
    content_add_line(playerui.content, "[STOP]");
    content_add_line(playerui.content, "[PLAY]");
    content_add_line(playerui.content, "[PAUSE]");
    content_add_line(playerui.content, "[FORWARD]");
}

static ui_action_t playerui_get_selected_action() {
    switch (playerui.selected_line) {
        case UI_PLAYER_BACK:
            return UI_ACTION_BACK;
            break;
        case UI_PLAYER_STOP:
            return UI_ACTION_STOP;
            break;
        case UI_PLAYER_PLAY:
            return UI_ACTION_PLAY;
            break;
        case UI_PLAYER_PAUSE:
            return UI_ACTION_PAUSE;
            break;
        case UI_PLAYER_FORWARD:
            return UI_ACTION_FORWARD;
            break;
        default:
            return UI_ACTION_NONE;
            break;
    }
}

static deecer_result_t now_playing_create_window() {
    now_playing.name = strdup("now_playing");
    now_playing.height = 3;
    now_playing.width = screen_width - (menu.width + search.width + (MARGIN*2));
    now_playing.starty = MARGIN;
    now_playing.startx = menu.width + search.width + (MARGIN*2);

    now_playing.win = newwin(now_playing.height, now_playing.width, now_playing.starty, now_playing.startx);

    if (!now_playing.win) {
        return DC_ERROR_UI_INIT;
    }
    now_playing.has_focus = false;
    now_playing.selected_line = 0;
    box(now_playing.win, 0, 0);
    wrefresh(now_playing.win);

    section_print(&now_playing);
    return DC_SUCCESS;
}

static void now_playing_create_content() {
    now_playing.content = content_create(1);
    content_add_line(now_playing.content, "...");
}

static deecer_result_t user_playlists_create_window() {
    user_playlists.name = strdup("user_playlists");
    user_playlists.height = 16;
    user_playlists.width = 16;
    user_playlists.starty = menu.height + MARGIN;
    user_playlists.startx = 0;
    user_playlists.win = newwin(user_playlists.height, user_playlists.width, user_playlists.starty, user_playlists.startx);

    if (user_playlists.win == NULL) {
        return DC_ERROR_UI_INIT; // devolvemos fallo
    }
    box(user_playlists.win, 0, 0);
    keypad(user_playlists.win, TRUE);

    section_print(&user_playlists);
    return DC_SUCCESS; // devolvemos OK
}

void user_playlists_create_content() {
     // contenido
    user_playlists.has_focus = false;
    user_playlists.selected_line = 0; // numero de linea seleccionada
    user_playlists.content = content_create(user_playlists.height);
    
    user_t *user = deezer_get_user();

    for (int i=0; i < user->nb_playlists; i++) {
        content_add_playlist(user_playlists.content, user->playlists[i]);
    }
}


#include "ui.h"
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


/****************
* 
* private functions declaration
*
*****************/
//ui 
static void ui_init_content();
static bool ui_init_windows();
static void ui_end_windows();
static void ui_end_content();
static void ui_start_colors();
static void ui_full_refresh();
static void ui_change_focus();
static bool ui_console_changed_size();
//section 
static void section_print(section_t *sec);
static int section_getch(section_t *sec); 
static void section_delwin(section_t *sec); 
static void section_next_option(section_t *sec); 
static void section_prev_option(section_t *sec); 
static void section_set_focus(section_t *sec); 
static void section_unset_focus(section_t *sec); 
static const char* section_get_selected_value(section_t *sec); 
//menu 
static int menu_create_window();
static void menu_create_content();
//search 
static int search_create_window();
static void search_create_content();
static void search_init_text();
//center 
static int center_create_window();
static void center_create_content();
//playerui 
static int playerui_create_window();
static void playerui_create_content();
static ui_action_t playerui_get_selected_action();

/*******************
 *
 * public funcions (declared in ui.h)
 *
 * bool ui_init();
 * void ui_end();
 * ui_action_t ui_handle_input(char *return_value);
 *
 * void center_set_content(content_t *content);
 * int center_get_selected_line_content(content_t *content);
 *
 *******************/ 


/*************
 * 
 * section functions definitions
 *
 *************/ 

// Print content
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
    // hemos seleccionado una linea que no cabe, entonces vamos moviendo 
    // la primera linea que imprimimos, de forma que creamos el scroll
    if (strcmp(sec->name, "playerui") != 0 &&sec->selected_line > sec->height -2) {
        i = (sec->selected_line - (sec->height - 2));
    }
    // imprimimos linea a linea y pintamos diferente 
    // la linea seleccionada
    for (; i<sec->content->numlines; ++i) {
        if (y < (sec->height - 1)) {
            if (sec->selected_line == i+1) {
                wattron(sec->win, A_REVERSE);
                mvwprintw(sec->win, y, x, "%s", sec->content->text[i]);
                wattroff(sec->win, A_REVERSE);
            } else {
                mvwprintw(sec->win, y, x, "%s", sec->content->text[i]);
            }
        }
       // playerui escribe en horizontal, el resto en vertical
        if (strcmp(sec->name, "playerui") == 0 ) {
            x += strlen(sec->content->text[i]) + 2;
        } else {
            ++y;
        }
    }
    // refrescamos la ventana
    wrefresh(sec->win);
}
// Get char from section
static int section_getch(section_t *sec) {
    return wgetch(sec->win);
}
// Delete win
static void section_delwin(section_t *sec) {
    if (sec->win != NULL) {
        werase(sec->win);
        delwin(sec->win);
    }
}
// Avanzar a la siguiente linea/opcion seleccionable
static void section_next_option(section_t *sec) {
    if (sec->selected_line == sec->content->numlines) {
        sec->selected_line = 1;
    } else {
        ++sec->selected_line;
    }
}
// Retroceder a la opcion anterior
static void section_prev_option(section_t *sec) {
    if (sec->selected_line == 1) {
        sec->selected_line = sec->content->numlines;
    } else {
        --sec->selected_line;
    }
}
// Fijar el foco en una seccion
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
// Quitamos el foco de la ventana
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
// Devolvemos el contenido de la linea seleccionada

static const char* section_get_selected_value(section_t *sec) {
    return sec->content->text[sec->selected_line - 1];
}

/*************
 *
 *  curses functions definitions
 *
 ************/
// Inicializacion de ncurses y nuestras ventanas
static void ui_init_content() {
    menu_create_content();
    search_create_content();
    center_create_content();
    playerui_create_content();
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
    if (!menu_create_window()) {
        ui_end();
        return false;
    }
    if (!search_create_window()) {
        ui_end();
        return false;
    }
    if (!center_create_window()) {
        ui_end();
        return false;
    }
    if (!playerui_create_window()) {
        return false;
    }

    return true;

}
bool ui_init() {
    ui_init_content();
    return ui_init_windows();
}
// Destruccion de la UI
void ui_end() {
    ui_end_windows();
    ui_end_content();
}
// Liberamos ventanas
static void ui_end_windows() {
    section_delwin(&menu);
    section_delwin(&search);
    section_delwin(&center);
    section_delwin(&playerui);
    clrtoeol();
    endwin();
}
// Liberamos contenido
static void ui_end_content() {
    content_free(menu.content);
    content_free(search.content);
    content_free(center.content);
    content_free(playerui.content);
}
// Inicializacion de los pares de colores
static void ui_start_colors() {
    start_color();
    init_pair(1, COLOR_CYAN, COLOR_BLACK);
}
// Full refresh
static void ui_full_refresh() {
    fprintf(stderr, "Limpieza de ventanas radical\n");
    ui_end_windows();
    ui_init_windows();

}
// Cambio de foco de ventana. 
// Comportamiento al pulsar la tecla TAB
static void ui_change_focus() {
    if (menu.has_focus) {
        // si teniamos foco en menu, pasamos a search
        section_unset_focus(&menu);
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
//
// Funcion que comprueba si ha cambiado
// la altura o la anchura de la ventana padre
//
static bool ui_console_changed_size() {
    int new_height;
    int new_width;
    
    getmaxyx(stdscr, new_height, new_width);

    if (new_height != screen_height || new_width != screen_width) {
        fprintf(stderr, "Han cambiado las dimensiones %dx%d\n", new_width, new_height);
        screen_width = new_width;
        screen_height = new_height;
        return true;
    }
    return false;
}
//
// Funcion que se ejecuta en el bucle principal y que espera 
// la accion del usuario. Una vez el usuario pulsa alguna tecla
// ejecutamos lo necesario y devolvemos la accion realizada
// excepto en el caso de la busqueda, que nos quedamos en un bucle
// para capturar lo que el usuario escriba por teclado
// hasta que el usuario pulse ENTER o TAB para salir
//
ui_action_t ui_handle_input(char *return_value) {
    // Primero comprobamos si la terminal ha cambiado de tamaño.
    if (ui_console_changed_size()) {
        //redraw everything
        ui_full_refresh();
    }
    int pressed_key = 0;

    if (menu.has_focus) {   // Acciones en la ventana de menu
        pressed_key = section_getch(&menu);
        switch (pressed_key) {
            case KEY_UP: // flecha arriba
                section_prev_option(&menu);
                break;
            case KEY_DOWN: // flecha abajo
                section_next_option(&menu);
                break;
            case 9: // TAB
                ui_change_focus();
                return UI_ACTION_CHANGE_FOCUS;
                break;
            case 10: // ENTER
                // seleccionada opcion
                // de momento solo imprimimos
                // la seleccion en la ventana central
                content_add_line(center.content, section_get_selected_value(&menu));
                section_print(&center);
                return UI_ACTION_SELECT;
            case 'q':
            case 'Q':
                // Salimos
                ui_end();
                return UI_ACTION_QUIT;
            default:
                return UI_ACTION_NONE;
        }
        section_print(&menu);
        
    } else if (search.has_focus) { // Acciones en la ventana de busqueda
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

    } else if (center.has_focus) { // Acciones en la ventana central
        pressed_key = section_getch(&center);
        switch (pressed_key) {
            case KEY_UP:
                section_prev_option(&center);
                break;
            case KEY_DOWN:
                section_next_option(&center);
                break;
            case 9:
                //TAB
                ui_change_focus();
                return UI_ACTION_CHANGE_FOCUS;
            case 10:
                //ENTER
                // hay que reproducir la seleccion
                //
                // copiamos el texto y lo devolvemos en el return_value 
                // (Inutil porque desde main pediremos el content de center)
                strcpy(return_value, center.content->text[center.selected_line-1]);
                // pasamos playerui el estado para reflejarlo en la ui y preseleccionar el play
                // pero no cambiamos el foco, simplemente iluminamos el play
                playerui.selected_line = UI_PLAYER_PLAY;
                section_print(&playerui);
                return UI_ACTION_LOAD_PLAYLIST;
            case 'q':
            case 'Q':
                // Salimos
                ui_end();
                return UI_ACTION_QUIT;
            default:
                return UI_ACTION_NONE;
        }
        section_print(&center);
        return UI_ACTION_NONE;
    } else if (playerui.has_focus) {
        pressed_key = section_getch(&playerui);
        switch (pressed_key) {
            case KEY_LEFT:
                section_prev_option(&playerui);
                break;
            case KEY_RIGHT:
                section_next_option(&playerui);
                break;
            case 9:
                // TAB
                ui_change_focus();
                return UI_ACTION_CHANGE_FOCUS;
            case 10:
                // ENTER
                return playerui_get_selected_action();
                break;
            case 'q':
            case 'Q':
                ui_end();
                return UI_ACTION_QUIT;
            default:
                return UI_ACTION_NONE;
        }
        section_print(&playerui);
        return UI_ACTION_NONE;
    }
    return UI_ACTION_NONE;
}

/*************
 *  
 *  menu section functions
 *
 *************/
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
static int menu_create_window() {
    menu.name = "name";
    menu.height = 6;
    menu.width = 16;
    menu.starty = MARGIN;
    menu.starty = MARGIN;
    menu.win = newwin(menu.height, menu.width, menu.starty, menu.startx);

    if (menu.win == NULL) {
        return 0; // devolvemos fallo
    }
    box(menu.win, 0, 0);
    keypad(menu.win, TRUE);

    section_print(&menu);
    return 1; // devolvemos OK
}

/*************
 *
 * search section functions
 *
 *************/
static void search_create_content() {
     // Contenido 
    search.has_focus = false; // no tenemos el foco al inicial la app
    search.selected_line = 0; // en esta ventana no se usa el campo selected_line
    search.content = content_create(2); // inicializamos la memoria con 2 lineas de maximo.
}
static int search_create_window() {
    search.name = "search";
    search.height = 3;
    search.width = screen_width - (menu.width + (MARGIN*2));
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

/*************
 *
 * center section functions
 *
 *************/
static int center_create_window() {
    center.name = "center";
    center.height = screen_height - (search.height + MARGIN + 3);
    center.width = screen_width - (menu.width + (MARGIN*2));
    center.starty = search.height + (MARGIN);
    center.startx = menu.width + (MARGIN*2);

    center.win = newwin(center.height, center.width, center.starty, center.startx);

    if (center.win == NULL) {
        return 0;
    }

    box(center.win, 0, 0);

    section_print(&center);

    return 1;
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

/************
 *
 * playerui section functions
 *
 ***********/
static int playerui_create_window() {
    playerui.name = "playerui";
    playerui.height = screen_height - (search.height + center.height + MARGIN);
    playerui.width = 2;
    for (int i=0; i<playerui.content->numlines; i++) {
        playerui.width += strlen(playerui.content->text[i]) + 2;
    }
    playerui.starty = center.starty + center.height;
    playerui.startx = (center.width / 2) - (playerui.width / 2) + menu.width;

    playerui.win = newwin(playerui.height, playerui.width, playerui.starty, playerui.startx);

    if (playerui.win == NULL) {
        return 0;
    }

    playerui.has_focus = false;
    playerui.selected_line = 0;
    
    section_print(&playerui);

    return 1;
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
            if (center.selected_line < center.content->numlines) {
                section_next_option(&center);
                section_print(&center);
            }
            return UI_ACTION_FORWARD;
            break;
        default:
            return UI_ACTION_NONE;
            break;
    }
}

#include "ui.h"
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/*************
 *  HELPERS
 ************/
// Print content
void section_print(section_t *sec) {
    // borramos el contenido que hubiera en pantalla
    werase(sec->win);
    // caso especial para la ventana de busqueda
    // si tenemos el foco en search cambiamos el color 
    // del borde de la ventana, en cualquier otro caso
    // creamos el box estandard
    if (strcmp(sec->name, "search") == 0 && sec->has_focus) {
        wattron(sec->win, COLOR_PAIR(1));
        box(sec->win, 0, 0);
        wattroff(sec->win, COLOR_PAIR(1));
        sec->selected_line = 0;
    } else {
        box(sec->win, 0, 0);
    }
    // padding del texto
    int x = 2;
    int y = 1;
    // contador de lineas
    int i = 0;
    // si tenemos mas lineas de las que caben seteamos el inicio 
    // de forma que la ultima linea impresa sea la ultima 
    // (util si estamos bajando o imprimiendo mientras llegan datos
    // pero no es util cuando le pongamos un monton de datos en los 
    // que hay que navegar)
    if (sec->content.numlines > sec->height - 2) {
        i = sec->content.numlines - (sec->height - 2);
    }
    // imprimimos linea a linea y pintamos diferente 
    // la linea seleccionada
    for (; i<sec->content.numlines; ++i) {
        if (sec->selected_line == i+1) {
            wattron(sec->win, A_REVERSE);
            mvwprintw(sec->win, y, x, "%s", sec->content.text[i]);
            wattroff(sec->win, A_REVERSE);
        } else {
            mvwprintw(sec->win, y, x, "%s", sec->content.text[i]);
        }
        ++y;
    }
    // refrescamos la ventana
    wrefresh(sec->win);
}
// Get char from section
int section_getch(section_t *sec) {
    return wgetch(sec->win);
}
// Delete win
void section_delwin(section_t *sec) {
    if (sec->win != NULL) {
        delwin(sec->win);
    }
}
// Avanzar a la siguiente linea/opcion seleccionable
void section_next_option(section_t *sec) {
    if (sec->selected_line == sec->content.numlines) {
        sec->selected_line = 1;
    } else {
        ++sec->selected_line;
    }
}
// Retroceder a la opcion anterior
void section_prev_option(section_t *sec) {
    if (sec->selected_line == 1) {
        sec->selected_line = sec->content.numlines;
    } else {
        --sec->selected_line;
    }
}
// Fijar el foco en una seccion
void section_set_focus(section_t *sec) {
    // excepcion para la ventana de busqueda
    if (strcmp(sec->name, "search") == 0) {
        content_clear(&sec->content);
        content_init(&sec->content, 2);
    }
    // marcamos la variable a true
    sec->has_focus = true;
    // activamos la entrada del teclado en la ventana
    keypad(sec->win, TRUE);
    // seleccionamos la primera linea por defecto
    sec->selected_line = 1;
    // repintamos la pantalla
    section_print(sec);
}
// Quitamos el foco de la ventana
void section_unset_focus(section_t *sec) {
    // guardamos el estado actual
    sec->has_focus = false;
    // desactivamos la entrada por teclado
    keypad(sec->win, FALSE);
    // eliminamos la seleccion
    sec->selected_line = 0;
    // refrescamos la pantalla
    section_print(sec);
}
// Devolvemos el contenido de la linea seleccionada
const char* section_get_selected_value(section_t *sec) {
    return sec->content.text[sec->selected_line - 1];
}
/*************
 *
 *  MAIN NCURSES SCREEN AND SHARED
 *
 ************/
int screen_height, screen_width;

// Las secciones de la UI
section_t menu = {0};
section_t search = {0};
section_t center = {0};

// Inicializacion de ncurses y nuestras ventanas
bool ui_init() {
    // funciones de inicializacion de ncurses
    initscr();
    cbreak();
    noecho();
    curs_set(0);
    ui_start_colors();
    keypad(stdscr, FALSE);
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

    return true;
}
// Destruccion de la UI
void ui_end() {
    section_delwin(&menu);
    section_delwin(&search);
    section_delwin(&center);
    clrtoeol();
    endwin();
}
// Inicializacion de los pares de colores
void ui_start_colors() {
    start_color();
    init_pair(1, COLOR_CYAN, COLOR_BLACK);
}
// Cambio de foco de ventana. 
// Comportamiento al pulsar la tecla TAB
void ui_change_focus() {
    if (menu.has_focus) {
        section_unset_focus(&menu);
        content_add_line(&center.content, "Cambiamos foco a search");
        section_print(&center);
        section_set_focus(&search);
    } else if (search.has_focus) {
        section_unset_focus(&search);
        section_set_focus(&center);
    } else if (center.has_focus) {
        section_unset_focus(&center);
        section_set_focus(&menu);
    }
}
// Funcion que se ejecuta en el bucle principal y que espera 
// la accion del usuario. Una vez el usuario pulsa alguna tecla
// ejecutamos lo necesario y devolvemos la accion realizada
// excepto en el caso de la busqueda, que nos quedamos en un bucle
// hasta que el usuario pulse ENTER o TAB para salir del campo de busqueda
ui_action_t ui_handle_input(char *return_value) {
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
                content_add_line(&center.content, section_get_selected_value(&menu));
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
        // de texto. Tenemos que resolver pulsaciones no alfanumericas y
        // permitir borrar
        while (pressed_key != 10 && pressed_key != 9) {
            pressed_key = wgetch(search.win);
            content_add_char(&search.content, 0, pressed_key); // la añadimos a nuestro content
            section_print(&search);
        }
        // una vez fuera del bucle comprobamos si era ENTER o TAB
        if (pressed_key == 9) {
            content_add_line(&center.content, "No quieren buscar :(");
            section_print(&center);
            ui_change_focus();
            return UI_ACTION_CHANGE_FOCUS;

        } else if (pressed_key == 10) {
            content_add_line(&center.content, "Se pide una busqueda. Devolvemos UI_ACTION_SEARCH");
            section_print(&center);
            ui_change_focus();
            strcpy(return_value, search.content.text[0]);
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
                return UI_ACTION_SELECT;
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
    }
    return UI_ACTION_NONE;
}
/*************
 *  MENU WINDOW
 *************/

int menu_create_window() {
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

    // contenido
    menu.has_focus = true; // el foco al iniciar la app es en el menu
    menu.selected_line = 1; // preseleccionamos la primera linea
    content_init(&menu.content, 5); // inicializamos el contenido y su espacio en memoria
    content_add_line(&menu.content, "Home");
    content_add_line(&menu.content, "Explore");
    content_add_line(&menu.content, "Library");
    content_add_line(&menu.content, "Settings");
    
    section_print(&menu);
    return 1; // devolvemos OK
}

/*************
 * SEARCH WINDOW
 *************/
int search_create_window() {
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

    // Contenido 
    search.has_focus = false; // no tenemos el foco al inicial la app
    search.selected_line = 0; // en esta ventana no se usa el campo selected_line
    content_init(&search.content, 2); // inicializamos la memoria con 2 lineas de maximo.
    search_init_text(); // inicializamos el texto

    return 1;
}
void search_init_text() {
    content_add_line(&search.content, "Search ..."); // añadimos la linea al contenido.
    wattron(search.win, COLOR_PAIR(1)); // set color
    mvwprintw(search.win, 1, 1, "%s", search.content.text[search.content.numlines - 1]); // imprimimos la linea
    wattroff(search.win, COLOR_PAIR(1)); // unset color
    wrefresh(search.win);
}
void search_set_focus() {
    //Cambiamos foco a la caja de busqueda
    search.has_focus = true;
    content_clear(&search.content);
    content_init(&search.content, 2);
    keypad(search.win, TRUE);
    werase(search.win);
    wattron(search.win, COLOR_PAIR(1));
    box(search.win, 0, 0);
    wattroff(search.win, COLOR_PAIR(1));
    wrefresh(search.win);
}
/*************
 * CENTER WINDOW
 *************/
int center_create_window() {
    center.name = "center";
    center.height = screen_height - (search.height + MARGIN);
    center.width = screen_width - (menu.width + (MARGIN*2));
    center.starty = search.height + (MARGIN);
    center.startx = menu.width + (MARGIN*2);

    center.win = newwin(center.height, center.width, center.starty, center.startx);

    if (center.win == NULL) {
        return 0;
    }

    // contenido
    center.has_focus = false;
    center.selected_line = 0; // numero de linea seleccionada
    content_init(&center.content, center.height);

    box(center.win, 0, 0);

    content_add_line(&center.content, "Bienvenido a deecer <3");
    section_print(&center);

    return 1;
}
void center_set_content(content_t *content) {
    content_copy(&center.content, content);
    section_print(&center);
}

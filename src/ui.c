#include "ui.h"
#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/*************
 *  HELPERS
 ************/
// Init content array and its memory
void section_init_content(Section *sec, int content_size) {
    // se configura maxlines
    sec->maxlines = content_size;
    // se reserva el espacio para el numero maximo de lineas de tipo char*
    sec->content = malloc(sec->maxlines * sizeof(char*));
    // se ponen cada uno de esos espacios a NULL
    for (int i=0; i < sec->maxlines; i++) {
        sec->content[i] = NULL;  
    }
    // se inicializa nuestro contador de contenido.
    sec->numlines = 0;
    
}
// Add line to section line array - does not print anything, only to manage memory
void section_add_line(Section *sec, const char *texto) {
    // comprobamos si tenemos memoria suficiente y si no 
    // tenemos ampliamos al doble del tamaño actual.
    if (sec->numlines >= sec->maxlines) {
        sec->maxlines *= 2;
        sec->content = realloc(sec->content, sec->maxlines * sizeof(char*));
    }
    // copiamos el contenido de texto en el indice del ultimo contador
    // strdup reservará el espacio necesario en nuestro char*
    sec->content[sec->numlines] = strdup(texto);
    sec->numlines++;
}
// Add char to the current line
void section_add_char(Section *sec, int line_index, const char c) {
    // iniciamos el contador del indice del ultimo caracter en la linea
    // que vamos a editar
    int len = 0;
    // si la linea no está vacia entonces medimos su contenido
    if (sec->content[line_index] != NULL) {
        len = strlen(sec->content[line_index]);
    }
    // creamos el espacio de memoria para poder añadir caracteres
    sec->content[line_index] = realloc(sec->content[line_index], len + 2);
    // añadimos el caracter en el indice que toca
    sec->content[line_index][len] = c;
    // añadimos el carecter de terminacion del string
    sec->content[line_index][len+1] = '\0';
    // pasamos a la linea siguiente para estar preparado.
    sec->numlines = line_index + 1;
}
// Clear content and free memory
void section_clear(Section *sec) {
    // recorremos cada linea usada y liberamos su memoria
    for (int i = 0; i < sec->numlines+1; i++) {
        free(sec->content[i]);
    }
    // liberamos el espacio de content globalmente
    free(sec->content);
    // seteamos content a NULL
    sec->content = NULL;
    // reinciamos el contador de lineas escritas
    sec->numlines = 0;
    // seteamos a 0 el maximo de lineas
    sec->maxlines = 0;
}
// Print content
void section_print(Section *sec) {
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
    if (sec->numlines > sec->height - 2) {
        i = sec->numlines - (sec->height - 2);
    }
    // imprimimos linea a linea y pintamos diferente 
    // la linea seleccionada
    for (; i<sec->numlines; ++i) {
        if (sec->selected_line == i+1) {
            wattron(sec->win, A_REVERSE);
            mvwprintw(sec->win, y, x, "%s", sec->content[i]);
            wattroff(sec->win, A_REVERSE);
        } else {
            mvwprintw(sec->win, y, x, "%s", sec->content[i]);
        }
        ++y;
    }
    // refrescamos la ventana
    wrefresh(sec->win);
}
// Get char from section
int section_getch(Section *sec) {
    return wgetch(sec->win);
}
void section_delwin(Section *sec) {
    if (sec->win != NULL) {
        delwin(sec->win);
    }
}
void section_next_option(Section *sec) {
    if (sec->selected_line == sec->numlines) {
        sec->selected_line = 1;
    } else {
        ++sec->selected_line;
    }
}
void section_prev_option(Section *sec) {
    if (sec->selected_line == 1) {
        sec->selected_line = sec->numlines;
    } else {
        --sec->selected_line;
    }
}
void section_set_focus(Section *sec) {
    if (strcmp(sec->name, "search") == 0) {
        section_clear(sec);
        section_init_content(sec, 2);
    }
    sec->has_focus = true;
    keypad(sec->win, TRUE);
    sec->selected_line = 1;
    section_print(sec);
}
void section_unset_focus(Section *sec) {
    sec->has_focus = false;
    keypad(sec->win, FALSE);
    sec->selected_line = 0;
    section_print(sec);
}
const char* section_get_selected_value(Section *sec) {
    return sec->content[sec->selected_line - 1];
}
/*************
 *
 *  MAIN NCURSES SCREEN AND SHARED
 *
 ************/
int screen_height, screen_width;
/* struct way*/
Section menu = {0};
Section search = {0};
Section center = {0};

bool ui_init() {
    initscr();
    cbreak();
    noecho();
    curs_set(0);
    ui_start_colors();
    keypad(stdscr, FALSE);
    refresh();

    getmaxyx(stdscr, screen_height, screen_width);

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

void ui_end() {
    section_delwin(&menu);
    section_delwin(&search);
    section_delwin(&center);
    clrtoeol();
    endwin();
}
void ui_start_colors() {
    start_color();
    init_pair(1, COLOR_CYAN, COLOR_BLACK);
}
void ui_change_focus() {
    if (menu.has_focus) {
        section_unset_focus(&menu);
        section_add_line(&center, "Cambiamos foco a search");
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
ui_action_t ui_handle_input() {
    int pressed_key = 0;

    if (menu.has_focus) {
        pressed_key = section_getch(&menu);
        switch (pressed_key) {
            case KEY_UP:
                section_prev_option(&menu);
                break;
            case KEY_DOWN:
                section_next_option(&menu);
                break;
            case 9:
                ui_change_focus();
                return UI_ACTION_CHANGE_FOCUS;
                break;
            case 10:
                // seleccionada opcion
                // de momento solo imprimimos
                // la seleccion en la ventana central
                section_add_line(&center, section_get_selected_value(&menu));
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
        
    } else if (search.has_focus) {
        while (pressed_key != 10 && pressed_key != 9) {
            pressed_key = wgetch(search.win);
            section_add_char(&search, 0, pressed_key); // la añadimos a nuestro content
            section_print(&search);
        }
        if (pressed_key == 9) {
            section_add_line(&center, "No quieren buscar :(");
            section_print(&center);
            ui_change_focus();
            return UI_ACTION_CHANGE_FOCUS;

        } else if (pressed_key == 10) {
            section_add_line(&center, "Se pide una busqueda. Devolvemos UI_ACTION_SEARCH");
            section_print(&center);
            ui_change_focus();
            return UI_ACTION_SEARCH;
        }
        return UI_ACTION_NONE;

    } else if (center.has_focus) {
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
    section_init_content(&menu, 5); // inicializamos el contenido y su espacio en memoria
    section_add_line(&menu, "Home");
    section_add_line(&menu, "Explore");
    section_add_line(&menu, "Library");
    section_add_line(&menu, "Settings");
    
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
    section_init_content(&search, 2); // inicializamos la memoria con 2 lineas de maximo.
    search_init_text(); // inicializamos el texto

    return 1;
}
void search_init_text() {
    section_add_line(&search, "Search ..."); // añadimos la linea al contenido.
    wattron(search.win, COLOR_PAIR(1)); // set color
    mvwprintw(search.win, 1, 1, "%s", search.content[search.numlines - 1]); // imprimimos la linea
    wattroff(search.win, COLOR_PAIR(1)); // unset color
    wrefresh(search.win);
}
void search_set_focus() {
    //Cambiamos foco a la caja de busqueda
    search.has_focus = true;
    section_clear(&search);
    section_init_content(&search, 2);
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
    section_init_content(&center, center.height);

    box(center.win, 0, 0);

    section_add_line(&center, "Bienvenido a deecer <3");
    section_print(&center);

    return 1;
}
void center_set_content(char *content[], int numlines) {
    // liberamos nuestro anterior content
    section_clear(&center);
    //creamos el espacio para el nuevo contenido
    section_init_content(&center, numlines + 1);
    // recorremos el array recibido y lo copiamos en el nuestro
    for (int i=0; i<numlines; i++) {
        section_add_line(&center, content[i]);
    }
    section_print(&center);
}

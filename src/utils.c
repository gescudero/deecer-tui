#include "utils.h"
#include "deezer_api.h"
#include "models.h"
#include <sched.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

content_t* content_create(size_t content_size) {
    content_t *cont = malloc(sizeof(content_t));
    if (cont == NULL) {
        return NULL;
    }
    content_init(cont, content_size);
    return cont;
}

void content_init(content_t *cont, size_t content_size) {
    // forzamos a un espacio minimo de 1
    if (0 == content_size) {
        content_size = 1;
    }
    // se configura maxlines
    cont->maxlines = content_size;
    // se reserva el espacio para el numero maximo de lineas de tipo char*
    // reservamos tambien espacio para los posibles punteros a tracks
    // y a playlists
    cont->text = calloc(cont->maxlines, sizeof(char*));
    cont->tracks = calloc(cont->maxlines, sizeof(track_t*));
    cont->playlists = calloc(cont->maxlines, sizeof(playlist_t*));
    if (cont->text == NULL || cont->tracks == NULL || cont->playlists == NULL ) {
        return;
    }
    // se ponen cada uno de esos espacios a NULL
    // (Creo que esta funcion sobra desde que uso calloc en vez de malloc)
    for (int i=0; i < cont->maxlines; i++) {
        cont->text[i] = NULL;  
        cont->tracks[i] = NULL;
        cont->tracks[i] = NULL;
    }
    // se inicializa nuestro contador de contenido.
    cont->numlines = 0;
}
void content_add_line(content_t *cont, const char *texto){
    // comprobamos si tenemos memoria suficiente y si no 
    // tenemos ampliamos al doble del tamaño actual.
    // añadimos tambien espacio para un posible puntero a track
    if (cont->numlines >= cont->maxlines) {
        // creamos un puntero temporal por si nos falla realloc
        size_t old_max = cont->maxlines;
        size_t new_max = old_max == 0 ? 1 : old_max * 2;
        char **temp_text = realloc(cont->text, new_max * sizeof(char*));
        track_t **temp_tracks = realloc(cont->tracks, new_max * sizeof(track_t*));
        playlist_t **temp_playlists = realloc(cont->playlists, new_max * sizeof(playlist_t*));
        if ( temp_text == NULL || temp_tracks == NULL || temp_playlists == NULL) {
            return;
        }
        // si realloc ha podido reservar el espacio asignamos ese
        // puntero a nuestro puntero text y inicializamos a NULL los 
        // nuevos espacios
        cont->text = temp_text;
        cont->tracks = temp_tracks;
        cont->playlists = temp_playlists;
        for (int i = old_max; i < new_max; i++ )
        {
            cont->text[i] = NULL;
            cont->tracks[i] = NULL;
            cont->playlists[i] = NULL;
        }
        // Ahora ya podemos validar el nuevo numero de maxlines
        cont->maxlines = new_max;
    }
    // copiamos el contenido de texto en el indice del ultimo contador
    // strdup reservará el espacio necesario en nuestro char*
    cont->text[cont->numlines] = strdup(texto);
    cont->numlines++;
};
void content_add_char(content_t *cont, int line_index, const char c) {
    // Evitamos escribir algunos caracteres no imprimibles
    // evitamos el 7 porque lo necesitamos luego (es backspace)
    if (32 > c && c != 7) {
        return;
    }
    // comprobamos que line_index no supere maxlines, si es asi
    // debemos hacer espacio primero,
    if (line_index >= cont->maxlines) {
        // Creamos ese nuevo espacio apuntando a un puntero temporal
        // de esta forma, si falla no perdemos nuestro puntero OG
        size_t prev_max = cont->maxlines;
        size_t new_max = line_index + 1;
        char **temp_text = realloc(cont->text, new_max * sizeof(char*));
        if (temp_text == NULL) {
            return;
        }
        // asignamos la nueva direccion de memoria
        cont->text = temp_text;
        // limpiamos a NULL los nuevos espacios
        for (int i = prev_max; i < new_max; i++) {
            cont->text[i] = NULL;
        }

        // ya podemos asignar el nuevo maximo
        cont->maxlines = new_max;
    }

    // iniciamos el contador del indice del ultimo caracter en la linea
    // que vamos a editar
    int len = 0;
    // si la linea no está vacia entonces medimos su contenido
    if (cont->text[line_index] != NULL) {
        len = strlen(cont->text[line_index]);
    }
    // Creamos un puntero temporal para alojar la direccion
    // que nos devuelve realloc
    char *temp_line = realloc(cont->text[line_index], len + 2);
    
    if (temp_line == NULL) {
        return;
    }
     // creamos el espacio de memoria para poder añadir caracteres
    cont->text[line_index] = temp_line;
   
    // comprobamos que no hayan pulsado backspace o delete, si es asi
    // seteamos el final de la cadena en el caracter anterior a len 
    // y retornamos (usamos supr igual que backspace)
    if (c == 7 || c == 74) {
        // restamos dos a len, de esa forma quitamos un caracter
        // en el momento que ejecutamos la siguiente linea despues
        // de este if
        len -= 2;
    } else {
        // añadimos el caracter en el indice que toca
        cont->text[line_index][len] = c;
    }

    // añadimos el caracter de terminacion del string
    cont->text[line_index][len+1] = '\0';
    // pasamos a la linea siguiente para estar preparado.
    cont->numlines = line_index + 1;

};
void content_add_track(content_t *cont, track_t *track) {
    // guardamos la linea donde queremos insertar el track
    int index = cont->numlines;
    // Solo añadimos contenido si nos han pasado un track valido
    if (deezer_track_is_valid(track)) {
        char *tmp_text;
        asprintf(&tmp_text, "%s (%s)", track->title, track->artist[0]->name) ;
        content_add_line(cont, tmp_text);
        cont->tracks[index] = track;
        free(tmp_text);
   }
}
int content_add_playlist(content_t *cont, playlist_t *playlist) {
    // guardamos la linea donde queremos insertar la playlist
    int index = cont->numlines; 
    if (deezer_playlist_is_valid(playlist)) {
        char *tmp_text;
        asprintf(&tmp_text, "%s", playlist->title);
        content_add_line(cont, tmp_text);
        cont->playlists[index] = playlist;
        free(tmp_text);
        return 0;
    }
    return 1;
}
int content_add_playlist_in_row(content_t *cont, playlist_t *playlist, int line_index) {
    if (line_index >= cont->numlines) {
        return 1;
    }
    if (deezer_playlist_is_valid(playlist)) {
        cont->playlists[line_index] = playlist;
    }
    return 0;
}
bool content_line_is_track(const content_t *cont, int line_index) {
    if (line_index >= cont->maxlines) {
        return false;
    }
    if (cont->tracks == NULL) {
        return false;
    }
    if (cont->tracks[line_index] == NULL) {
        return false;
    }
    if (!deezer_track_is_valid(cont->tracks[line_index])) {
        return false;
    }
    return true;
}
bool content_line_is_playlist(const content_t *cont, int line_index) {
    if (line_index >= cont->maxlines) {
        return false;
    }
    if (cont->playlists == NULL) {
        return false;
    }
    if (cont->playlists[line_index] == NULL) {
        return false;
    }
    if (!deezer_playlist_is_valid(cont->playlists[line_index])) {
        return false;
    }
    return true;
}

// añadimos todo el contenido de addition a la 
// estructura destino
void content_add(content_t *dest, const content_t *addition) {
    // copiamos linea a linea
    for (int i= 0; i<addition->numlines; i++) {
        if (content_line_is_track(addition, i)) {
            content_add_track(dest, addition->tracks[i]);
        } else {
            content_add_line(dest, addition->text[i]);
        }
    }
}

void content_clear(content_t *cont) {
    // si el puntero es null evitamos free porque lo hace petar
    if (cont == NULL) {
        return;
    }
    // lo mismo. si el texto es null, no free
    if (cont->text == NULL) {
        cont->numlines = 0;
        cont->maxlines = 0;
        return;
    }
    // recorremos cada linea usada y liberamos su memoria
    // si es necesario
    for (int i = 0; i < cont->numlines; i++) {
        if (cont->text[i] != NULL) {
            free(cont->text[i]);
            cont->text[i] = NULL;
        }
    }
    // liberamos el espacio de content->text
    if (cont->text != NULL) {
        free(cont->text);
    }
    // liberamos la lista de tracks, pero no los tracks,
    // ya que no nos pertenecen (al menos esa es mi intención)
    if (cont->tracks != NULL) {
        free(cont->tracks);
    }
    // seteamos content->text a NULL
    cont->text = NULL;
    cont->tracks = NULL;
    // reinciamos el contador de lineas escritas
    cont->numlines = 0;
    // seteamos a 0 el maximo de lineas
    cont->maxlines = 0;
};

void content_free(content_t *cont) {
    if (cont == NULL) {
        return;
    }
    content_clear(cont);
    free(cont);
}

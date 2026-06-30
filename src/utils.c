#include "utils.h"
#include <stddef.h>
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
    cont->text = malloc(cont->maxlines * sizeof(char*));
    if (cont->text == NULL) {
        return;
    }
    // se ponen cada uno de esos espacios a NULL
    for (int i=0; i < cont->maxlines; i++) {
        cont->text[i] = NULL;  
    }
    // se inicializa nuestro contador de contenido.
    cont->numlines = 0;
}
void content_add_line(content_t *cont, const char *texto){
    // comprobamos si tenemos memoria suficiente y si no 
    // tenemos ampliamos al doble del tamaño actual.
    if (cont->numlines >= cont->maxlines) {
        // creamos un puntero temporal por si nos falla realloc
        size_t old_max = cont->maxlines;
        size_t new_max = old_max == 0 ? 1 : old_max * 2;
        char **temp_text = realloc(cont->text, new_max * sizeof(char*));
        if ( temp_text == NULL) {
            return;
        }
        // si realloc ha podido reservar el espacio asignamos ese
        // puntero a nuestro puntero text y inicializamos a NULL los 
        // nuevos espacios
        cont->text = temp_text;
        for (int i = old_max; i < new_max; i++ )
        {
            cont->text[i] = NULL;
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
    // de momento filtramos \n y \t
    if (c == 10 || c == 13 || c == 9) {
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
    // añadimos el caracter en el indice que toca
    cont->text[line_index][len] = c;
    // añadimos el carecter de terminacion del string
    cont->text[line_index][len+1] = '\0';
    // pasamos a la linea siguiente para estar preparado.
    cont->numlines = line_index + 1;

};
// substituimos el contenido de nuestra estructura destino por 
// el contenido de la estructura origen
void content_copy(content_t *dest, const content_t *origin) {
    // liberamos del anterior contenido
    content_clear(dest);
    // inicializamos con el tamaño del nuevo
    content_init(dest, origin->numlines);
    // le añadimos el contenido
    content_add(dest, origin);
}
// añadimos todo el contenido de addition a la 
// estructura destino
void content_add(content_t *dest, const content_t *addition) {
    // copiamos linea a linea
    for (int i= 0; i<addition->numlines; i++) {
        content_add_line(dest, addition->text[i]);
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
    free(cont->text);
    // seteamos content->text a NULL
    cont->text = NULL;
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

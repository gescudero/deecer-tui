#include "utils.h"
#include <stdlib.h>

void content_init(content_t *cont, size_t content_size) {
    // se configura maxlines
    cont->maxlines = content_size;
    // se reserva el espacio para el numero maximo de lineas de tipo char*
    cont->text = malloc(cont->maxlines * sizeof(char*));
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
        cont->maxlines *= 2;
        cont->text = realloc(cont->text, cont->maxlines * sizeof(char*));
    }
    // copiamos el contenido de texto en el indice del ultimo contador
    // strdup reservará el espacio necesario en nuestro char*
    cont->text[cont->numlines] = strdup(texto);
    cont->numlines++;
};
void content_add_char(content_t *cont, int line_index, const char c) {
    // iniciamos el contador del indice del ultimo caracter en la linea
    // que vamos a editar
    int len = 0;
    // si la linea no está vacia entonces medimos su contenido
    if (cont->text[line_index] != NULL) {
        len = strlen(cont->text[line_index]);
    }
    // creamos el espacio de memoria para poder añadir caracteres
    cont->text[line_index] = realloc(cont->text[line_index], len + 2);
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
    // recorremos cada linea usada y liberamos su memoria
    for (int i = 0; i < cont->numlines+1; i++) {
        free(cont->text[i]);
    }
    // liberamos el espacio de content globalmente
    free(cont->text);
    // seteamos content a NULL
    cont->text = NULL;
    // reinciamos el contador de lineas escritas
    cont->numlines = 0;
    // seteamos a 0 el maximo de lineas
    cont->maxlines = 0;
};

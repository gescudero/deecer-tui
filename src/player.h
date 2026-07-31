// player.h 

#ifndef PLAYER_H
#define PLAYER_H

#include <stdbool.h>

extern int player_running;

/**
 * Inicializa el handle mpv. Sera el mismo
 * objeto durante toda la vida de la app
 */
bool player_init();

/**
 * Libera los recursos de mpv. Solo se llama 
 * al cerrar la app
 */
void player_end();

/**
 * Abre un fichero o url para reproducir y 
 * comienza su reproduccion.
 *
 * @param url: url al stream o path al fichero.
 */
void player_openurl(char *url);

/**
 * Reproduce una lista de ficheros o de urls.
 * Necesita un fichero de texto, en el que cada linea tiene
 * una ruta a un fichero o una url a un stream.
 * Notifica a la ui cada vez que cambia de cancion
 *
 * @param file_path: La ruta al fichero de texto que contiene la lista.
 */
void player_openplaylist(char *file_path);

/**
 * Pulsacion del boton de play.
 * Solo tiene uso si estamos en pausa
 * para volver a la reproduccion
 */
void player_play(); 

/**
 * Orden de pausa
 */
void player_pause();

/**
 * Paro de la reproduccion y salida del bucle de eventos
 * Se usa tanto para acabar la reproduccion, como cuando
 * salimos de la aplicacion antes de llamar a player_end()
 */
void player_stop();

/**
 * Cuando estamos reproduciendo una playlist reproduce la
 * cancion anterior
 */
void player_back();

/**
 * Cuando estamos reproduciendo una playlist reproduce la
 * cancion siguiente.
 */
void player_forward();

void player_shuffle();

void player_unshuffle();

#endif

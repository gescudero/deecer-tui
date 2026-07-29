//utils.h
#ifndef UTILS_H
#define UTILS_H
#include "models.h"
#include <stdbool.h>
#include <string.h>

#define LOG(msg, ...) { \
  fprintf(stderr, "[%s][Line %d] ", __FILE__, __LINE__); \
  fprintf(stderr, (msg), ##__VA_ARGS__); \
}
/**
 * Estructura basica para textos
 * con control de lineas
 * puede contener track en cada linea individualmente
 * también podrá contener playlist, artists o albums
 * Es el tipo de objetos que maneja la ui 
 */
struct content_t {
    char **text;
    size_t numlines;
    size_t maxlines;
    track_t **tracks;
    playlist_t **playlists;
};

/**
 * Initializes a content object, allocating memory
 *
 * @param content_size: the number of lines to allocate
 * @return: a content_t initialized object
 */
content_t* content_create(size_t content_size);

/**
 * Initializes a content_t object
 *
 * It is used by search section in ui.c to clean in
 * each time it got focus
 *
 */
void content_init(content_t *cont, size_t content_size);

/**
 * Write one line in content object
 * It copies the string text into the next free line
 * It increase in one the numlines
 * It increase maxlines if is needed with more mem allocation
 *
 * @param cont: the content_t to add line
 * @param texto: the text to add
 */
void content_add_line(content_t *cont, const char *texto);

/**
 * Write one char in the specified line
 *
 * @param cont: the content_t to write the char
 * @param line_index: the line number (base 0)
 * @param c: the char to add
 */
void content_add_char(content_t *cont, int line_index, const char c);

/**
 * Add one track pointer to content in the first free line
 * also writes text in that line with track_name (artist_name)
 *
 * @param cont: the content_t to add the track
 * @param track: the track 
 */
void content_add_track(content_t *cont, track_t *track);

/**
 * Add one playlist pointer to content in the first free line
 * also writes text in that line with playlist_name or [ Play PLaylist]
 * if is an auto-playlist
 *
 * @param cont: the content_t to add the playlist
 * @param playlist: the playlist
 */
int content_add_playlist(content_t *cont, playlist_t *playlist);

/**
 * Same as content_add_playlist but can decide in which line to add the playlist
 * You have the responsabity to check if that line is occupied
 * Does not add text at all, only the playlist to the list of playlists
 */
int content_add_playlist_in_row(content_t *cont, playlist_t *playlist, int line_index);

/**
 * Check if the specified line has a valid track in it
 * Returns a boolean
 */
bool content_line_is_track(const content_t *cont, int line_index);

/**
 * Check if the specified line has a valid playlist in it 
 * Returns a boolean
 */
bool content_line_is_playlist(const content_t *cont, int line_index);

/**
 * Add all the lines of a content_t starting in the first free line of destination
 * content_t. 
 */
void content_add(content_t *dest, const content_t *addition);

/**
 * Clear a content and fills with the tracks of a playlist_t
 *
*/ 
void content_fill_with_playlists(content_t *dest, unsigned long playlist_id);

/**
 * Clear content of a content_t and frees memory of it contents 
 * but not the object it self. Can be used to clean its content and
 * reset content_t. After clear you have to call content_init() again
 * to start to fill with content again
 */
void content_clear(content_t *cont);

/**
 * Frees completely the content_t and its content
 */
void content_free(content_t *cont);

/**
 * Helper to remove extension from a filename string
 * changing the dot with '\0'
 */
void remove_extension(char *filename);
#endif

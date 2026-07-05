//deezer_api.h
#ifndef DEEZER_API_H
#define DEEZER_API_H
#include "models.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <curl/curl.h>
 // LIBCURL struct for receiving response
struct memory {
    char *memory;
    size_t size;
};
// DEEZER OBJECTS
struct track_t {
    int id; // id
    bool readable; // is readable on device
    char *title; // track title
    char *title_short; // track short title
    artist_t *artist; // Artist object
    album_t *album; // Album object
    char *preview; // link to preview [url]
    char *track_token; // token for media service
};

struct album_t {
    int id; // id
    char *title; // title of album
    char *md5_image; // id de la imagen (se pueden construir las url de descarga)
    char *tracklist; // enlace a la lista de tracks https://api.deezer.com/album/<id>/tracks
};

struct artist_t {
    int id; // id
    char *name; // name
    char *link; // https://www.deezer.com/artist/<id>
    char *tracklist; //api link to top of this artist [url]
                     //https://api.deezer.com/artist/<id>/top?limit=50
};

struct playlist_t {
    int id; // id
    char *title; // title 
    char *description; // descripcion
    char *link; // link en deezer 
    int nb_tracks; // numero de tracks en la playlist
    track_t **tracks; // array de punteros a tracks;
};

void deezer_init();
void deezer_cleanup();
content_t* deezer_search(const char *query);
char* deezer_get_album(int id);
char* deezer_get_artist(int id);
char* deezer_get_chart(int id);
char* deezer_get_playlist(int id);
char* deezer_get_track(int id);
char* deezer_get_user();
bool deezer_track_is_valid(track_t *track);
bool deezer_artist_is_valid(artist_t *artist);
bool deezer_album_is_valid(album_t *album); 
void deezer_track_free(track_t *track);
void deezer_artist_free(artist_t *artist);
void deezer_album_free(album_t *album);

#endif

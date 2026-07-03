//deezer_api.h
#ifndef DEEZER_API_H
#define DEEZER_API_H
#include "models.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <curl/curl.h>
/****
 *  LIBCURL
 ***/
struct memory {
    char *memory;
    size_t size;
};

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



void deezer_init();
void deezer_cleanup();
content_t* deezer_search(const char *query);
char* deezer_get_album(int id);
char* deezer_get_artist(int id);
char* deezer_get_chart(int id);
char* deezer_get_playlist(int id);
char* deezer_get_track(int id);
char* deezer_get_user();

#endif

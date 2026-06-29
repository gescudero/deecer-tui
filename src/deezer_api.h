#ifndef DEEZER_API_H
#define DEEZER_API_H
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

/****
 * DEEZER MODELS
 ****/
typedef struct track_t track_t;
typedef struct album_t album_t;
typedef struct artist_t artist_t;

struct track_t {
    int id; // id
    bool readable; // is readable on device
    char *title; // track title
    char *title_short; // track short title
    artist_t *artist; // Artist object
    album_t *album; // Album object
    char *preview; // link to preview [url]
};

struct album_t {
    int id; // id
    char *title; // title of album
    int nb_tracks; // number of tracks
    artist_t *artist; // artist object
    track_t **tracks; // list of track objects
};

struct artist_t {
    int id; // id
    char *name; // name
    char *link; // link to deezer [url]
    char *tracklist; //api link to top of this artist [url]
};

void deezer_init();
void deezer_cleanup();
char* deezer_search(const char *query);
char* deezer_get_album(int id);
char* deezer_get_artist(int id);
char* deezer_get_chart(int id);
char* deezer_get_playlist(int id);
char* deezer_get_track(int id);
char* deezer_get_user();

#endif

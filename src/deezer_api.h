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
typedef struct Track Track;
typedef struct Album Album;
typedef struct Artist Artist;

struct Track {
    int id; // id
    bool readable; // is readable on device
    char *title; // track title
    char *title_short; // track short title
    Artist *artist; // Artist object
    Album *album; // Album object
    char *preview; // link to preview [url]
};

struct Album {
    int id; // id
    char *title; // title of album
    int nb_tracks; // number of tracks
    Artist *artist; // artist object
    Track **tracks; // list of track objects
};

struct Artist {
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

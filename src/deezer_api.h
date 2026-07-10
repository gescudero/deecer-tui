//deezer_priv_api.h
/******************
 *
 * API privada de deezer 
 * Necesitamos arl para usarla
 *
 ******************/

#ifndef DEEZER_API_H
#define DEEZER_API_H
#include "models.h"
#include <stdbool.h>
#include <time.h>
#include <curl/curl.h>

struct memory_t {
    char * memory;
    size_t size;
};

struct deezer_client_t {
    CURL *curl_handle; // the curl object
    CURLcode curl_res; // response codes for curl 
    memory_t mem; // dataholder for curl requests
    char *arl;//from config file
    char *session_id;//results:SESSION_ID 
    char *api_token;//results:checkform
    char *license_token; //results:USER:OPTIONS:license_token
};

struct user_t {
    int id; //results:USER:USER_ID
    char *name;//results:USER:BLOG_NAME
    char *email;//results:USER:EMAIL
    char *lovedtracks_id;//results:USER:LOVEDTRACKS_ID 
    char *user_token;//results:USER_TOKEN 
};
struct track_t {
    int id; //results:DATA:SNG_ID
    char *title; //results:DATA:SNG_TITLE
    char *token; //results:TRACK_TOKEN
    time_t *token_expire; //results:TRACK_TOKEN_EXPIRE
    artist_t **artist; //results:ARTISTS ->
    int nb_artists; // number of artists 
    album_t *album; //results:ALB_ID ->
    char *media_url; // peticion a -> DEEZER_MEDIA_URL
};
struct artist_t {
    int id;
    char *name;
    track_t **tops;
    album_t **albums;
};
struct album_t {
    int id;
    char *title;
    artist_t **artists;
    int nb_artists;
    track_t **tracks;
    int nb_tracks;

};
struct playlist_t {
    int id;
    char *title;
    user_t *user;
    int nb_tracks;
    track_t **tracks;
};

// Funcion de inicializacion
// se encarga de crear todo lo necesario
// para arrancar el cliente y el pool de objetos
// solo se debe ejecutar una vez
int deezer_init(config_t *config);

// constructores
int deezer_create_client();
int deezer_create_user();
track_t *deezer_create_track();
artist_t *deezer_create_artist();
album_t *deezer_create_album();
playlist_t *deezer_create_playlist();

// utilities
content_t *deezer_search(const char *query);

// getters (pide el objeto a la pool, y si no existe
// lo pedira a la api)
user_t *deezer_get_user(int id);
track_t *deezer_get_track(int id);
artist_t *deezer_get_artist(int id);
album_t *deezer_get_album(int id);
playlist_t *deezer_get_playlist(int id);

// comprobadores
bool deezer_track_is_valid(track_t *track);
bool deezer_artist_is_valid(artist_t *artist);
bool deezer_album_is_valid(album_t *album);
bool deezer_playlist_is_valid(playlist_t *playlist);

// destructores
void deezer_cleanup();
void deezer_free_client(deezer_client_t *client);
void deezer_free_user(user_t *user);
void deezer_free_track(track_t *track);
void deezer_free_artist(artist_t *artist);
void deezer_free_album(album_t *album);
void deezer_free_playlist(playlist_t *playlist);

#endif

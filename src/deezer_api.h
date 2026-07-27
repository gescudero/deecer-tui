//deezer_api.h

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
    time_t token_expire; //results:TRACK_TOKEN_EXPIRE
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

/**
 * Funcion de inicializacion
 * se encarga de crear todo lo necesario
 * para arrancar el cliente y el pool de objetos
 * solo se debe ejecutar una vez
 *
 * @param configuracion
 * @return error code
 */
int deezer_init(config_t *config);

/**
 * Busqueda en deezer de un termino.
 * Devuelve una lista de tracks en formato
 * content_t 
 *
 * @param el texto de busqueda 
 * @return content_t con la lista de tracks lista para
 * imprimir en la ventana center de la ui 
 */
content_t *deezer_search(const char *query);

/**
 * Se solicita el path hasta el fichero de audio
 * de un track en concreto. La funcion descarga el fichero
 * encriptado, lo desencripta y escribe en filename la ruta
 * Se comprueba si ya existe en disco, y si es asi se evita
 * el proceso.
 *
 * @param el struct track con los datos de la cancion que queremos
 * @param un puntero a string donde escribir el nombre de fichero
 * @result error code 
 */
int deezer_get_media(track_t *track, char **filename);

/**
 * Se solicita solo el path al fichero de audio
 * No se descarga, simplemente devuelve el string
 * Es útil para generar la playlist con la lista de ficheros
 * sin tener que esperar a que descargue
 */
char *deezer_get_filepath(track_t *track);

// getters (pide el objeto a la pool, y si no existe
// lo pedira a la api)
user_t *deezer_get_user(int id);
track_t *deezer_get_track(int id);
artist_t *deezer_get_artist(int id);
album_t *deezer_get_album(int id);
playlist_t *deezer_get_playlist(int id);

/**
 * Comprueba que el arl pasado por parametro sea un 
 * token arl válido, o al menos lo parezca
 *
 * @param el texto con el arl a comprobar 
 * @return bool 
 */
bool deezer_arl_is_valid(const char *arl);

/**
 * Comprueba que el track pasado por parametro sea un 
 * track válido, o al menos lo parezca
 *
 * @param el objeto track a comprobar 
 * @return bool 
 */
bool deezer_track_is_valid(track_t *track);

/**
 * Comprueba que el artist pasado por parametro sea un 
 * artist válido, o al menos lo parezca
 *
 * @param el objeto artist a comprobar 
 * @return bool 
 */
bool deezer_artist_is_valid(artist_t *artist);

/**
 * Comprueba que el album pasado por parametro sea un 
 * token arl válido, o al menos lo parezca
 *
 * @param el objeto album a comprobar 
 * @return bool 
 */
bool deezer_album_is_valid(album_t *album);

/**
 * Comprueba que la playlist pasado por parametro sea una 
 * playlist válida, o al menos lo parezca
 *
 * @param la playlist a comprobar 
 * @return bool 
 */
bool deezer_playlist_is_valid(playlist_t *playlist);

/**
 * Funcion de limpieza del cliente de deezer
 * incluyendo curl y la pool de tracks, albums, etc
 * también intentará borrar los ficheros que hayan quedado
 * descargados (se podrá evitar este comportamiento por configuracion)
 */
void deezer_cleanup();


#endif

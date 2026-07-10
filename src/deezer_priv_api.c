//deezer_priv_api.c
/******************
 *
 * API privada de deezer,
 * implementacion
 *
 ******************/

#include "deezer_priv_api.h"
#include "models.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <cjson/cJSON.h>
#include <curl/easy.h>
#include <curl/typecheck-gcc.h>
/*****************
*
* private vars
*
*****************/
static const char *api_url = "https://www.deezer.com/ajax/gw-light.php";
static const char *media_url = "https://media.deezer.com/v1/get_url";
static deezer_client_t *client; // deezer handle
static user_t *user;
static int nb_tracks = 0; // numero de tracks en la pool
static track_t **tracks = NULL; // lista de tracks
static int nb_artists = 0;
static artist_t **artists = NULL;
static int nb_albums = 0;
static album_t **albums = NULL;
static int nb_playlists = 0;
static playlist_t **playlists = NULL;

/******************
 *
 * private functions
 *
 ******************/
// direct request to api
static int deezer_get_user_data(user_t *user);
static int deezer_get_track_data(track_t *track, int id);
static int deezer_get_artist_data(artist_t *artist, int id);
static int deezer_get_album_data(album_t *album, int id);
static int deezer_get_playlist_data(playlist_t *playlist, int id);

// libcurl
static size_t writecallback(char *contents, size_t size, size_t nmemb, void *userp);

// cJSON
static track_t* deezer_convert_json_to_track(cJSON *json_track);
static artist_t* deezer_convert_json_to_artist(cJSON *json_artist);
static album_t* deezer_convert_json_to_album(cJSON *json_album);

/*******************
 *
 * Definitions
 *
 ******************/
// inicializador
int deezer_init(config_t *config) {
    int err_code = 0;
    // creamos el objeto para el cliente
    err_code = deezer_create_client(client);
    if (DC_SUCCESS != err_code) {
        return err_code;
    }
    // inicializamos curl
    curl_global_init(CURL_GLOBAL_ALL);
    client->curl_handle = curl_easy_init();
    
    client->arl = config->arl;
    // inicializamos user
    err_code = deezer_create_user(user);
    if (DC_SUCCESS != err_code) {
        return err_code;
    }

    return DC_SUCCESS;
}
// constructores
int deezer_create_client(deezer_client_t *clt)  {
    clt = calloc(1, sizeof(deezer_client_t));
    if (clt == NULL) {
        return DC_ERROR_INICIALIZATION_FAILED;
    }
    return DC_SUCCESS;
}
int deezer_create_user(user_t *usr) {
    // reservamos la memoria para el usuria
    usr = calloc(1, sizeof(user_t));
    if (usr == NULL) {
        return DC_ERROR_INICIALIZATION_FAILED;
    }
    // comprobamos que tengamos el handle de curl
    if (client->curl_handle == NULL) {
        return DC_ERROR_INICIALIZATION_FAILED;
    }
    // construimos la url
    char *url = NULL;
    asprintf(&url, api_url, "?method=deezer.getUserData&api_version=1.0&api_token");
    // creamos las opciones de curl
    curl_easy_setopt(client->curl_handle, CURLOPT_URL, url);
    curl_easy_setopt(client->curl_handle, CURLOPT_WRITEFUNCTION, writecallback);
    curl_easy_setopt(client->curl_handle, CURLOPT_WRITEDATA, &client->mem);
    // liberamos url que ya no lo necesitamos
    free(url);
    //ejecutamos la request
    client->curl_res = curl_easy_perform(client->curl_handle);

    if (client->curl_res != CURLE_OK) {
        // algo ha fallado si se quiere comprobar,
        // el propio objeto tiene acceso a la respuesta
        // en client->curl_res
        return DC_ERROR_CURL_RESPONSE_ERROR;
    } else {
        // obtenemos la info del contenido obtenido en el anterior request
        char *contenttype;
        client->curl_res = curl_easy_getinfo(client->curl_handle, CURLINFO_CONTENT_TYPE, &contenttype);
        if ((CURLE_OK == client->curl_res) && contenttype) {
            // comprobamos que hayamos recibido un json
            if (strstr(contenttype, "application/json")) {
                //liberamos contenttype, ya no lo necesitamos
                free(contenttype);

                cJSON *json = cJSON_Parse(client->mem.memory);
                if (json == NULL || cJSON_IsInvalid(json) || !cJSON_IsObject(json)) {
                    //liberamos el objeto json y salimos (petará si es null??)
                    //quiza deban estar separados los casos
                    cJSON_Delete(json);
                    return DC_ERROR_UNKNOWN;
                } else {
                    // aqui es donde extraemos la info del json. Esperamos un objeto
                    // que es el que contiene todo lo que nos interesa
                    cJSON *results = cJSON_GetObjectItem(json, "results");
                    // el objeto USER contiene cosas que necesitamos para el user
                    // como el ID, el nombre o el email
                    cJSON *r_user = cJSON_GetObjectItem(results, "USER");
                    cJSON *u_userid = cJSON_GetObjectItem(r_user, "USER_ID");
                    cJSON *u_name = cJSON_GetObjectItem(r_user, "BLOG_NAME");
                    cJSON *u_mail = cJSON_GetObjectItem(r_user, "EMAIL");
                    cJSON *u_loved = cJSON_GetObjectItem(r_user, "LOVEDTRACKS_ID");
                    user->id = u_userid->valueint;
                    user->name = strdup(u_name->valuestring);
                    user->email = strdup(u_mail->valuestring);
                    user->lovedtracks_id = strdup(u_loved->valuestring);
                    // extraemos el license token de las opciones del usuario
                    cJSON *u_options = cJSON_GetObjectItem(r_user, "OPTIONS");
                    cJSON *o_license = cJSON_GetObjectItem(u_options, "license_token");
                    client->license_token = strdup(o_license->valuestring);
                    // ahora sacamos datos que no estan dentro de user pero
                    // necesitamos tanto para user como client
                    cJSON *r_utoken = cJSON_GetObjectItem(results, "USER_TOKEN");
                    cJSON *r_sessionid = cJSON_GetObjectItem(results, "SESSION_ID");
                    cJSON *r_apitoken = cJSON_GetObjectItem(results, "checkForm");
                    user->user_token = strdup(r_utoken->valuestring);
                    client->session_id = strdup(r_sessionid->valuestring);
                    client->api_token = strdup(r_apitoken->valuestring);
                }
                // liberamos el objeto json
                cJSON_Delete(json);
            }
        }
    }
    return DC_SUCCESS;
}
track_t *deezer_create_track();
artist_t *deezer_create_artist();
album_t *deezer_create_album();
playlist_t *deezer_create_playlist();

// JSON conversions



// public actions
content_t *deezer_search(const char *query);

static size_t writecallback(char *contents, size_t size, size_t nmemb, void *userp) {
    /*
     * realsize is alwais size * nmemb
     * the data is in contents
     * userp is what CURLOPT_WRITEDATA set, 
     * can be the structure or object you want
     *
     * return realsize
     */
    size_t realsize = size * nmemb;
    memory_t *mem = (memory_t *)userp;

    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if (ptr == NULL){
        return 0;
    }
    mem->memory = ptr;
    memcpy(&mem->memory[mem->size], contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size]=0;
    return realsize;
}

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
void deezer_free_client(deezer_client_t *client);
void deezer_free_user(user_t *user);
void deezer_free_track(track_t *track);
void deezer_free_artist(artist_t *artist);
void deezer_free_album(album_t *album);
void deezer_free_playlist(playlist_t *playlist);


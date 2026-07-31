//deezer_api.c

#include "deezer_api.h"
#include "models.h"
#include "config.h"
#include "content.h"
#include "utils.h"
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <curl/curl.h>
#include <cjson/cJSON.h>
#include <curl/easy.h>
#include <curl/typecheck-gcc.h>

/****************
 * Tipos de solicitudes a la api de deezer
 ****************/
enum deezer_requests {
    DC_GET_TOKEN = 1,
    DC_PAGE_SEARCH = 2,
    DC_PAGE_PLAYLISTS = 3,
    DC_PAGE_ALBUM = 4,
    DC_PAGE_ARTIST = 5,
    DC_PAGE_HOME = 6,
    DC_PAGE_TRACK = 7,
    DC_PAGE_PROFILE = 8,
    DC_PAGE_MEDIA_GET_URL = 9,
    DC_PAGE_MEDIA_GET_URL_MULTI = 10,
};

/* ============================================================================
 * DECLARACIONES DE LA BIBLIOTECA RUST
 * ============================================================================ */

// Estas funciones están en la biblioteca Rust
// La biblioteca se encuentra en: lib/libdeezer_crypto.a 
// y en la misma carpeta se puede encontrar también el código
extern unsigned char* decrypt_audio(const char* track_id, 
                                    const unsigned char* data, 
                                    size_t data_len, 
                                    size_t* out_len);
extern void free_decrypted(unsigned char* ptr, size_t len);

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

/**
 * Init deezer client. Make a request to get token 
 *
 * @param configuration struct 
 * @return error code 
 */
static int deezer_create_client(); 

/***
 * Create the deezer user for connections to deezer api
 * Gets the api key and stores it on global client struct
 * Sets curl handle and unset after use it 
 * Create a json object and free after use it 
 *
 * @return error code
 */
static int deezer_create_user();

/**
 * Create the playlists from user at the 
 * begining.
 */
static int deezer_get_playlists_for_user(); 

/**
 *  Create playlist from a list of tracks
 *
 */
static int deezer_create_playlist_with_tracks(track_t **tracklist, int nb_tracks, playlist_t **outplaylist);
static deecer_result_t deezer_init_playlist(playlist_t **playlist);

/**
 * Generate a track_t object from a json object.
 * Checks if track exists in pool, in that case returns
 * a pointer to it and ignore json
 * It allocates necesary memory
 *
 * @param json with track data
 * @param ref track_t object to write data
 * @return error code
 */
static int deezer_get_track_from_json(const cJSON *json_track, track_t **track);

/**
 * Get track from pool
 *
 * @param track id
 * @return track_t pointer 
 */
static track_t *deezer_get_track_from_pool(unsigned long id);

/**
 * Add track to the tracks pool creating space for it
 *
 * @param track The track pointer to add to the pool
 * @return RESULT CODE
 */
static int deezer_add_track(track_t *track);

/**
 * Check if track id exists on pool
 *
 * @param track id 
 * @return boolean
 */
static bool deezer_track_exists(unsigned long id);

/**
 * Ask to the API for track info
 *
 */
static int deezer_get_track_data(track_t **track, unsigned long id);

/**
 * Generate an artist_t object from a json snippet.
 * Checks if exists in pool before to create
 *
 * @param the json 
 * @param reference to a artist_t pointer
 * @return error code
 */
static int deezer_get_artist_from_json(const cJSON *json_artist, artist_t **artist);

/**
 * Get artist from pool. Can use deezer_artist_exists first
 * to avoid null return
 * 
 * @param artist id
 * @return artist_t pointer
 */
static artist_t *deezer_get_artist_from_pool(unsigned long id);

/**
 * Add artist to the pool reserving memory for it 
 *
 * @param artist The artist pointer to add to the pool
 * @return RESULT CODE
 */
static int deezer_add_artist(artist_t *artist);

/**
 * Check if artist id exists on pool
 *
 * @param artist id 
 * @return boolean
 */
static bool deezer_artist_exists(unsigned long id);

/**
 * Generate album_t from a json snippet if does not 
 * exists on pool
 *
 */
static int deezer_get_album_from_json(const cJSON *json_album, album_t **album);

/**
 * Get album from pool. Can use deezer_album_exists first
 * to avoid null return
 * 
 * @param album id
 * @return album_t pointer
 */
static album_t *deezer_get_album_from_pool(unsigned long id);

/**
 * Add album to pool 
 *
 */
static int deezer_add_album(album_t *album);

/**
 * Check if album id exists on pool
 * 
 * @param album id
 * @return boolean
 */
static bool deezer_album_exists(unsigned long id);

/**
 * Generate playlist_t from a json snippet if does not 
 * exists on pool
 *
 */
static int deezer_get_playlist_from_json(const cJSON *json_playlist, playlist_t **playlist);  
 
/**
 * Get playlist from pool. Can use deezer_playlist_exists first
 * to avoid null return
 * 
 * @param playlist id
 * @return playlist_t pointer
 */
static playlist_t *deezer_get_playlist_from_pool(unsigned long id);

/**
 *
 */
static char *deezer_get_track_tokens_array(track_t **tracks_list, int nb_songs); 

/**
 *
 */
static deecer_result_t deezer_get_media_url_from_json(char **out_url, cJSON *media_json);

/**
 * Add playlist to pool 
 *
 */
static int deezer_add_playlist(playlist_t *playlist);

/**
 * Check if playlist id exists on pool
 * 
 * @param playlist id
 * @return boolean
 */
static bool deezer_playlist_exists(unsigned long id);

/**
 * Add a track to the list of tracks of a playlist
 */
static int deezer_playlist_add_track(playlist_t *playlist, track_t *track); 


// direct request to api
//
// NOT NEEDED AT THIS MOMENT
// static int deezer_get_user_data(user_t *user);
// static int deezer_get_artist_data(artist_t *artist, unsigned long id);
// static int deezer_get_album_data(album_t *album, unsigned long id);

/**
 * Make request to API to get all the data from a playlist
 * and write it in the pointer passed as param
 *
 */
static int deezer_get_playlist_data(playlist_t **playlist, unsigned long id);

/**
 * Get url for downloading media
 *
 * @param pointer to track pointer
 * @return error code
 */
static int deezer_get_media_url(track_t *track);

/****
 * Downloads media file to a temporary folder
 * and returns path to file
 *
 * @param track_t track for download 
 * @param char **path: path to file. Caller has to free char*
 * @return error code
 */
static int deezer_download_media_file(track_t *track);

/**
 * Takes crypted file stored locally and generate a new decrypted audio file
 * after decryption removes crypted file. 
 *
 * @param the track_t to generate file paths
 * @return error code
 */
static int deezer_decrypt_file(track_t *track);

/**
 * Make all other preparations together and request
 * 1.init_options()
 * 2.set_headers()
 * 3.set_url()
 * 4.set_post()
 * 5.reset memory chunk
 * 6. make request
 * 7. check if request return a json filetype (dont check content)
 */
static int deezer_make_request(enum deezer_requests request, bool needToken, const char *param);

/**
 * Set curlopt with common values for all
 * requests except media file download 
 *
 */
static int deezer_curl_set_init_options(); 

/**
 * Set curl headers 
 *
 * @param boolean is token is needed for request (only false in deezer_get_user)
 */
static int deezer_curl_set_headers(bool needToken); 

/**
 * Set url for request. build url depending on request type
 *
 * @param enum deezer_requests 
 */
static int deezer_curl_set_url(enum deezer_requests request); 

/**
 * Set json for POST depending on request type.
 *
 * @param only usefull depending on request
 *      - search: query search
 *      - get media: track_token 
 */
static int deezer_curl_set_post_json(enum deezer_requests request, const char *param);

/**
 * Callback for receiving data from requests
 *
 * @param userp: memory_t struct 
 */
static size_t writecallback(char *contents, size_t size, size_t nmemb, void *userp);

/**
 * Callback for downloading file and store locally
 *
 * @param stream: file stream where to save data 
 */
static size_t writefilecallback(void *ptr, size_t size, size_t nmemb, FILE *stream);

static void deezer_free_client(deezer_client_t *client);
static void deezer_free_user(user_t *user);
static void deezer_free_track(track_t *track);
static void deezer_free_artist(artist_t *artist);
static void deezer_free_album(album_t *album);
static void deezer_free_playlist(playlist_t *playlist);

/*******************
 *
 * Public functions
 *
 ******************/

int deezer_init(config_t *config) {
    int err_code = 0;
    // creamos el objeto para el cliente
    err_code = deezer_create_client();
    if (DC_SUCCESS != err_code) {
        return err_code;
    }
    
    // añadimos el arl antes de intentar conectar
    client->arl = config->arl;
    // inicializamos curl
    curl_global_init(CURL_GLOBAL_ALL);
    client->curl_handle = curl_easy_init();

    // inicializamos user
    err_code = deezer_create_user();
    if (DC_SUCCESS != err_code) {
        return err_code;
    }
    return DC_SUCCESS;
}

content_t *deezer_search(const char *query) {
    // aqui guardaremos el contenido que retornaremos
    // tanto si tenemos exito como si no
    content_t *resp = content_create(1);

    int err = deezer_make_request(DC_PAGE_SEARCH, true, query);

    if (DC_SUCCESS != err) {
        content_add_line(resp, "[deezer_search] Problemas con curl.");
        return resp;
    }

    //tenemos un json
    // objeto que contiene el json global
    cJSON *json = cJSON_Parse(client->mem.memory);
    // LOG("json: %s\n", cJSON_Print(json));
    // Queremos quedarnos con la lista de TRACK
    cJSON *json_results = cJSON_GetObjectItem(json, "results"); // incluye todos los results
    // LOG("results: %s\n", cJSON_Print(json_results));
    cJSON *json_tracks = cJSON_GetObjectItem(json_results, "TRACK"); // incluye la info de tracks
    // LOG("track: %s\n", cJSON_Print(json_tracks));
    cJSON *json_data = cJSON_GetObjectItem(json_tracks, "data"); // array con los tracks

    // debo devolver un content_t con una lista de tracks, para ello
    // tengo que añadirlos tracks a la pool y componer el content_t
    if (cJSON_IsArray(json_data)) {
        content_add_line(resp, "[ Play Playlist ]");
        cJSON *iterator = NULL;
        cJSON_ArrayForEach(iterator, json_data) {
            track_t *track = NULL;
            if (deezer_get_track_from_json(iterator, &track) == DC_SUCCESS) {
                content_add_track(resp, track);
            }
        }

        // Add playlist item to first row
        playlist_t *playlist = NULL;
        if (DC_SUCCESS == deezer_create_playlist_with_tracks(resp->tracks, resp->numlines - 1, &playlist)) {
            content_add_playlist_in_row(resp, playlist, 0);
        }
    }
    curl_easy_reset(client->curl_handle);
    return resp;
}

int deezer_get_media(track_t *track, char **filename) {
    // seteamos el path 
    *filename = deezer_get_filepath(track);
    //comprobamos si ya existe el fichero y nos 
    //evitamos la descarga y desencriptacion
    if (access(*filename, F_OK) == 0) {
        return DC_SUCCESS;
    }
    // descargamos el fichero cifrado
    if (DC_SUCCESS != deezer_download_media_file(track)) {
        return DC_ERROR_CURL_DOWNLOADING;
    }
    if (DC_SUCCESS != deezer_decrypt_file(track)) {
        return DC_ERROR_DECRYPT;
    }
    return DC_SUCCESS;
}

char *deezer_get_filepath(track_t *track) {
    char *filepath = NULL;
    if (config->keep_downloads) {
        asprintf(&filepath, "%s/%lu.mp3", config->download_path, track->id);
    } else {
        asprintf(&filepath, "/tmp/%lu.mp3", track->id);
    }
    return filepath;
}
user_t *deezer_get_user() {
    return user;
}
track_t *deezer_get_track(unsigned long id) {
    for (int i=0; i<nb_tracks; i++) {
        if (id == tracks[i]->id) {
            return tracks[i];
        }
    }
    return NULL;
} 
artist_t *deezer_get_artist(unsigned long id) {
    // NOT IMPLEMENTED
    return NULL;
}
album_t *deezer_get_album(unsigned long id) {
    // NOT IMPLEMENTED
    return NULL;
}
playlist_t *deezer_get_playlist(unsigned long id) {
    playlist_t *playlist = NULL;

    if (deezer_playlist_exists(id)) {
        playlist = deezer_get_playlist_from_pool(id);
        if (playlist->has_tracks) {
            return playlist;
        }
    }
    
    deezer_get_playlist_data(&playlist, id);

    return playlist;
}

bool deezer_arl_is_valid(const char *arl) {
    // checkeo muy rudimentario del valor de la clave arl
    if (arl == NULL) {
        return false;
    }
    if (strlen(arl) < 20) {
        return false;
    }
    return true;
}

bool deezer_track_is_valid(track_t *track) {
    if (track == NULL) {
        return false;
    }
    if (0 >= track->id) {
        return false;
    }
    return true;
}

bool deezer_artist_is_valid(artist_t *artist) {
    // NOT IMPLEMENTED
    return true;
}
bool deezer_album_is_valid(album_t *album) {
    // NOT IMPLEMENTED
    return false;
}
bool deezer_playlist_is_valid(const playlist_t *playlist) {
    if (!playlist) {
        LOG("playlist null\n");
        return false;
    }
    if (0 >= playlist->id) {
        LOG("playlists con id 0 o negativo.\n");
        return false;
    }
    if (0 >= playlist->nb_tracks) {
        LOG("playlists con 0 numero de tracks.\n");
        return false;
    }
    return true;
}

/*******************
 *
 * Private functions
 *
 ******************/

static int deezer_create_client()  {
    client = calloc(1, sizeof(deezer_client_t));
    if (client == NULL) {
        return DC_ERROR_MEMORY_MAP_FAILED;
    }
    return DC_SUCCESS;
}

// ====
// USER
// ====
static int deezer_create_user() {
    // reservamos la memoria para la usuaria
    user = calloc(1, sizeof(user_t));
    if (!user) {
        return DC_ERROR_INICIALIZATION_FAILED;
    }
    //reservamos memoria para la lista de punteros a playlists
    user->playlists = calloc(1,sizeof(playlist_t*));
    if (!user->playlists) {
        return DC_ERROR_MEMORY_MAP_FAILED;
    }

    int err = deezer_make_request(DC_GET_TOKEN, false, "");
    if (DC_SUCCESS != err) {
        return err;
    }

    cJSON *json = cJSON_Parse(client->mem.memory);

    if (!json || cJSON_IsInvalid(json) || !cJSON_IsObject(json)) {
        //liberamos el objeto json y salimos (petará si es null??)
        //quiza deban estar separados los casos
        cJSON_Delete(json);
        return DC_ERROR_UNKNOWN;
    }
    // comprobamos si el json nos informa de posibles errores en la consulta
    cJSON *errors = cJSON_GetObjectItem(json, "error");
    if (errors->child != NULL) {
        LOG("%s - %s\n", errors->child->string, errors->child->valuestring);
        cJSON_Delete(json);
        return DC_ERROR_UNKNOWN;
    }
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
    user->id = (unsigned long)u_userid->valuedouble;
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
    
    // liberamos el objeto json
    cJSON_Delete(json);
    
    LOG("user id: %lu\n", user->id);
    LOG("user email: %s\n", user->email);
    LOG("user name: %s\n", user->name);
    LOG("loved tracks: %s\n", user->lovedtracks_id);
    LOG("session id: %s\n", client->session_id);
    LOG("api token: %s\n", client->api_token);
    // reseteamos las opciones de curl antes de retornar.
    curl_easy_reset(client->curl_handle);

    //Ahora pedimos la pagina de profile para rellenar las playlists del usuario
    deezer_get_playlists_for_user();
    return DC_SUCCESS;
}

static int deezer_get_playlists_for_user() {
    int user_nb_playlists = 0;
    playlist_t **lista = NULL;

    int err = deezer_make_request(DC_PAGE_PROFILE, true, "");

    if (DC_SUCCESS != err) {
        return err;
    }

    cJSON *json = cJSON_Parse(client->mem.memory);
    // comprobamos que sea un json valido
    if (!json || cJSON_IsInvalid(json) || !cJSON_IsObject(json)) {
        cJSON_Delete(json);
        return DC_ERROR_CJSON_INVALID;
    }
    // comprobamos si el json nos informa de posibles errores en la consulta
    cJSON *errors = cJSON_GetObjectItem(json, "error");
    if (errors->child != NULL) {
        LOG("%s - %s\n", errors->child->string, errors->child->valuestring);
        cJSON_Delete(json);
        return DC_ERROR_UNKNOWN;
    }

    cJSON *results = cJSON_GetObjectItem(json, "results");
    cJSON *TAB = cJSON_GetObjectItem(results, "TAB");
    cJSON *playlists = cJSON_GetObjectItem(TAB, "playlists");
    cJSON *count = cJSON_GetObjectItem(playlists, "count");
    cJSON *data = cJSON_GetObjectItem(playlists, "data");

    if (!cJSON_IsNumber(count) || !cJSON_IsArray(data)) {
        LOG("El array de playlists del usuario no es valido.\n");
        char *data_prt = cJSON_Print(data);
        LOG("%s", data_prt);
        cJSON_Delete(json);
        return DC_ERROR_CJSON_INVALID;
    }
    
    cJSON *iterator = NULL;
    cJSON_ArrayForEach(iterator, data) {
        lista = realloc(lista, (user_nb_playlists + 1) * sizeof(playlist_t*));
        if (!lista) {
            cJSON_Delete(json);
            return DC_ERROR_MEMORY_MAP_FAILED;
        }

        cJSON *pl_id = cJSON_GetObjectItem(iterator, "PLAYLIST_ID");
        if (deezer_playlist_exists(strtoul(pl_id->valuestring, NULL, 10))) {
            lista[user_nb_playlists] = deezer_get_playlist_from_pool(strtoul(pl_id->valuestring, NULL, 10));
            user_nb_playlists++;
            continue;
        }
        cJSON *pl_title = cJSON_GetObjectItem(iterator, "TITLE");
        cJSON *pl_nbtracks = cJSON_GetObjectItem(iterator, "NB_SONG");

        playlist_t *playlist = NULL;
        deezer_init_playlist(&playlist);
        playlist->id = strtoul(pl_id->valuestring, NULL, 10);
        playlist->title = strdup(pl_title->valuestring);
        playlist->user = user;
        playlist->nb_tracks = pl_nbtracks->valueint;
        playlist->has_tracks = false;
        lista[user_nb_playlists] = playlist;
        user_nb_playlists++;
        deezer_add_playlist(playlist);
    }
    cJSON_Delete(json);
    user->playlists = lista;
    user->nb_playlists = user_nb_playlists;

    // reseteamos las opciones de curl antes de retornar.
    curl_easy_reset(client->curl_handle);
    
    return DC_SUCCESS;
}

static int deezer_create_playlist_with_tracks(track_t **tracklist, int nb_tracks, playlist_t **playlist) {
    if (DC_SUCCESS != deezer_init_playlist(&(*playlist)))
    {
        return DC_ERROR_MEMORY_MAP_FAILED;
    }
    (*playlist)->id = 1;
    (*playlist)->title = strdup("[ Play Playlist ]");
    (*playlist)->nb_tracks = nb_tracks;
    (*playlist)->tracks = tracklist;
    return DC_SUCCESS;
}

// =====
// TRACK
// =====
static int deezer_get_track_from_json(const cJSON *json_track, track_t **track) {
    if (!cJSON_IsObject(json_track)) {
        return DC_ERROR_CJSON_INVALID;
    }
    
    // Si existe el objeto FALLBACK dentro del json, usamos FALLBACK,
    // ya que contiene los datos correctos de track token
    cJSON *FALLBACK = cJSON_GetObjectItem(json_track, "FALLBACK");
    if (FALLBACK && !cJSON_IsInvalid(FALLBACK) && cJSON_IsObject(FALLBACK)) {
        json_track = FALLBACK;
    }

    cJSON *id = cJSON_GetObjectItem(json_track, "SNG_ID");
    
    // una vez que tenemos el id, buscamos si ya tenemos ese track
    // en la pool, si es así, devolvemos el track desde la pool
    if (deezer_track_exists(strtoul(id->valuestring, NULL, 10))) {
        //buscar el track en la pool y retornarlo
        *track = deezer_get_track_from_pool(strtoul(id->valuestring, NULL, 10));
        return DC_SUCCESS;
    }

    // si no es asi, lo tenemos que desmembrar y añadir
    cJSON *title = cJSON_GetObjectItem(json_track, "SNG_TITLE");
    cJSON *token = cJSON_GetObjectItem(json_track, "TRACK_TOKEN");
    cJSON *token_expire = cJSON_GetObjectItem(json_track, "TRACK_TOKEN_EXPIRE");
    cJSON *artist = cJSON_GetObjectItem(json_track, "ARTISTS"); // array de artists
    // de momento aun no tratamos albumes
    //cJSON *album_id = cJSON_GetObjectItem(json_track, "ALB_ID");
    
    *track = calloc(1, sizeof(track_t));
    if (*track == NULL) {
        return DC_ERROR_MEMORY_MAP_FAILED;
    }
    if (cJSON_IsString(id)){
        (*track)->id = strtoul(id->valuestring, NULL, 10);
    }
    if (cJSON_IsString(title)) {
        (*track)->title = strdup(title->valuestring);
    }
    if (cJSON_IsString(token)) {
        (*track)->token = strdup(token->valuestring);
    }
    if (cJSON_IsNumber(token_expire)) {
        (*track)->token_expire = (time_t)token_expire->valuedouble;
    }
    
    if (!cJSON_IsArray(artist)) {
        return DC_ERROR_CJSON_INVALID;
    }

    cJSON *iterator = NULL;
    cJSON_ArrayForEach(iterator, artist) {
        artist_t *artist = NULL;
        if (deezer_get_artist_from_json(iterator, &artist) == DC_SUCCESS) {
            if (0 == (*track)->nb_artists) {
                (*track)->artists = calloc(1, sizeof(artist_t*));
            } else {
                (*track)->artists = realloc((*track)->artists, ((*track)->nb_artists + 1) * sizeof(artist_t*) );
            }
            (*track)->artists[(*track)->nb_artists] = artist;
            (*track)->nb_artists++;
        }
    }
    return DC_SUCCESS;
}

static track_t *deezer_get_track_from_pool(unsigned long id) {
    for (int i=0; i < nb_tracks; i++) {
        if (id == tracks[i]->id) {
            return tracks[i];
        }
    }
    return NULL;
}

static int deezer_add_track(track_t *track) {
    if (track == NULL) {
        return DC_ERROR_INICIALIZATION_FAILED; 
    }
    if (0 == nb_tracks) {
        tracks = calloc (1, sizeof(track_t*));
        if (tracks == NULL) {
            return DC_ERROR_MEMORY_MAP_FAILED;
        }
    } else {
        // creamos un puntero temporal donde pedimos el espacio,
        // no queremos perder nuestro tracks bueno
        track_t **tmp_tracks = realloc(tracks, (nb_tracks + 1) * sizeof(track_t*));
        if (tmp_tracks == NULL) {
            return DC_ERROR_MEMORY_MAP_FAILED;
        }
        tracks = tmp_tracks;
    }

    // Si no tenemos media-url, vamos a conseguirla de forma individual (no optimo)
    if (!track->has_url) {
        int err = deezer_get_media_url(track);
        if (DC_SUCCESS != err) {
            LOG("No se ha podido conseguir los datos del track, cancelando insercion.\n");
            return err;
        }

        /**
         * DEPRECATED
         * Este fue un mecanismo cuando tuvimos problemas con canciones que no
         * conseguiamos un media-url correcto. Este sistema no lo solucionó.
         * Finalmente era el tema del FALLBACK
         */
        // if (DC_SUCCESS != err) {
        //     //no tenemos una url valida, asi que descargamos de nuevo los datos
        //     //del track y volvemos a intentarlo
        //     LOG("Segundo intento de conseguir media url.\n");
        //     err = deezer_get_track_data(&track, track->id);
        //     if (DC_SUCCESS != err) {
        //         LOG("No se ha podido conseguir los datos del track, cancelando insercion.\n");
        //         return err;
        //     }
        //     err = deezer_get_media_url(track);
        //     if (DC_SUCCESS != err) {
        //         LOG("Segundo intento de conseguir media url fallido tambien.\n");
        //         return err;
        //     }
        // }
    }
    LOG("=== NEW TRACK ===\n");
    LOG("id: %lu\n", track->id);
    LOG("title: %s\n", track->title);
    if (track->artists[0]) {
        LOG("artist: %s\n", track->artists[0]->name);
    }
    if (track->has_url) {
        LOG("media-url: %s\n", track->media_url);
    }
    tracks[nb_tracks] = track;
    nb_tracks++;
    return DC_SUCCESS;
}

static bool deezer_track_exists(unsigned long id) {
    for (int i=0; i < nb_tracks; i++) {
       if (id == tracks[i]->id) {
           return true;
       }
    }
    return false;
}

static int deezer_get_track_data(track_t **track, unsigned long id) {
    // 1.inicializamos *track, borrando lo que hubiere si hay algo
    if (!*track) {
        track = calloc(1, sizeof(track_t*));
        if (!*track) {
            return DC_ERROR_MEMORY_MAP_FAILED;
        }
    }
    (*track)->id = 0;
    (*track)->token_expire = 0;
    free((*track)->title);
    (*track)->title = NULL;
    free((*track)->token);
    (*track)->token = NULL;
    free((*track)->artists);
    (*track)->artists = NULL;
    (*track)->album = NULL; // album no lo liberamos, pero ya no apuntamos a el.
    free((*track)->media_url);
    (*track)->media_url = NULL;
    // 2.hacemos la request a la pagina de track info
    char *str_id = NULL;
    asprintf(&str_id, "%lu", id);
    int err = deezer_make_request(DC_PAGE_TRACK, true, str_id);
    free(str_id);
    if (DC_SUCCESS != err) {
        return err;
    }
    // 3.convertimos el json en un track
    cJSON *json = cJSON_Parse(client->mem.memory);
    // comprobamos que sea un json valido
    if (!json || cJSON_IsInvalid(json) || !cJSON_IsObject(json)) {
        cJSON_Delete(json);
        return DC_ERROR_CJSON_INVALID;
    }
    
    // comprobamos si el json nos informa de posibles errores en la consulta
    cJSON *errors = cJSON_GetObjectItem(json, "error");
    if (errors->child != NULL) {
        LOG("%s - %s\n", errors->child->string, errors->child->valuestring);
        cJSON_Delete(json);
        return DC_ERROR_UNKNOWN;
    }

    cJSON *results = cJSON_GetObjectItem(json, "results");
    cJSON *DATA = cJSON_GetObjectItem(results, "DATA");
   
    err = deezer_get_track_from_json(DATA, &(*track));

    if (DC_SUCCESS != err) {
        return err;
    }
    // 4.actualizamos la pool (ya se hace porque nos han pasado 
    // una referencia de la pool) o se encarga quien me llame
    return DC_SUCCESS;
}


// ======
// ARTIST
// ======
static int deezer_get_artist_from_json(const cJSON *json_artist, artist_t **artist) {
    if (!cJSON_IsObject(json_artist)) {
        return DC_ERROR_CJSON_INVALID;
    }
    cJSON *id = cJSON_GetObjectItem(json_artist, "ART_ID");
    if (deezer_artist_exists(strtoul(id->valuestring, NULL, 10))) {
        *artist = deezer_get_artist_from_pool(strtoul(id->valuestring, NULL, 10));
        return DC_SUCCESS;
    }
    cJSON *name = cJSON_GetObjectItem(json_artist, "ART_NAME");
    *artist = calloc(1, sizeof(artist_t));
    if (*artist == NULL) {
        return DC_ERROR_MEMORY_MAP_FAILED;
    }
    if (cJSON_IsString(id)) {
        (*artist)->id = strtoul(id->valuestring, NULL, 10);
    }
    if (cJSON_IsString(name)) {
        (*artist)->name = strdup(name->valuestring);
    }
    return deezer_add_artist(*artist);
}

static artist_t *deezer_get_artist_from_pool(unsigned long id) {
    for (int i=0; i < nb_artists; i++) {
        if (id == artists[i]->id) {
            return artists[i];
        }
    }
    return NULL;
}

static int deezer_add_artist(artist_t *artist) {
    if (artist == NULL) {
        return DC_ERROR_INICIALIZATION_FAILED; 
    }
    if (0 == nb_artists) {
        artists = calloc (1, sizeof(artist_t*));
        if (artists == NULL) {
            return DC_ERROR_MEMORY_MAP_FAILED;
        }
    } else {
        // creamos un puntero temporal donde pedimos el espacio,
        // no queremos perder nuestra lista de artists
        artist_t **tmp_artists = realloc(artists, (nb_artists + 1) * sizeof(artist_t*));
        if (tmp_artists == NULL) {
            return DC_ERROR_MEMORY_MAP_FAILED;
        }
        artists = tmp_artists;
    }

    LOG("=== NEW ARTIST ===\n");
    LOG("id: %lu\n", artist->id);
    LOG("name: %s\n", artist->name);
    artists[nb_artists] = artist;
    nb_artists++;
    return DC_SUCCESS;
}

static bool deezer_artist_exists(unsigned long id) {
    for (int i=0; i < nb_artists; i++) {
       if (id == artists[i]->id) {
           return true;
       }
    }
    return false;
}
// =====
// ALBUM
// =====
static int deezer_get_album_from_json(const cJSON *json_album, album_t **album) {
    // NOT IMPLEMENTED
    return DC_ERROR_NOT_IMPLEMENTED;
}

static album_t *deezer_get_album_from_pool(unsigned long id) {
    for (int i=0; i < nb_albums; i++) {
        if (id == albums[i]->id) {
            return albums[i];
        }
    }
    return NULL;
}

static int deezer_add_album(album_t *album) {
    return DC_ERROR_NOT_IMPLEMENTED;
}

static bool deezer_album_exists(unsigned long id) {
    for (int i=0; i < nb_albums; i++) {
        if (id == albums[i]->id) {
            return true;
        }
    }
    return false;
}

// ========
// PLAYLIST 
// ========
static int deezer_get_playlist_from_json(const cJSON *json_playlist, playlist_t **playlist) {
    // NOT IMPLEMENTED
    return DC_ERROR_NOT_IMPLEMENTED;
}

static playlist_t *deezer_get_playlist_from_pool(unsigned long id) {
    for (int i=0; i < nb_playlists; i++) {
        if (id == playlists[i]->id) {
            return playlists[i];
        }
    }
    return NULL;
}

static int deezer_get_playlist_data(playlist_t **playlist, unsigned long id) {
    /*
     * 1. nos pasan un puntero a un puntero de playlist.
     *    puede ser que tenga datos pero no tenga tracks
     *    porque viene de user_playlists y es la primera 
     *    vez que se manda a center. La segunda vez 
     *    deberia estar completa 
     * 2. Comprobamos si tiene datos, si no tiene creamos 
     *    la memoria necesario con calloc e inicializamos.
     *    Lo mejor sería tener una funcion init playlist
     */
    if (!(*playlist)) {
        deezer_init_playlist(&(*playlist));
    }
    /** 
     * 3. Comprobamos si tiene tracks con has_tracks
     * 4. Si tiene, no tenemos que hacer nada, la devolvemos 
     *    tal cual.
     */
    if ((*playlist)->has_tracks) {
        return DC_SUCCESS;
    }

    /** 
     * 5. Si no tiene tracks, pedimos a la pagina de playlists
     *    la info de dicha playlist y refrescamos sus datos
     *    basicos.
     */
    char *id_str;
    asprintf(&id_str, "%lu", id);
    int err = deezer_make_request(DC_PAGE_PLAYLISTS, true, id_str);
    free(id_str);

    if (DC_SUCCESS != err) {
        return err;
    }

    cJSON *json = cJSON_Parse(client->mem.memory);
    // comprobamos que sea un json valido
    if (!json || cJSON_IsInvalid(json) || !cJSON_IsObject(json)) {
        cJSON_Delete(json);
        return DC_ERROR_CJSON_INVALID;
    }
    cJSON *results = cJSON_GetObjectItem(json, "results");
    cJSON *DATA = cJSON_GetObjectItem(results, "DATA");
    cJSON *PLAYLIST_ID = cJSON_GetObjectItem(DATA, "PLAYLIST_ID");
    cJSON *TITLE = cJSON_GetObjectItem(DATA, "TITLE");
    
    (*playlist)->id = strtoul(PLAYLIST_ID->valuestring, NULL, 10);
    (*playlist)->title = strdup(TITLE->valuestring);
    (*playlist)->nb_tracks = 0;

     /**
     * 6. Cuando llega el momento de añadir las tracks, el 
     *    problema viene si tenemos que pedir la media-url de 
     *    cada track, como eso no es optimo, lo que haremos será
     *    rellenar los datos que nos da el json de la playlist 
     *    (que son todos menos la media-url) para ello tenemos
     */
    cJSON *SONGS = cJSON_GetObjectItem(results, "SONGS");
    cJSON *data = cJSON_GetObjectItem(SONGS, "data");

    if (!cJSON_IsArray(data)) {
        cJSON_Delete(json);
        LOG("No tenemos un json valido de tracks en la playlist.\n");
        return DC_ERROR_CJSON_INVALID;
    }

    cJSON *iterator = NULL;
    cJSON_ArrayForEach(iterator, data) {
        track_t *track = NULL;
        // convertimos el json en un track
        err = deezer_get_track_from_json(iterator, &track);
        if (DC_SUCCESS != err) {
            continue;
        }
        // añadimos el track a la playlist
        if (DC_SUCCESS != deezer_playlist_add_track(*playlist, track)) {
            LOG("Error añadiendo track a la playlist. \n");
        }
    }

    cJSON_Delete(json);

    /**    
     * 7. Cuando ya tenemos la lista de tracks (y no los hemos 
     *    añadido a la pool aun), vamos a crear una única request
     *    a get-media-url, pero en vez de pasarle un track_token, 
     *    le pasaremos un array de track_tokens y tendremos que 
     *    parsear a cada track su media-url, y una vez completos 
     *    los tracks, ya los podemos añadir a la pool
     */
    char *tokens_list = deezer_get_track_tokens_array((*playlist)->tracks, (*playlist)->nb_tracks);
    err = deezer_make_request(DC_PAGE_MEDIA_GET_URL_MULTI, true, tokens_list);
    free(tokens_list);
    
    if (DC_SUCCESS != err) {
        return err;
    }

    cJSON *json_media = cJSON_Parse(client->mem.memory);
    // comprobamos que sea un json valido
    if (!json_media || cJSON_IsInvalid(json_media) || !cJSON_IsObject(json_media)) {
        cJSON_Delete(json_media);
        return DC_ERROR_CJSON_INVALID;
    }

    cJSON *data_media = cJSON_GetObjectItem(json_media, "data");
    if (!cJSON_IsArray(data_media)) {
        LOG("No tenemos un array de media_url's.\n");
        return DC_ERROR_CJSON_INVALID;
    }

    cJSON *iterator_media = NULL;
    int i=0;
    char *media_url = NULL;
    cJSON_ArrayForEach(iterator_media, data_media) {
        cJSON *media = cJSON_GetObjectItem(iterator_media, "media");
        if (DC_SUCCESS == deezer_get_media_url_from_json(&media_url, media)) {
            (*playlist)->tracks[i]->media_url = strdup(media_url);
            (*playlist)->tracks[i]->has_url = true;
        } else {
            LOG("%s - no hemos recibido un media_url adecuado, eso parece.\n", (*playlist)->tracks[i]->title);
        } 
        err = deezer_add_track((*playlist)->tracks[i]);
        if (DC_SUCCESS != err) {
            LOG("No hemos podido añadir el track %s a la pool.\n", (*playlist)->tracks[i]->title);
        }
        i++;
    }

    cJSON_Delete(json_media);
    free(media_url);
    return DC_SUCCESS;
}

static char *deezer_get_track_tokens_array(track_t **tracks_list, int nb_songs) {
    char *retval = NULL;
    cJSON *tokens_array = cJSON_CreateArray();

    for (int i=0; i<nb_songs; i++) {
        if (!tracks_list[i]) {
            continue;
        }
        if (!tracks_list[i]->title) {
            continue;
        }
        cJSON *token_item = cJSON_CreateString(tracks_list[i]->token);
        cJSON_AddItemToArray(tokens_array, token_item);
    }
    retval = cJSON_PrintUnformatted(tokens_array);
    cJSON_Delete(tokens_array);
    return retval;
}

static deecer_result_t deezer_get_media_url_from_json(char **out_url, cJSON *media_json) {

    if (!cJSON_IsArray(media_json)) {
        LOG("[deezer_get_media_url] - media not valid\n");
        return DC_ERROR_CURL_RESPONSE_ERROR;
    }

    cJSON *media_item = cJSON_GetArrayItem(media_json, 0);
    if (!media_item) {
        LOG("[deezer_get_media_url] - media_item not valid\n");
        return DC_ERROR_CURL_RESPONSE_ERROR;
    }
    cJSON *sources = cJSON_GetObjectItem(media_item, "sources");
    if (!cJSON_IsArray(sources)) {
        LOG("[deezer_get_media_url] - sources not valid\n");
        return DC_ERROR_CURL_RESPONSE_ERROR;
    }
    cJSON *sources_item = cJSON_GetArrayItem(sources, 0);
    cJSON *url = cJSON_GetObjectItem(sources_item, "url");
    *out_url = strdup(url->valuestring);
    return DC_SUCCESS;
}

static int deezer_add_playlist(playlist_t *playlist) {
    if (!playlist || !deezer_playlist_is_valid(playlist)) {
        LOG("No se ha podido añadir una playlists a la pool.\n");
        return DC_ERROR_UNKNOWN;
    }
    if (0 == nb_playlists) {
        playlists = calloc(1, sizeof(playlist_t*));
        if (!playlists) {
            return DC_ERROR_MEMORY_MAP_FAILED;
        }
    } else {
        playlist_t **tmp_playlists = realloc(playlists, (nb_playlists + 1) * sizeof(playlist_t*));
        if (!tmp_playlists) {
            return DC_ERROR_MEMORY_MAP_FAILED;
        }
        playlists = tmp_playlists;
    }

    LOG("=== NEW PLAYLIST ===\n");
    LOG("id: %lu\n", playlist->id);
    LOG("title: %s\n", playlist->title);
    LOG("nb_songs: %d\n", playlist->nb_tracks);
    LOG("has_tracks: %d\n", playlist->has_tracks);
    playlists[nb_playlists] = playlist;
    nb_playlists++;
    return DC_SUCCESS;
}

static bool deezer_playlist_exists(unsigned long id) {
    for (int i=0; i < nb_playlists; i++) {
        if (id == playlists[i]->id) {
            return true;
        }
    }
    return false;
}

static int deezer_playlist_add_track(playlist_t *playlist, track_t *track) {
    if (!playlist || !track ) {
        return DC_ERROR_UNKNOWN;
    }
    if (!deezer_track_is_valid(track)) {
        LOG("TRACK INVALIDO\n");
        return DC_ERROR_UNKNOWN;
    }
    if (0 == playlist->nb_tracks) {
        playlist->tracks = calloc(1, sizeof(track_t*));
    } else {
        track_t **tmp = realloc(playlist->tracks, (playlist->nb_tracks + 1) * sizeof(track_t*));
        if (!tmp) {
            return DC_ERROR_MEMORY_MAP_FAILED;
        }
        playlist->tracks = tmp;
    }

    if (!playlist->tracks) {
        return DC_ERROR_MEMORY_MAP_FAILED;
    }

    LOG("=== ADD TRACK TO PLAYLIST ===\n");
    LOG("track index: %d\n", playlist->nb_tracks);
    LOG("track_name: %s\n", track->title);
    LOG("playlist_title: %s\n", playlist->title);
    playlist->tracks[playlist->nb_tracks] = track;
    playlist->nb_tracks++;
    playlist->has_tracks = true;
    return DC_SUCCESS;
}

// ===============
// MEDIA FUNCTIONS
// ===============
static int deezer_get_media_url(track_t *track) {
    if (track == NULL) {
        return DC_ERROR_UNKNOWN;
    }

    int err = deezer_make_request(DC_PAGE_MEDIA_GET_URL, true, track->token);
    if (DC_SUCCESS != err) {
        return err;
    }

    // objeto que contiene el json global
    cJSON *json = cJSON_Parse(client->mem.memory);
    
    cJSON *data = cJSON_GetObjectItem(json, "data");
    if (!cJSON_IsArray(data)) {
        LOG("[deezer_get_media_url] - data is not an array\n");
        return DC_ERROR_CURL_RESPONSE_ERROR;
    }
    
    cJSON *data_item = cJSON_GetArrayItem(data, 0);
    if (!data_item) {
        LOG("[deezer_get_media_url] - data_item not valid\n");
        return DC_ERROR_CURL_RESPONSE_ERROR;
    }
    cJSON *media = cJSON_GetObjectItem(data_item, "media");
    /**
     *
     * EXTRAIDO A deezer_get_media_url_from_json
     * para poder usarlo en deezer_get_playlist_data
     *
     *
    if (!cJSON_IsArray(media)) {
        LOG("[deezer_get_media_url] - media not valid\n");
        return DC_ERROR_CURL_RESPONSE_ERROR;
    }

    cJSON *media_item = cJSON_GetArrayItem(media, 0);
    LOG("track-token: %s\n", track->token);
    if (!media_item) {
        LOG("[deezer_get_media_url] - media_item not valid\n");
        LOG("%s\n", client->mem.memory);
        return DC_ERROR_CURL_RESPONSE_ERROR;
    }
    cJSON *sources = cJSON_GetObjectItem(media_item, "sources");
    if (!cJSON_IsArray(sources)) {
        LOG("[deezer_get_media_url] - sources not valid\n");
        return DC_ERROR_CURL_RESPONSE_ERROR;
    }
    cJSON *sources_item = cJSON_GetArrayItem(sources, 0);
    cJSON *url = cJSON_GetObjectItem(sources_item, "url");
    track->media_url = strdup(url->valuestring);
    **/
    char *media_url = NULL;
    if (DC_SUCCESS == deezer_get_media_url_from_json(&media_url, media)) {
        track->media_url = strdup(media_url);
    }
    cJSON_Delete(json);
    
    curl_easy_reset(client->curl_handle);
    return DC_SUCCESS;
}

static int deezer_download_media_file(track_t *track) {
    char *path = deezer_get_filepath(track);
    if (!client->curl_handle) {
        return DC_ERROR_CURL_INIT;
    }
    asprintf(&path, "/tmp/%lu-crypt.mp3", track->id);
    FILE *fp = fopen(path, "wb");
    curl_easy_setopt(client->curl_handle, CURLOPT_URL, track->media_url);
    curl_easy_setopt(client->curl_handle, CURLOPT_WRITEFUNCTION, writefilecallback);
    curl_easy_setopt(client->curl_handle, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(client->curl_handle, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36");
    client->curl_res = curl_easy_perform(client->curl_handle);
    fclose(fp);
    curl_easy_reset(client->curl_handle);
    free(path);
    if (CURLE_OK == client->curl_res) {
        return DC_SUCCESS;
    } else {
        return DC_ERROR_CURL_RESPONSE_ERROR;
    }
}

static int deezer_decrypt_file(track_t *track) {
    LOG("Vamos a desencriptar %s\n", track->title);

    char *track_id = NULL;
    char *sourcefile = NULL;
    char *destfile = deezer_get_filepath(track);
    
    asprintf(&track_id, "%lu", track->id);
    asprintf(&sourcefile, "/tmp/%s-crypt.mp3", track_id);
    
    // Leer el archivo completo a memoria
    FILE *f = fopen(sourcefile, "rb");
    fseek(f, 0, SEEK_END);
    size_t file_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    unsigned char *encrypted = malloc(file_size);
    fread(encrypted, 1, file_size, f);
    fclose(f);

    // Llamar a decrypt_audio
    size_t out_len;
    unsigned char *decrypted = decrypt_audio(track_id, encrypted, file_size, &out_len);
    free(encrypted);

    // Guardar resultado
    
    FILE *out = fopen(destfile, "wb");
    fwrite(decrypted, 1, out_len, out);
    fclose(out);
    // borramos el fichero encriptado
    remove(sourcefile);

    LOG("fichero desencriptado en %s\n", destfile);
    // decrypted tenemos que liberarlo con la funcion rust
    // porque un free de C no liberaria el allocation de rust
    free_decrypted(decrypted, out_len);
    free(track_id);
    free(sourcefile);
    free(destfile);
    return DC_SUCCESS;
}

// ============
// CURL HELPERS
// ============
static int deezer_make_request(enum deezer_requests request, bool needToken, const char *param) {
    int err;
    err = deezer_curl_set_init_options();
    if (err != DC_SUCCESS) {
        return err;
    }
    err = deezer_curl_set_url(request);
    if (err != DC_SUCCESS) {
        return err;
    }
    err = deezer_curl_set_headers(needToken);
    if (err != DC_SUCCESS) {
        return err;
    }
   if (DC_GET_TOKEN != request) {
        err = deezer_curl_set_post_json(request, param);
    
        if (err != DC_SUCCESS) {
            return err;
        }
    }
    
    // liberamos el contenedor de los datos de la respuesta,
    // asi nos aseguramos que no nos queden restos de alguna
    // request anterior
    free(client->mem.memory);
    client->mem.memory = NULL;
    client->mem.size = 0;

    // Hacemos la request
    // realizamos la request y guardamos el codigo de error
    client->curl_res = curl_easy_perform(client->curl_handle);
    
    if (CURLE_OK != client->curl_res) {
        LOG("Error en request\n");
        curl_easy_reset(client->curl_handle);
        return DC_ERROR_CURL_RESPONSE_ERROR;
    }
    char *contenttype = NULL;
    client->curl_res = curl_easy_getinfo(client->curl_handle, CURLINFO_CONTENT_TYPE, &contenttype);
    if ((CURLE_OK != client->curl_res) || !contenttype) {
        LOG("curl_easy_getinfo() no nos da la respuesta correcta.\n");
        LOG("requesttype %d\n", request);

        return DC_ERROR_CURL_CONTENTTYPE;
    }
    if (!strstr(contenttype, "application/json")) {
        LOG("contenttype: %s\n", contenttype);
        return DC_ERROR_CURL_CONTENTTYPE;
    }

    return DC_SUCCESS;
}

static int deezer_curl_set_init_options() {
    if (client->curl_handle == NULL) {
        return DC_ERROR_INICIALIZATION_FAILED;
    }
    curl_easy_setopt(client->curl_handle, CURLOPT_WRITEFUNCTION, writecallback);
    curl_easy_setopt(client->curl_handle, CURLOPT_WRITEDATA, &client->mem);
    curl_easy_setopt(client->curl_handle, CURLOPT_USERAGENT, "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/133.0.0.0 Safari/537.36");
    return DC_SUCCESS;
}

static int deezer_curl_set_headers(bool needToken) {
    // Creamos los headers [man CURLOPT_HTTPHEADER]
    struct curl_slist *list = NULL;
    char *cookie = NULL;
    if (needToken) {
        asprintf(&cookie, "Cookie: arl=%s; sid=%s", client->arl, client->session_id);
    } else {
        asprintf(&cookie, "Cookie: arl=%s", client->arl);
    }
    list = curl_slist_append(list, cookie);
    list = curl_slist_append(list, "Content-Type: application/json");

    curl_easy_setopt(client->curl_handle, CURLOPT_HTTPHEADER, list);

    // liberamos coolie porque append hace una copia de su contenido.
    // no liberamos list porque se necesita como minimo hasta que se haga
    // la request.
    free(cookie);
    return DC_SUCCESS;
    
}
static int deezer_curl_set_url(enum deezer_requests request) {
    //LOG("Entrando en ... deezer_curl_set_url(enum deezer_requests request)\n");
    // construimos la url
    char *url = NULL;

    switch (request) {
        case DC_PAGE_ALBUM:
            asprintf(&url, "%s?method=deezer.pageAlbum&api_version=1.0&api_token=%s&input=3", api_url, client->api_token);
            break;
        case DC_PAGE_HOME:
            asprintf(&url, "%s?method=deezer.pageExplore&api_version=1.0&api_token=%s&input=3", api_url, client->api_token);
            break;
        case DC_PAGE_ARTIST:
            asprintf(&url, "%s?method=deezer.pageArtist&api_version=1.0&api_token=%s&input=3", api_url, client->api_token);
            break;
        case DC_PAGE_PLAYLISTS:
            asprintf(&url, "%s?method=deezer.pagePlaylist&api_version=1.0&api_token=%s&input=3", api_url, client->api_token);
            break;
        case DC_PAGE_TRACK:
            asprintf(&url, "%s?method=deezer.pageTrack&api_version=1.0&api_token=%s", api_url, client->api_token);
            break;
        case DC_PAGE_SEARCH:
            asprintf(&url, "%s?method=deezer.pageSearch&api_version=1.0&api_token=%s&input=3", api_url, client->api_token);
            break;
        case DC_GET_TOKEN:
            asprintf(&url, "%s?method=deezer.getUserData&api_version=1.0&api_token", api_url);
            break;
        case DC_PAGE_PROFILE:
        
            asprintf(&url, "%s?method=deezer.pageProfile&api_version=1.0&api_token=%s&input=3", api_url, client->api_token);
            break;
        case DC_PAGE_MEDIA_GET_URL:
        case DC_PAGE_MEDIA_GET_URL_MULTI:
            asprintf(&url, "%s", media_url);
        default:
            break;
    }
    if (url == NULL) {
        return DC_ERROR_UNKNOWN;
    }

    // creamos las opciones de curl
    curl_easy_setopt(client->curl_handle, CURLOPT_URL, url);
    return DC_SUCCESS;
}
static int deezer_curl_set_post_json(enum deezer_requests request, const char *param) {
    char *post_data = NULL;
    cJSON *json = cJSON_CreateObject();
    if (json == NULL) {
        return DC_ERROR_CJSON_CREATING;
    }
    switch (request) {
        case DC_PAGE_ALBUM:
            // {
            //     "ALB_ID": "302127",
            //     "lang": "en",
            //     "header": true,
            //     "tab": "overview",
            //     "nb": 50,
            //     "start": 0
            // }
            break;
        case DC_PAGE_HOME:
            // {
            //     "lang": "en",
            //     "tab": "home",
            //     "nb": 80,
            //     "start": 0
            // }
            break;
        case DC_PAGE_ARTIST:
            // {
            //   "art_id": "13",
            //   "lang": "en",
            //   "header": true,
            //   "tab": "overview",
            //   "nb": 50,
            //   "start": 0
            // }
            break;
        case DC_PAGE_PLAYLISTS:
            {
            // {
            //   "playlist_id": "14303680621",
            //   "lang": "en",
            //   "header": true,
            //   "start": 0,
            //   "nb": 500
            // }
            cJSON *playlist_id = cJSON_CreateString(param);
            cJSON *lang = cJSON_CreateString("en");
            cJSON *header = cJSON_CreateBool(true);
            cJSON *start = cJSON_CreateNumber(0);
            cJSON *nb = cJSON_CreateNumber(500);

            if (!playlist_id || !lang || !header || !start || !nb) {
                cJSON_Delete(json);
                return DC_ERROR_CJSON_CREATING;
            }
            cJSON_AddItemToObject(json, "playlist_id", playlist_id);
            cJSON_AddItemToObject(json, "lang", lang);
            cJSON_AddItemToObject(json, "header", header);
            cJSON_AddItemToObject(json, "start", start);
            cJSON_AddItemToObject(json, "nb", nb);

            post_data = cJSON_PrintUnformatted(json);
            break;
            }
        case DC_PAGE_TRACK:
            {
            // {
            //   "sng_id": "3135556"
            // }
            cJSON *sng_id = cJSON_CreateString(param);

            if (!sng_id) {
                cJSON_Delete(json);
                return DC_ERROR_CJSON_CREATING;
            }
            cJSON_AddItemToObject(json, "sng_id", sng_id);

            post_data = cJSON_PrintUnformatted(json);
            break;
            }
        case DC_PAGE_SEARCH:
            {
            // {
            //   "query": "the beatles",
            //   "QUERY": "the beatles",
            //   "start": 0,
            //   "nb": 20,
            //   "suggest": true,
            //   "artist_suggest": true,
            //   "top_tracks": true
            // }
            cJSON *query = NULL;
            cJSON *QUERY = NULL;
            cJSON *start = NULL;
            cJSON *nb = NULL;
            cJSON *suggest = NULL;
            cJSON *artist_suggest = NULL;
            cJSON *top_tracks = NULL;

            query = cJSON_CreateString(param);
            QUERY = cJSON_CreateString(param);
            start = cJSON_CreateNumber(0);
            nb = cJSON_CreateNumber(20);
            suggest = cJSON_CreateBool(true);
            artist_suggest = cJSON_CreateBool(true);
            top_tracks = cJSON_CreateBool(true);
            if (!query || !QUERY || !start || !nb || !suggest || !artist_suggest || !top_tracks) {
                cJSON_Delete(json);
                return DC_ERROR_CJSON_CREATING;
            }
            cJSON_AddItemToObject(json, "query", query);
            cJSON_AddItemToObject(json, "QUERY", QUERY);
            cJSON_AddItemToObject(json, "start", start);
            cJSON_AddItemToObject(json, "nb", nb);
            cJSON_AddItemToObject(json, "suggest", suggest);
            cJSON_AddItemToObject(json, "artist_suggest", artist_suggest);
            cJSON_AddItemToObject(json, "top_tracks", top_tracks);
            // pasamos a string el json
            post_data = cJSON_PrintUnformatted(json);
            break;
            }
        case DC_PAGE_PROFILE:
            {
            // {
            //   "profile_id": {{user_id}},
            //   "user_id": {{user_id}},
            //   "USER_ID": {{user_id}},
            //   "tab": "playlists",
            //   "nb": 40
            // }
            cJSON *profile_id = NULL;
            cJSON *user_id = NULL;
            cJSON *USER_ID = NULL;
            cJSON *tab = NULL;
            cJSON *nb = NULL;
            
            profile_id = cJSON_CreateNumber(user->id);
            user_id = cJSON_CreateNumber(user->id);
            USER_ID = cJSON_CreateNumber(user->id);
            tab = cJSON_CreateString("playlists");
            nb = cJSON_CreateNumber(40);
            if (!profile_id || !user_id || !USER_ID || !tab || !nb) {
                cJSON_Delete(json);
                return DC_ERROR_CJSON_CREATING;
            }
            cJSON_AddItemToObject(json, "profile_id", profile_id);
            cJSON_AddItemToObject(json, "user_id", user_id);
            cJSON_AddItemToObject(json, "USER_ID", USER_ID);
            cJSON_AddItemToObject(json, "tab", tab);
            cJSON_AddItemToObject(json, "nb", nb);
            // pasamos a string el json
            post_data = cJSON_PrintUnformatted(json);
            break;
            }
        case DC_PAGE_MEDIA_GET_URL:
            {
            // {
            //     "license_token": "{{license_token}}",
            //     "media": [
            //     {
            //         "type": "FULL",
            //         "formats": [
            //         {
            //             "cipher": "BF_CBC_STRIPE",
            //             "format": "MP3_128"
            //         }
            //         ]
            //     }
            //     ],
            //     "track_tokens": ["{track_token}"]
            // }
            cJSON *license_token = NULL;
            cJSON *media = cJSON_CreateArray();
            cJSON *mediaobj = cJSON_CreateObject();
            cJSON *type = NULL;
            cJSON *formats = cJSON_CreateArray();
            cJSON *formatobj = cJSON_CreateObject();
            cJSON *cipher = NULL;
            cJSON *format = NULL;
            cJSON *track_tokens = cJSON_CreateArray();
            cJSON *track_token = NULL;
            
            //string pelao que va en root
            license_token = cJSON_CreateString(client->license_token);
            // string que va en el mediaobj, que a su vez va en media
            type = cJSON_CreateString("FULL");
            cJSON_AddItemToObject(mediaobj, "type", type);
            // va en formatobj que va dentro de formats que va en media
            cipher = cJSON_CreateString("BF_CBC_STRIPE");
            cJSON_AddItemToObject(formatobj, "cipher", cipher);
            // va en formatobj que va dentro de formats que va en media
            format = cJSON_CreateString("MP3_320");
            cJSON_AddItemToObject(formatobj, "format", format);

            cJSON_AddItemToArray(formats, formatobj);
            cJSON_AddItemToObject(mediaobj, "formats", formats);

            cJSON_AddItemToArray(media, mediaobj);
            //string token que va dentro de track_tokens
            track_token = cJSON_CreateString(param);
            cJSON_AddItemToArray(track_tokens, track_token);

            //Ahora todo va en el objeto json
            cJSON_AddItemToObject(json, "license_token", license_token);
            cJSON_AddItemToObject(json, "media", media);
            cJSON_AddItemToObject(json, "track_tokens", track_tokens);
            // pasamos el json a string
            post_data = cJSON_PrintUnformatted(json);
            break;
            }
        case DC_PAGE_MEDIA_GET_URL_MULTI:
            {
            // {
            //     "license_token": "{{license_token}}",
            //     "media": [
            //     {
            //         "type": "FULL",
            //         "formats": [
            //         {
            //             "cipher": "BF_CBC_STRIPE",
            //             "format": "MP3_128"
            //         }
            //         ]
            //     }
            //     ],
            //     "track_tokens": ["{track_token}", "{track_token}"]
            // }
            cJSON *license_token = NULL;
            cJSON *media = cJSON_CreateArray();
            cJSON *mediaobj = cJSON_CreateObject();
            cJSON *type = NULL;
            cJSON *formats = cJSON_CreateArray();
            cJSON *formatobj = cJSON_CreateObject();
            cJSON *cipher = NULL;
            cJSON *format = NULL;
            cJSON *track_tokens = cJSON_Parse(param); // viene un array
                                                      // json en string
            //string pelao que va en root
            license_token = cJSON_CreateString(client->license_token);
            // string que va en el mediaobj, que a su vez va en media
            type = cJSON_CreateString("FULL");
            cJSON_AddItemToObject(mediaobj, "type", type);
            // va en formatobj que va dentro de formats que va en media
            cipher = cJSON_CreateString("BF_CBC_STRIPE");
            cJSON_AddItemToObject(formatobj, "cipher", cipher);
            // va en formatobj que va dentro de formats que va en media
            format = cJSON_CreateString("MP3_320");
            cJSON_AddItemToObject(formatobj, "format", format);

            cJSON_AddItemToArray(formats, formatobj);
            cJSON_AddItemToObject(mediaobj, "formats", formats);

            cJSON_AddItemToArray(media, mediaobj);

            //Ahora todo va en el objeto json
            cJSON_AddItemToObject(json, "license_token", license_token);
            cJSON_AddItemToObject(json, "media", media);
            cJSON_AddItemToObject(json, "track_tokens", track_tokens);
            // pasamos el json a string
            post_data = cJSON_PrintUnformatted(json);

            break;
            }
        case DC_GET_TOKEN:
            // No haremos POST asi que simplemente salimos
            // sin que se llegue a setear con setopt 
            cJSON_Delete(json);
            free(post_data);
            return DC_SUCCESS;
            break;
        default:
            // para el resto de casos borramos los parametros que hubiere
            // seteando con un json vacio.
            break;
    }
    // le pasamos el string a curl, en el caso de que no se haya seteado 
    // en el switch y venga a NULL, curl se configura en modo GET, mientras
    // que si tiene contenido, lo hace en modo POST 
    curl_easy_setopt(client->curl_handle, CURLOPT_COPYPOSTFIELDS, post_data);
    //LOG("Se setean los POSTFIELDS\n");
    // limpiamos
    cJSON_Delete(json);
    //LOG("Se libera el json\n");
    free(post_data);
    //LOG("Se libera post_data (podemos hacerlo al usar CURLOPT_COPYPOSTFIELDS)\n");
    return DC_SUCCESS;
}
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
static size_t writefilecallback(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    // Escribimos directamente a fichero conforme vamos recibiendo
    return fwrite(ptr, size, nmemb, stream);
}

static deecer_result_t deezer_init_playlist(playlist_t **playlist) {
    *playlist = calloc(1, sizeof(playlist_t));
    if (!*playlist) {
        return DC_ERROR_MEMORY_MAP_FAILED;
    }
    (*playlist)->id = 0;
    (*playlist)->nb_tracks = 0;
    (*playlist)->has_tracks = false;
    (*playlist)->title = NULL;
    (*playlist)->tracks = NULL;
    (*playlist)->user = NULL;
    return DC_SUCCESS;
}

// =========
// CLEANERS
// =========
void deezer_cleanup() {
        for (int i=0; i<nb_artists; i++) {
        deezer_free_artist(artists[i]);
    }
    for (int i=0; i<nb_albums; i++) {
        deezer_free_album(albums[i]);
    }
    for (int i=0; i<nb_playlists; i++) {
        deezer_free_playlist(playlists[i]);
    }
    for (int i=0; i<nb_tracks; i++) {
        deezer_free_track(tracks[i]);
    }
    deezer_free_user(user);
    deezer_free_client(client);

        if (artists) {
        free(artists);
        artists = NULL;
    }
    if (albums) {
        free(albums);
        albums = NULL;
    }
    if (playlists) {
        free(playlists);
        playlists = NULL;
    }
    if (tracks) {
        free(tracks);
        tracks = NULL;
    }

    return;
}


static void deezer_free_client(deezer_client_t *client) {
    free(client->session_id);
    free(client->api_token);
    free(client->license_token);
    free(client->mem.memory);
    curl_easy_cleanup(client->curl_handle);
    free(client);
}
static void deezer_free_user(user_t *user) {
    free(user->name);
    free(user->email);
    free(user->lovedtracks_id); 
    free(user->user_token);
    free(user);
}
static void deezer_free_track(track_t *track) {
    free(track->title);
    free(track->token);
    free(track->media_url);
    free(track);
}
static void deezer_free_artist(artist_t *artist) {
    free(artist->name);
    free(artist->tops);
    free(artist);
}
static void deezer_free_album(album_t *album) {
    free(album->title);
    free(album);
}
static void deezer_free_playlist(playlist_t *playlist) {
    playlist->nb_tracks = 0;
    playlist->has_tracks = false;
    playlist->id = 0;
    free(playlist->title);
    free(playlist->tracks);
    free(playlist);
}

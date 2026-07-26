//deezer_priv_api.c
/******************
 *
 * API privada de deezer,
 * implementacion
 *
 ******************/

#include "deezer_api.h"
#include "models.h"
#include "config.h"
#include "utils.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <cjson/cJSON.h>
#include <curl/easy.h>
#include <curl/typecheck-gcc.h>

/****************
 *
 *
 *
 ****************/
enum deezer_requests {
    DC_GET_TOKEN,
    DC_PAGE_SEARCH,
    DC_PAGE_PLAYLISTS,
    DC_PAGE_ALBUM,
    DC_PAGE_ARTIST,
    DC_PAGE_HOME,
    DC_PAGE_TRACK,
    DC_PAGE_PROFILE,

};
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
// Constructores
static int deezer_create_client(); 
static int deezer_create_user(); 
// funciones para gestionar la pool y convertir los json
// track
static int deezer_get_track_from_json(const cJSON *json_track, track_t **track);
static track_t *deezer_get_track_from_pool(int id);
static int deezer_add_track(track_t *track);
static bool deezer_track_exists(int id);
// artist
static int deezer_get_artist_from_json(const cJSON *json_artist, artist_t **artist);
static artist_t *deezer_get_artist_from_pool(int id);
static int deezer_add_artist(artist_t *artist);
static bool deezer_artist_exists(int id);
// album
static int deezer_get_album_from_json(const cJSON *json_album, album_t **album);
static album_t *deezer_get_album_from_pool(int id);
static int deezer_add_album(album_t *album);
static bool deezer_album_exists(int id);

// direct request to api
static int deezer_get_user_data(user_t *user);
static int deezer_get_track_data(track_t *track, int id);
static int deezer_get_artist_data(artist_t *artist, int id);
static int deezer_get_album_data(album_t *album, int id);
static int deezer_get_playlist_data(playlist_t *playlist, int id);
static int deezer_get_media_url(track_t *track);

// libcurl
static size_t writecallback(char *contents, size_t size, size_t nmemb, void *userp);
static int deezer_curl_set_init_options(); 
static int deezer_curl_set_headers(bool needToken); 
static int deezer_curl_set_url(enum deezer_requests request); 
static int deezer_curl_set_post_json(enum deezer_requests request, const char *param);

/*******************
 *
 * Definitions
 *
 ******************/

/**
 * Init deezer client. Make a request to get token 
 *
 * @param configuration struct 
 * @return error code 
 */
int deezer_init(config_t *config) {
    int err_code = 0;
    // creamos el objeto para el cliente
    err_code = deezer_create_client();
    if (DC_SUCCESS != err_code) {
        return err_code;
    }
    LOG("Hemos creado el cliente deezer. Un simple calloc.\n");
    
    // añadimos el arl antes de intentar conectar
    client->arl = config->arl;
    // inicializamos curl
    curl_global_init(CURL_GLOBAL_ALL);
    client->curl_handle = curl_easy_init();
    err_code = deezer_curl_set_init_options(); // seteamos opciones comunes
    if (DC_SUCCESS != err_code) {
        return err_code;
    }
    LOG("Curl inicializado\n");

    // inicializamos user
    err_code = deezer_create_user();
    if (DC_SUCCESS != err_code) {
        return err_code;
    }

    return DC_SUCCESS;
}

static int deezer_create_client()  {
    client = calloc(1, sizeof(deezer_client_t));
    if (client == NULL) {
        return DC_ERROR_INICIALIZATION_FAILED;
    }
    return DC_SUCCESS;
}
static int deezer_create_user() {
    // reservamos la memoria para la usuaria
    user = calloc(1, sizeof(user_t));
    if (user == NULL) {
        return DC_ERROR_INICIALIZATION_FAILED;
    }
    LOG("Memoria reservada para la usuaria con éxito.\n");
    
    // construimos la url
    deezer_curl_set_url(DC_GET_TOKEN);
    // añadimos los headers
    deezer_curl_set_headers(false);
    //ejecutamos la request
    client->curl_res = curl_easy_perform(client->curl_handle);
    LOG("Request ejecutada.\n");
    if (client->curl_res != CURLE_OK) {
        // algo ha fallado si se quiere comprobar,
        // el propio objeto tiene acceso a la respuesta
        // en client->curl_res
        return DC_ERROR_CURL_RESPONSE_ERROR;
    } else {
        // obtenemos la info del contenido obtenido en el anterior request
        // contenttype NO se libera, ya se encarga libcurl
        char *contenttype;
        client->curl_res = curl_easy_getinfo(client->curl_handle, CURLINFO_CONTENT_TYPE, &contenttype);
        if ((CURLE_OK == client->curl_res) && contenttype) {
            // comprobamos que hayamos recibido un json
            if (strstr(contenttype, "application/json")) {
                LOG("La solicitud ha ido bien y tenemos una respuesta en json.\n");
                // LOG("%s\n", client->mem.memory);
                cJSON *json = cJSON_Parse(client->mem.memory);
                if (json == NULL || cJSON_IsInvalid(json) || !cJSON_IsObject(json)) {
                    //liberamos el objeto json y salimos (petará si es null??)
                    //quiza deban estar separados los casos
                    cJSON_Delete(json);
                    return DC_ERROR_UNKNOWN;
                } else {
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
    LOG("user id: %d\n", user->id);
    LOG("user email: %s\n", user->email);
    LOG("user name: %s\n", user->name);
    LOG("session id: %s\n", client->session_id);
    LOG("api token: %s\n", client->api_token);
    return DC_SUCCESS;
}

/**
 * Add track to the tracks pool
 *
 * @param track The track pointer to add to the pool
 * @return RESULT CODE
 */
static int deezer_add_track(track_t *track) {
    LOG("Añadimos un track a la pool.\n");
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

    tracks[nb_tracks] = track;
    nb_tracks++;
    return DC_SUCCESS;
}
/**
 * Add artist to the tracks pool
 *
 * @param artist The artist pointer to add to the pool
 * @return RESULT CODE
 */
static int deezer_add_artist(artist_t *artist) {
    LOG("Añadimos un artist a la pool.\n");
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

    artists[nb_artists] = artist;
    nb_artists++;
    return DC_SUCCESS;
}

/**
 * Get track from pool
 *
 * @param track id
 * @return track_t pointer 
 */
static track_t *deezer_get_track_from_pool(int id) {
   for (int i=0; i < nb_tracks; i++) {
       if (id == tracks[i]->id) {
           return tracks[i];
       }
   }
   return NULL;
}
/**
 * Get artist from pool. Can use deezer_artist_exists first
 * to avoid null return
 * 
 * @param artist id
 * @return artist_t pointer
 */
static artist_t *deezer_get_artist_from_pool(int id) {
    for (int i=0; i < nb_artists; i++) {
        if (id == artists[i]->id) {
            return artists[i];
        }
    }
    return NULL;
}

/**
 * Get album from pool. Can use deezer_album_exists first
 * to avoid null return
 * 
 * @param album id
 * @return album_t pointer
 */
static album_t *deezer_get_album_from_pool(int id) {
    for (int i=0; i < nb_albums; i++) {
        if (id == albums[i]->id) {
            return albums[i];
        }
    }
    return NULL;
}
/**
 * Check if track id exists on pool
 *
 * @param track id 
 * @return boolean
 */
static bool deezer_track_exists(int id) {
    for (int i=0; i < nb_tracks; i++) {
       if (id == tracks[i]->id) {
           return true;
       }
    }
    return false;
}
/**
 * Check if artist id exists on pool
 *
 * @param artist id 
 * @return boolean
 */
static bool deezer_artist_exists(int id) {
    for (int i=0; i < nb_artists; i++) {
       if (id == artists[i]->id) {
           return true;
       }
    }
    return false;
}

/**
 * Check if album id exists on pool
 * 
 * @param album id
 * @return boolean
 */
static bool deezer_album_exists(int id) {
    for (int i=0; i < nb_albums; i++) {
        if (id == albums[i]->id) {
            return true;
        }
    }
    return false;
}

// public actions
content_t *deezer_search(const char *query) {
    // aqui guardaremos el contenido que retornaremos
    // tanto si tenemos exito como si no
    content_t *resp = content_create(1);

    LOG("Vamos a realizar una busqueda. query=%s\n", query);

    // seteamos los headers 
    deezer_curl_set_headers(true);
    // seteamos la url
    deezer_curl_set_url(DC_PAGE_SEARCH);
    // construimos el json del post
    deezer_curl_set_post_json(DC_PAGE_SEARCH, query);
    // comprobamos que tengamos el handle
    if (client->curl_handle == NULL) {
        LOG("el handle es null\n");
    }
    // liberamos el contenedor de los datos de la respuesta,
    // asi nos aseguramos que no nos queden restos de alguna
    // request anterior
    free(client->mem.memory);
    client->mem.memory = NULL;
    client->mem.size = 0;
    // realizamos la request y guardamos el codigo de error
    client->curl_res = curl_easy_perform(client->curl_handle);
    LOG("Realizada request de busqueda.\n");
    // comprobamos el codigo de respuesta.
    if (CURLE_OK != client->curl_res) {
        LOG("No hemos tenido una respues amigable\n");
        content_add_line(resp, "Parece que no hemos tenido una respuesta amigable al buscar.");
        return resp;
    }

    LOG("La request ha ido bien\n");
    char *contenttype = NULL;
    client->curl_res = curl_easy_getinfo(client->curl_handle, CURLINFO_CONTENT_TYPE, &contenttype);

    if ((CURLE_OK == client->curl_res) && contenttype) {
        LOG("contenttype: %s\n", contenttype);
        if (strstr(contenttype, "application/json")) {
            //tenemos un json
            // LOG("La respuesta de la busqueda:\n%s\n", client->mem.memory);
            // objeto que contiene el json global
            cJSON *json = cJSON_Parse(client->mem.memory);
            // LOG("json: %s\n", cJSON_Print(json));
            // Queremos quedarnos con la lista de TRACK
            cJSON *json_results = cJSON_GetObjectItem(json, "results"); // incluye todos los results
            // LOG("results: %s\n", cJSON_Print(json_results));
            cJSON *json_tracks = cJSON_GetObjectItem(json_results, "TRACK"); // incluye la info de tracks
            // LOG("track: %s\n", cJSON_Print(json_tracks));
            cJSON *json_data = cJSON_GetObjectItem(json_tracks, "data"); // array con los tracks

            LOG("Tenemos un json. Vamos a recorrer el array de tracks.\n");
            // LOG("data: %s\n", cJSON_Print(json_data));
            // debo devolver un content_t con una lista de tracks, para ello
            // tengo que añadirlos tracks a la pool y componer el content_t
            if (cJSON_IsArray(json_data)) {
                cJSON *iterator = NULL;
                LOG("Vamos a recorrer el array de tracks\n");
                cJSON_ArrayForEach(iterator, json_data) {
                    track_t *track = NULL;
                    if (deezer_get_track_from_json(iterator, &track) == DC_SUCCESS) {
                        LOG("Añadimos un track al content. %s \n", track->title);
                        content_add_track(resp, track);
                    }
                }
            } else {
                LOG("No es un array??\n");
            }
        }
    }
    LOG("devolvemos resp con %zu tracks\n", resp->numlines );
    return resp;
}

// getters (pide el objeto a la pool, y si no existe
// lo pedira a la api)
user_t *deezer_get_user(int id) {
    //not IMPLEMENTED
    user_t *user;
    return user;
}
track_t *deezer_get_track(int id) {
    // NOT IMPLEMENTED
    track_t *track;
    return track;
} 
artist_t *deezer_get_artist(int id) {
    // NOT IMPLEMENTED
    artist_t *artist;
    return artist;
}
album_t *deezer_get_album(int id) {
    // NOT IMPLEMENTED
    album_t *album;
    return album;
}
playlist_t *deezer_get_playlist(int id) {
    // NOt IMPLEMENTED
    playlist_t *playlist;
    return playlist;
}



// comprobadores
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
    LOG("deezer_track_is_valid-0\n");
    if (track == NULL) {
        return false;
    }
    if (0 >= track->id) {
        LOG("deezer_track_is_valid-1\n");
        return false;
    }
    LOG("deezer_track_is_valid-2\n");
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
bool deezer_playlist_is_valid(playlist_t *playlist) {
    // NOT IMPLEMENTED
    return false;
}

// destructores
void deezer_cleanup() {
    // NOT IMPLEMENTED;
    return;
}
void deezer_free_client(deezer_client_t *client);
void deezer_free_user(user_t *user);
void deezer_free_track(track_t *track);
void deezer_free_artist(artist_t *artist);
void deezer_free_album(album_t *album);
void deezer_free_playlist(playlist_t *playlist);

/***************
 *
 * JSON converters
 *
 ***************/
static int deezer_get_track_from_json(const cJSON *json_track, track_t **track) {
    LOG("Nos piden sacar un track de un json\n")
    if (!cJSON_IsObject(json_track)) {
        return DC_ERROR_CJSON_INVALID;
    }    
    cJSON *id = cJSON_GetObjectItem(json_track, "SNG_ID");
    LOG("id: %s\n", cJSON_Print(id));
    
    // una vez que tenemos el id, buscamos si ya tenemos ese track
    // en la pool, si es así, devolvemos el track desde la pool
    if (deezer_track_exists(atoi(id->valuestring))) {
        //buscar el track en la pool y retornarlo
        *track = deezer_get_track_from_pool(atoi(id->valuestring));
        return DC_SUCCESS;
    }

    // si no es asi, lo tenemos que desmembrar y añadir
    cJSON *title = cJSON_GetObjectItem(json_track, "SNG_TITLE");
    LOG("title: %s\n", cJSON_Print(title));
    cJSON *token = cJSON_GetObjectItem(json_track, "TRACK_TOKEN");
    cJSON *token_expire = cJSON_GetObjectItem(json_track, "TRACK_TOKEN_EXPIRE");
    cJSON *artist = cJSON_GetObjectItem(json_track, "ARTISTS"); // array de artists
    cJSON *album_id = cJSON_GetObjectItem(json_track, "ALB_ID");
    
    *track = calloc(1, sizeof(track_t));
    if (*track == NULL) {
        return DC_ERROR_MEMORY_MAP_FAILED;
    }
    if (cJSON_IsString(id)){
        (*track)->id = atoi(id->valuestring);
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
    
    if (cJSON_IsArray(artist))
    {
        cJSON *iterator = NULL;
        cJSON_ArrayForEach(iterator, artist) {
            artist_t *artist = NULL;
            if (deezer_get_artist_from_json(iterator, &artist) == DC_SUCCESS) {
                if (0 == (*track)->nb_artists) {
                    (*track)->artist = calloc(1, sizeof(artist_t*));
                } else {
                    (*track)->artist = realloc((*track)->artist, ((*track)->nb_artists + 1) * sizeof(artist_t*) );
                }
                (*track)->artist[(*track)->nb_artists] = artist;
                (*track)->nb_artists++;
            }
        }
    }
    LOG("Hemos encontrado un track en el json.\n");
    LOG("track->title: %s\n", (*track)->title)
    // media_url se consigue haciendo request a https://media.deezer.com/v1/get_url, la haremos cuando
    // se pida reproducir y entonces la guardaremos
    
    // Añadimos el track a la pool
    return deezer_add_track(*track);
}
static int deezer_get_artist_from_json(const cJSON *json_artist, artist_t **artist) {
    //LOG("Nos piden sacar un artist de este json.\n%s\n", cJSON_Print(json_artist));
    if (!cJSON_IsObject(json_artist)) {
        return DC_ERROR_CJSON_INVALID;
    }
    cJSON *id = cJSON_GetObjectItem(json_artist, "ART_ID");
    if (deezer_artist_exists(atoi(id->valuestring))) {
        *artist = deezer_get_artist_from_pool(atoi(id->valuestring));
        return DC_SUCCESS;
    }
    cJSON *name = cJSON_GetObjectItem(json_artist, "ART_NAME");
    *artist = calloc(1, sizeof(artist_t));
    if (*artist == NULL) {
        return DC_ERROR_MEMORY_MAP_FAILED;
    }
    if (cJSON_IsString(id)) {
        (*artist)->id = atoi(id->valuestring);
    }
    if (cJSON_IsString(name)) {
        (*artist)->name = strdup(name->valuestring);
    }
    LOG("Añadimos a la pool. artist: %s\n", (*artist)->name);
    return deezer_add_artist(*artist);
}
static album_t* deezer_convert_json_to_album(cJSON *json_album) {
    // NOT IMPLEMENTED
    album_t *album = NULL;
    return album;
}

/****************
 *
 * CURL helpers
 *
 * *************/

static int deezer_curl_set_init_options() {
    if (client->curl_handle == NULL) {
        return DC_ERROR_INICIALIZATION_FAILED;
    }
    curl_easy_setopt(client->curl_handle, CURLOPT_WRITEFUNCTION, writecallback);
    curl_easy_setopt(client->curl_handle, CURLOPT_WRITEDATA, &client->mem);
    curl_easy_setopt(client->curl_handle, CURLOPT_USERAGENT, "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/133.0.0.0 Safari/537.36");
    LOG("deezer_curl_set_init_options()\n");
    return DC_SUCCESS;

}
static int deezer_curl_set_headers(bool needToken) {
    LOG("Entrando en ... deezer_curl_set_headers(bool needToken)\n");
    // Creamos los headers [man CURLOPT_HTTPHEADER]
    struct curl_slist *list;
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
    LOG("Headers seteados.\n");
    return DC_SUCCESS;
    
}
static int deezer_curl_set_url(enum deezer_requests request) {
    LOG("Entrando en ... deezer_curl_set_l(enum deezer_requests request)\n");
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
        default:
            break;
    }
    if (url == NULL) {
        return DC_ERROR_UNKNOWN;
    }

    LOG("La url final es: %s\n", url);
    // creamos las opciones de curl
    curl_easy_setopt(client->curl_handle, CURLOPT_URL, url);
    return DC_SUCCESS;
}
static int deezer_curl_set_post_json(enum deezer_requests request, const char *param) {
    LOG("Entrando en... deezer_curl_set_post_json()\n");
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
            // {
            //   "playlist_id": "14303680621",
            //   "lang": "en",
            //   "header": true,
            //   "start": 0,
            //   "nb": 500
            // }
            break;
        case DC_PAGE_TRACK:
            // {
            //   "sng_id": "3135556"
            // }
            break;
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
            // {
            //   "profile_id": {{user_id}},
            //   "user_id": {{user_id}},
            //   "USER_ID": {{user_id}},
            //   "tab": "playlists",
            //   "nb": 40
            // }
            break;
        default:
            // para el resto de casos borramos los parametros que hubiere
            // seteando con un json vacio.
            break;
    }
    LOG("Hemos pasado la ristra de switch.\n");
    // le pasamos el string a curl, en el caso de que no se haya seteado 
    // en el switch y venga a NULL, curl se configura en modo GET, mientras
    // que si tiene contenido, lo hace en modo POST 
    curl_easy_setopt(client->curl_handle, CURLOPT_COPYPOSTFIELDS, post_data);
    LOG("Se setean los POSTFIELDS\n");
    // limpiamos
    cJSON_Delete(json);
    LOG("Se libera el json\n");
    free(post_data);
    LOG("Se libera post_data (podemos hacerlo al usar CURLOPT_COPYPOSTFIELDS)\n");
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


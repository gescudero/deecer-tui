#include "deezer_api.h"
#include "models.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>
#include <cjson/cJSON.h>

// el objeto curl, el manejador del cotarro
static CURL *curl_handle;
// objeto que contiene la respuesta (OK, ERROR, ETC)
static CURLcode curl_res;
// funcion callback necesaria para curl, es con la que recibimos los datos
// a pesar de llamarse write, en realidad seria lo que leemos nosotros
static size_t writecallback(char *contents, size_t size, size_t nmemb, void *userp);
// funcion para convertir un objeto json track a un track_t
static track_t* deezer_convert_json_to_track(cJSON *json_track);
static artist_t* deezer_convert_json_to_artist(cJSON *json_artist);
static album_t* deezer_convert_json_to_album(cJSON *json_album);
static void deezer_track_free(track_t *track);
static void deezer_artist_free(artist_t *artist);
static void deezer_album_free(album_t *album);

void deezer_init() {
    // Inicializacion global recomendable, no tengo muy claro que hace
    curl_global_init(CURL_GLOBAL_ALL);
    // Inicializacion de nuestro objeto curl
    curl_handle = curl_easy_init();
}
void deezer_cleanup() {
    curl_easy_cleanup(curl_handle);
    curl_global_cleanup();
}


content_t* deezer_search(const char *query) {
    // el objeto que contendrá la respuesta a nuestra peticion curl
    // lo inicializo a 0 y NULLS
    struct memory chunk = {0};

    // devolvemos un puntero de tipo content_t
    content_t *resp = content_create(1);

    if (resp == NULL) {
        return resp;
    }

    // Vamos a ello
    if (curl_handle) {
        // Mediante setopt vamos configurando nuestra peticion
        // 1. le damos la url
        char *url = NULL;
        asprintf(&url, "https://api.deezer.com/search?q=%s", query);
        curl_easy_setopt(curl_handle, CURLOPT_URL, url); 
        // 2. le pasamos nuestra funcion de callback para que nos responda
        curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, writecallback);
        // 3. le pasamos el objeto donde debe escribir la respuesta
        curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &chunk);

        // Hacemos la peticion
        curl_res = curl_easy_perform(curl_handle);

        // comprobamos que tal ha ido
        if (curl_res != CURLE_OK) {
            // ha habido un erro, ya veremos a ver como lo notificamos
            // este es el ejemplo que pone Daniel Stenberg 
            fprintf(stderr, "curl_easy_perform() returned error %s\n", 
                    curl_easy_strerror(curl_res));
            content_add_line(resp, "Se ha producido un error. Intentalo mas tarde.");
        } else {
            // comprobamos el content-type de la descarga
            char *contenttype;
            curl_res = curl_easy_getinfo(curl_handle, CURLINFO_CONTENT_TYPE, &contenttype);
            if ((CURLE_OK == curl_res) && contenttype) {
                // co0mprobamos si hemos recibido un json
                if (strstr(contenttype, "application/json")) {
                    
                    // creamos el objeto que contendrá el json
                    cJSON *json = cJSON_Parse(chunk.memory);
                    // comprobamos que el objeto JSON cumpla nuestras necesidades
                    if (json == NULL) {
                        const char *error_ptr = cJSON_GetErrorPtr();
                        content_add_line(resp, "Error leyendo el json");
                        content_add_line(resp, error_ptr);
                    } else if (cJSON_IsInvalid(json) || !cJSON_IsObject(json)){
                        // El json no se ha parseado bien o hemos recibido algo chunguer
                        // nosotros esperamos un Objeto, asi que si no es asi
                        // Cancelamos y devolvemos un mensajito de error
                        content_add_line(resp, "el objeto json es invalido, un poco cojo");
                    } else {
                        // access to data
                        cJSON *data = cJSON_GetObjectItemCaseSensitive(json, "data");
                        cJSON *num_objects = cJSON_GetObjectItem(json, "total");
                        // el objeto que contiene el numero total de items que devuelve 
                        // la query en distintas pages. Como maximo recibiremos 25 items
                        // por consulta
                        if (num_objects != NULL) {
                            if (cJSON_IsNumber(num_objects)) {
                                /**
                                 * comprobado que tenemos el numero de resultados
                                 * totales en la variable num_objects
                                char *text;
                                asprintf(&text, "Hay un total de %d resultados. Mostramos los 25 primeros\n", 
                                        num_objects->valueint);
                                content_add_line(resp, text);
                                free(text);
                                **/
                            } else {
                                content_add_line(resp, "Parece que no es numerico");
                            }
                        } else {
                            content_add_line(resp, "el objeto num_objects es NULL");
                        }
                        // el objeto que contiene todos los datos, es un Array de items
                        if (data !=NULL) {
                            if (cJSON_IsArray(data)) {
                                //Hemos recibido un array, vamos bien
                                cJSON *iterator = NULL;
                                cJSON_ArrayForEach(iterator, data) {
                                    fprintf(stderr, "Vamos a convertir el json a track\n");
                                    // convertimos a objeto track 
                                    track_t *trackp = deezer_convert_json_to_track(iterator);
                                    if (trackp != NULL) {
                                        fprintf(stderr, "track: %s\n", trackp->title);
                                        content_add_line(resp, trackp->title);
                                    }
                                    fprintf(stderr, "Vamos a liberar\n");
                                    deezer_track_free(trackp);
                                }
                            }
                        }
                    }
                    cJSON_Delete(json);
                } else {
                    char *text;
                    asprintf(&text, "Hemos recibido Content-Type: %s\n"
                            "Esperabamos otra cosa\n", contenttype);
                    content_add_line(resp, text);
                    free(text);
                }
            } else {
                content_add_line(resp, "No sabemos el Content-Type");
            }
        }
    }
    if (chunk.memory != NULL) {
        free(chunk.memory);
        chunk.memory = NULL;
        chunk.size = 0;
    }
    return resp;
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
    struct memory *mem = (struct memory *)userp;

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

static track_t* deezer_convert_json_to_track(cJSON *json_track) {
    track_t *track = calloc(1, sizeof(track_t));
    if (track == NULL) {
        return NULL;
    }
    cJSON *id = cJSON_GetObjectItem(json_track, "id");
    cJSON *title = cJSON_GetObjectItem(json_track, "title");
    cJSON *title_short = cJSON_GetObjectItem(json_track, "title_short");
    cJSON *artist = cJSON_GetObjectItem(json_track, "artist");
    cJSON *album = cJSON_GetObjectItem(json_track, "album");
    cJSON *preview = cJSON_GetObjectItem(json_track, "preview");
    cJSON *track_token = cJSON_GetObjectItem(json_track, "track_token");

    if (cJSON_IsNumber(id)) {
        track->id = id->valueint;
    }
    if (cJSON_IsString(title)) {
        track->title = strdup(title->valuestring);
    }
    if (cJSON_IsString(title_short)) {
        track->title_short = strdup(title_short->valuestring);
    }
    if (cJSON_IsObject(artist)) {
        track->artist = deezer_convert_json_to_artist(artist);
    } else {
        track->artist = NULL;
    }
    if (cJSON_IsObject(album)) {
        track->album = deezer_convert_json_to_album(album);
    } else {
        track->album = NULL;
    }
    if (cJSON_IsString(preview)) {
        track->preview = strdup(preview->valuestring);
    }
    if (cJSON_IsString(track_token)) {
        track->track_token = strdup(track_token->valuestring);
    }
    fprintf(stderr, "json_track: %p; json_artist: %p; json_album: %p\n", track, track->artist, track->album);
    return track;
}

static artist_t* deezer_convert_json_to_artist(cJSON *json_artist) {
    artist_t *artist = calloc(1, sizeof(artist_t));
    if (artist == NULL) {
        return NULL;
    }
    cJSON *id = cJSON_GetObjectItem(json_artist, "id");
    cJSON *name = cJSON_GetObjectItem(json_artist, "name");
    cJSON *link = cJSON_GetObjectItem(json_artist, "link");
    cJSON *tracklist = cJSON_GetObjectItem(json_artist, "tracklist");
    if (cJSON_IsNumber(id)) {
        artist->id = id->valueint;
    }
    if (cJSON_IsString(name)) {
        artist->name = strdup(name->valuestring);
    }
    if (cJSON_IsString(link)) {
        artist->link = strdup(link->valuestring);
    }
    if (cJSON_IsString(tracklist)) {
        artist->tracklist = strdup(tracklist->valuestring);
    }
 
    return artist;
}
static album_t* deezer_convert_json_to_album(cJSON *json_album) {
    album_t *album = calloc(1, sizeof(album_t));
    if (album == NULL) {
        return NULL;
    }
    cJSON *id = cJSON_GetObjectItem(json_album, "id");
    cJSON *title = cJSON_GetObjectItem(json_album, "title");
    cJSON *md5_image = cJSON_GetObjectItem(json_album, "md5_image");
    cJSON *tracklist = cJSON_GetObjectItem(json_album, "tracklist");
    if (cJSON_IsNumber(id)) {
        album->id = id->valueint;
    }
    if (cJSON_IsString(title)) {
        album->title = strdup(title->valuestring);
    }
    if (cJSON_IsString(md5_image)) {
        album->md5_image = strdup(md5_image->valuestring);
    }
    if (cJSON_IsString(tracklist)) {
        album->tracklist = strdup(tracklist->valuestring);
    }
    return album;
}
static void deezer_track_free(track_t *track) {
    fprintf(stderr, "Entramos en track free. %p\n", track);
    if (track == NULL) {
        return;
    }
    if (track->title) {
        free(track->title);
        track->title = NULL;
    }
    if (track->title_short) {
        free(track->title_short);
        track->title_short = NULL;
    }
    if (track->track_token) {
        free(track->track_token);
        track->track_token = NULL;
    }
    if (track->preview) {
        free(track->preview);
        track->preview = NULL;
    }
    if (track->artist) {
        deezer_artist_free(track->artist);
    }
    if (track->album) {
        deezer_album_free(track->album);
    }
    free(track);
    track = NULL;
}
static void deezer_artist_free(artist_t *artist) {
    fprintf(stderr, "Entramos en artist free. %p\n", artist);
    if (artist == NULL) {
        return;
    }
    if (artist->name) {
        free(artist->name);
        artist->name = NULL;
    }
    if (artist->link) {
        free(artist->link);
        artist->link = NULL;
    }
    if (artist->tracklist) {
        free(artist->tracklist);
        artist->tracklist = NULL;
    }
    free(artist);
    artist = NULL;
}
static void deezer_album_free(album_t *album) {
    fprintf(stderr, "Entramos en album free. %p\n", album);
    if (album == NULL) {
        return;
    }
    if (album->title) {
        free(album->title);
        album->title = NULL;
    }
    if (album->md5_image) {
        free(album->md5_image);
        album->md5_image = NULL;
    }
    if (album->tracklist) {
        free(album->tracklist);
        album->tracklist = NULL;
    }
    free(album);
    album = NULL;
}

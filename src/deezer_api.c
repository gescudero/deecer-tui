#include "deezer_api.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>
#include <cjson/cJSON.h>

// el objeto curl, el manejador del cotarro
CURL *curl_handle;
// objeto que contiene la respuesta (OK, ERROR, ETC)
CURLcode curl_res;
// funcion callback necesaria para curl, es con la que recibimos los datos
// a pesar de llamarse write, en realidad seria lo que leemos nosotros
static size_t writecallback(char *contents, size_t size, size_t nmemb, void *userp);

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
                                    cJSON *title = cJSON_GetObjectItem(iterator, "title");
                                    cJSON *artist = cJSON_GetObjectItem(iterator, "artist");
                                    if (cJSON_IsString(title) && cJSON_IsObject(artist)) {
                                        cJSON *artist_name = cJSON_GetObjectItem(artist, "Name");
                                        if (cJSON_IsString(artist_name)) {
                                            char *tmp_str;
                                            asprintf(&tmp_str, "%s:%s", artist_name->valuestring, title->valuestring);
                                            content_add_line(resp, tmp_str);
                                            free(tmp_str);
                                        } else {
                                            content_add_line(resp, title->valuestring);
                                        }
                                    }
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



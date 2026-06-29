#include "deezer_api.h"
#include <curl/curl.h>
#include <curl/easy.h>
#include <curl/typecheck-gcc.h>
#include <stdio.h>
#include <string.h>

// el objeto curl, el manejador del cotarro
CURL *curl_handle;
// objeto que contiene la respuesta (OK, ERROR, ETC)
CURLcode curl_res;
// el objeto que contendrá la respuesta a nuestra peticion curl
struct memory chunk;

// funcion callback necesaria para curl, es con la que recibimos los datos
// a pesar de llamarse write, en realidad seria lo que leemos nosotros
static size_t writecallback(char *contents, size_t size, size_t nmemb, void *userp);

void deezer_init() {
    // Inicializacion global recomendable, no tengo muy claro que hace
    curl_global_init(CURL_GLOBAL_ALL);
    // Inicializacion de nuestro objeto curl
    curl_handle = curl_easy_init();
    // Inicializamos nuestro chunk
    chunk.memory = NULL;
    chunk.size = 0;

}
void deezer_cleanup() {
    curl_easy_cleanup(curl_handle);
    curl_global_cleanup();
}


char* deezer_search(const char *query) {
    // de momento devuelvo algo sencillo
    // la concatenacion de dos strings
      char *resp = NULL;
//    asprintf(&resp, "Esta es la query que me has hecho %s", query);
//    return resp; //el que llame a la funcion debe liberar

    // Vamos a ello
    if (curl_handle) {
        // Mediante setopt vamos configurando nuestra peticion
        // 1. le damos la url
        char *url = NULL;
        asprintf(&url, "https://api.deezer.com/artist/%s", query);
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
            asprintf(&resp, "Se ha producido un error. Intentalo mas tarde.");
        } else {
            // comprobamos el content-type de la descarga
            char *contenttype;
            curl_res = curl_easy_getinfo(curl_handle, CURLINFO_CONTENT_TYPE, &contenttype);
            if ((CURLE_OK == curl_res) && contenttype) {
                // co0mprobamos si hemos recibido un json
                if (strstr(contenttype, "application/json")) {
                    asprintf(&resp, "Ahora si que hemos detectado el json: %s", chunk.memory);
                } else {
                    asprintf(&resp, "Hemos recibido Content-Type: %s\n"
                            "Esperabamos otra cosa\n", contenttype);
                }
            } else {
                asprintf(&resp, "No sabemos el Content-Type");
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



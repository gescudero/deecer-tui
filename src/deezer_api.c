#include "deezer_api.h"

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



char* deezer_search(const char *query) {
    char *resp = "Esta es la respuesta.";

    return resp;
}

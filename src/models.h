// models.h
#ifndef MODELS_H
#define MODELS_H

typedef struct config_t config_t;
typedef struct memory_t memory_t;
typedef struct content_t content_t;
typedef struct deezer_client_t deezer_client_t;
typedef struct user_t user_t;
typedef struct track_t track_t;
typedef struct album_t album_t;
typedef struct artist_t artist_t;
typedef struct playlist_t playlist_t;

// valores de resultados, inspirado en VkResult de VULKAN
enum deecer_result {
    DC_SUCCESS = 0,
    DC_NOT_READY = 1,
    DC_TIMEOUT = 2,
    DC_ERROR_INICIALIZATION_FAILED = -1,
    DC_ERROR_MEMORY_MAP_FAILED = -2,
    DC_ERROR_UNKNOWN = -5,
    DC_ERROR_CURL_RESPONSE_ERROR = -6,
};

#endif

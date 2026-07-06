/**
 * Funciones para desencriptar el fichero
 * de audio descargado de deezer, por el momento
 * somos capaces de desencriptar usando como libreria 
 * el crypto.rs de Minuga-RC/deezer-tui
 **/
#ifndef CRYPTO_H
#define CRYPTO_H

#include <stdio.h>
extern unsigned char* decrypt_audio(const char *track_id,
                                    const unsigned char *data,
                                    size_t data_len,
                                    size_t out_len);
extern void free_decrypted(unsigned char *ptr, size_t len);

#endif

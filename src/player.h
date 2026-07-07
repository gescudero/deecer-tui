#ifndef PLAYER_H
#define PLAYER_H

#include <stdbool.h>
#include <mpv/client.h>

bool player_init();
void player_end();
void player_openurl(char *url);
void player_openplaylist(char *url);
void player_playfile(char *audio_data, size_t audio_size);
void player_play(); // resume play
void player_pause();
void player_stop();
void player_back();
void player_forward();
#endif

// player.h 

#ifndef PLAYER_H
#define PLAYER_H

#include <stdbool.h>

extern int player_running;
bool player_init();
void player_end();
void player_openurl(char *url);
void player_openplaylist(char *url);
void player_play(); // resume play
void player_pause();
void player_stop();
void player_back();
void player_forward();
#endif

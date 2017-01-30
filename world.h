#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <stdbool.h>

#define WORLD_TIMEOUT 4
#define WORLD_CAPACITY 1024

typedef struct world {
    struct player* players[WORLD_CAPACITY];
    char stage[20][80];
} world_t;

void  world_init();
void* world_loop(void* world_void);

typedef struct player {
    int x;
    int dx;
    int y;
    int dy;

    int wand_x;
    int wand_y;

    pthread_t server;
    long int ping;
} player_t;

player_t* player_init(pthread_t server);
void player_tick(player_t* p);
void player_char(player_t* p, char c);
void player_draw(player_t* p, char buf[20][80], int width, int height);

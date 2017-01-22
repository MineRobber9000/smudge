#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

#define TIMEOUT 4

typedef struct world {
    struct player* players[1024];
} world_t;

void* world_loop(void* world_void);

typedef struct player {
    int x;
    int y;
    world_t* world;
    pthread_t server;
    long int ping;
} player_t;

player_t* player_init(world_t* world, pthread_t server);
void player_tick(player_t* p);
void player_char(player_t* p, char c);
void player_draw(player_t* p, char* buf, int width, int height);

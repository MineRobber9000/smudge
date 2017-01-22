#include "player.h"

void world_init(world_t* world) {
    pthread_t thread;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    if (pthread_create(&thread, &attr, world_loop, (void*)world) != 0) {
        fprintf(stderr, "Could not launch world.");
        exit(1);
    }
    pthread_attr_destroy(&attr);
}

void* world_loop(void* world_void) {
    world_t* world = (world_t*) world_void;
    int i;
    long int ping;
    while (1) {
        usleep(16000);
        ping = (long int) time(NULL);
        for (i=0; i<1024; i++) {
            if (world->players[i] != NULL) {
                fflush(stdout);
                if (ping - world->players[i]->ping > TIMEOUT) {
                    // Just in case!
                    pthread_cancel(world->players[i]->server);
                    free(world->players[i]);
                    world->players[i] = NULL;
                } else {
                    player_tick(world->players[i]);
                }
            }
        }
    }
}

player_t* player_init(world_t* world, pthread_t server) {
    player_t* p = malloc(sizeof(player_t));
    p->world = world;
    p->server = server;
    p->ping = time(NULL);
    for (int i=0; i<1024; i++) {
        if (p->world->players[i] == NULL) {
            p->world->players[i] = p;
            break;
        }
    }

    p->x = 5;
    p->y = 5;

    return p;
}

void player_tick(player_t* p) {
    // Occurs at 60fps, hopefully
    p->x %= 78;
    p->y %= 18;
    if (p->x < 0) p->x += 78;
    if (p->y < 0) p->y += 18;
}

void player_char(player_t* p, char c) {
    // Respond to keypresses
    switch (c) {
    case 'w':
        p->y--;
        break;
    case 'a':
        p->x--;
        break;
    case 's':
        p->y++;
        break;
    case 'd':
        p->x++;
        break;
    }
}

void player_draw(player_t* p, char* buf, int width, int height) {
    // Render the board onto buf, which has size (width * height)
    memset(buf, ' ', width*height);
    for (int i=0; i<1024; i++) {
        if (p->world->players[i] != NULL) {
            buf[p->world->players[i]->x+p->world->players[i]->y*width] = 'o';
        }
    }

    // Pretty border
    for (int i=0; i<width; i++) {
        buf[i] = '-';
        buf[width*(height-1) + i] = '-';
    }
    for (int j=0; j<height; j++) {
        buf[j*width] = '|';
        buf[j*width + width - 1] = '|';
    }
    buf[0] = '+';
    buf[width*height-1] = '+';
    buf[width-1] = '+';
    buf[width*(height-1)] = '+';
}

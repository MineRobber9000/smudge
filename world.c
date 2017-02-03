#include "world.h"

world_t world;

bool bound_check(int x, int y){
    if (x >= 0 && x < WORLD_WIDTH && y >= 0 && y < WORLD_HEIGHT) {
        return true;
    }
    return false;
}

void world_init() {
    pthread_t thread;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    if (pthread_create(&thread, &attr, world_loop, NULL) != 0) {
        fprintf(stderr, "Could not launch world.\n");
        exit(1);
    }
    pthread_attr_destroy(&attr);

    memset(world.stage, ' ', WORLD_WIDTH*WORLD_HEIGHT);
    for (int i= 0; i<80; i++) world.stage[19][i] = '#';
    for (int i= 0; i<80; i++) world.stage[ 0][i] = '#';
    for (int i= 0; i<20; i++) world.stage[i][ 0] = '#';
    for (int i= 0; i<20; i++) world.stage[i][79] = '#';

    for (int i=10; i<70; i++) world.stage[16][i] = '#';
    for (int i=20; i<60; i++) world.stage[14][i] = '=';
    for (int i=30; i<40; i++) world.stage[12][i] = '=';
}

void* world_loop(void* ign) {
    int i;
    long int ping;
    while (1) {
        usleep(16000 * 6);
        ping = (long int) time(NULL);
        for (i=0; i<WORLD_CAPACITY; i++) {
            if (world.players[i] != NULL) {
                fflush(stdout);
                if (ping - world.players[i]->ping > WORLD_TIMEOUT) {
                    pthread_cancel(world.players[i]->server);
                    free(world.players[i]);
                    world.players[i] = NULL;
                } else {
                    player_tick(world.players[i]);
                }
            }
        }
    }
}

player_t* player_init(pthread_t server) {
    player_t* p = malloc(sizeof(player_t));
    p->server = server;
    p->ping = time(NULL);
    int i;

    while (1) {
        for (i=0; i<WORLD_CAPACITY; i++) {
            if (world.players[i] == NULL) {
                world.players[i] = p;
                break;
            }
        }
        if (i < WORLD_CAPACITY) {
            break;
        }
    }

    p->x = 5;
    p->dx = 0;
    p->y = 5;
    p->dy = 0;

    p->wand_x = 1;
    p->wand_y = 0;

    return p;
}

void player_tick(player_t* p) {
    if (p->dy < 3) {
        p->dy++;
    }
    int sdx = p->dx < 0 ? -1 : p->dx > 0 ? +1 : 0;
    int sdy = p->dy < 0 ? -1 : p->dy > 0 ? +1 : 0;

    for (int i=0; i < abs(p->dy); i++) {
        if (world.stage[p->y + sdy][p->x] == ' ') {
            p->y += sdy;
        } else {
            p->dy = 0;
            break;
        }
    }
    for (int i=0; i < abs(p->dx); i++) {
        char t = world.stage[p->y][p->x + sdx];
        if (t == ' ') {
            p->x += sdx;
        } else if (t == '/'  && world.stage[p->y - 1][p->x + sdx] == ' '){
            p->x += sdx;
            if (sdx > 0){
                p->y -= 1;
            }
        } else if (t == '\\' && world.stage[p->y - 1][p->x + sdx] == ' ') {
            p->x += sdx;
            if (sdx < 0){
                p->y -= 1;
            }
        } else {
            p->dx = 0;
            break;
        }
    }
    // loop player position if OOB
    if (p->x < 0){
        p->x = WORLD_WIDTH-1;
    } else if (p->x >= WORLD_WIDTH){
        p->x = 0;
    }
    if (p->y < 0){
        p->y = WORLD_HEIGHT-1;
    } else if (p->y >= WORLD_HEIGHT){
        p->y = 0;
    }
}

void player_char(player_t* p, char c) {
    // Respond to keypresses
    // fprintf(stderr, "char: %x\n", c);
    switch (c) {
    case 'w':
    case 'k':
        p->dy = -4;
        break;
    case 'a':
    case 'h':
        if (p->dx > 0) {
            p->dx = 0;
        } else {
            p->dx--;
        }
        break;
    case 's':
    case 'j':
        if (p->wand_x == -1 && p->wand_y == 0) {
            p->wand_x = 0; p->wand_y = -1;
        } else if (p->wand_x == 0 && p->wand_y == -1) {
            p->wand_x = 1; p->wand_y = 0;
        } else if (p->wand_x == 1 && p->wand_y == 0) {
            p->wand_x = 0; p->wand_y = 1;
        } else if (p->wand_x == 0 && p->wand_y == 1) {
            p->wand_x = -1; p->wand_y = 0;
        }
        break;
    case 'd':
    case 'l':
        if (p->dx < 0) {
            p->dx = 0;
        } else {
            p->dx++;
        }
        break;

    case '=':
        if (world.stage[p->y+p->wand_y][p->x+p->wand_x] == ' ') {
            world.stage[p->y+p->wand_y][p->x+p->wand_x] = '=';
        }
        break;
    case 'x':
        if (world.stage[p->y+p->wand_y][p->x+p->wand_x] != '#') {
            world.stage[p->y+p->wand_y][p->x+p->wand_x] = ' ';
        }
        break;
    case '/':
        if (world.stage[p->y+p->wand_y][p->x+p->wand_x] == ' ') {
            world.stage[p->y+p->wand_y][p->x+p->wand_x] = '/';
        }
        break;
    case '\\':
        if (world.stage[p->y+p->wand_y][p->x+p->wand_x] == ' ') {
            world.stage[p->y+p->wand_y][p->x+p->wand_x] = '\\';
        }
        break;

/*
    case 'i':
        p->wand_x = 0;
        p->wand_y = -1;
        break;
    case 'j':
        p->wand_x = -1;
        p->wand_y = 0;
        break;
    case 'k':
        p->wand_x = 0;
        p->wand_y = 1;
        break;
    case 'l':
        p->wand_x = 1;
        p->wand_y = 0;
        break;
*/
    }
}


void player_draw(player_t* p, char buf[WORLD_HEIGHT][WORLD_WIDTH], int width, int height) {
    // Render the board onto buf, which has size (width * height)
    memcpy(buf, world.stage, width*height);
    //memset(buf, ' ', width*height);
    for (int i=0; i<WORLD_CAPACITY; i++) {
        if (world.players[i] != NULL) {
            int x = world.players[i]->x;
            int y = world.players[i]->y;
            int wx = world.players[i]->wand_x;
            int wy = world.players[i]->wand_y;
            if (bound_check(x, y)){
                buf[y][x] = 'o';
            }
            if (bound_check(x+wx, y+wy)){
                buf[y+wy][x+wx] = '*';
            }
        }
    }

    // Pretty border
    // for (int i=0; i<width; i++) {
    //     buf[0][i] = '-';
    //     buf[height-1][i] = '-';
    // }
    // for (int j=0; j<height; j++) {
    //     buf[j][0] = '|';
    //     buf[j][width-1] = '|';
    // }
    // buf[0][0] = '+';
    // buf[height-1][width-1] = '+';
    // buf[0][width-1] = '+';
    // buf[height-1][0] = '+';
}

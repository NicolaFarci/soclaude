#ifndef CROCODILE_H
#define CROCODILE_H

#include "game.h"
#include "buffer.h"

typedef struct {
    CircularBuffer *buffer;
    WINDOW* game_win;
    RiverLane* lane;
    int offset;
    int index;
} CrocodileArgs;

void init_lanes(RiverLane lanes[]);
void *crocodile_thread(void *arg);
void draw_crocodile(WINDOW *win, Entity *crocodile);
void clear_crocodile(WINDOW *win, Entity *crocodile);
void crocodile_init(Entity *crocodile, RiverLane *lane,int offset);
void *crocodile_projectile_thread(void *arg);

#endif

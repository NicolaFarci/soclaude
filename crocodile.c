#include "crocodile.h"
#include "map.h"
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

void init_lanes(RiverLane lanes[]) {
    
    // First lane direction is random
    lanes[0].direction = (rand() % 2 == 0) ? 1 : -1;
    lanes[0].speed = 60000;
    lanes[0].y = MAP_HEIGHT - 6;
    lanes[0].can_spawn = true;
    
    // Alternate directions for subsequent lanes
    for (int i = 1; i < NUM_RIVER_LANES; i++) {
        lanes[i].direction = -lanes[i-1].direction;
        lanes[i].speed = lanes[i-1].speed - 5000;
        lanes[i].y = lanes[i-1].y + FROG_HEIGHT;
        lanes[i].can_spawn = true;
    }
}

void crocodile_init(Entity *crocodile, RiverLane *lane) {
    
    crocodile->type = ENTITY_CROCODILE;
    crocodile->width = CROCODILE_WIDTH;
    crocodile->height = CROCODILE_HEIGHT;
    crocodile->y = lane->y;
    crocodile->dx = lane->direction;
    crocodile->speed = lane->speed;
    
    // Initial x position based on direction
    if (lane->direction > 0) {
        crocodile->x = 1;  // Start from left
    } else {
        crocodile->x = MAP_WIDTH - CROCODILE_WIDTH - 1;  // Start from right
    }

    // Crocodile sprite
    char sprite[CROCODILE_HEIGHT][CROCODILE_WIDTH] = {
        {'<', 'C', 'R', 'O', 'C', '>'},
        {'<', 'O', 'D', 'Y', 'L', '>'}
    };

    for (int i = 0; i < CROCODILE_HEIGHT; i++) {
        for (int j = 0; j < CROCODILE_WIDTH; j++) {
            crocodile->sprite[i][j] = sprite[i][j];
        }
    }
}

void *crocodile_thread(void *arg) {
    CrocodileArgs *args = (CrocodileArgs*) arg;
    CircularBuffer *buffer = args->buffer;

    RiverLane *lane = args->lane;

    Message msg;
    Entity crocodile;
    
    // Initialize crocodile
    crocodile_init(&crocodile,lane);
    
    // Prepare message
    msg.type = MSG_CROC_UPDATE;
    
    while (true){
        // Sleep based on speed
        usleep(crocodile.speed);
        
        // Update x position based on lane direction
        crocodile.x += crocodile.dx;
        // Prepare and send message
        msg.entity = crocodile;
        buffer_push(buffer, msg);
    }
    
    free(args);
    return NULL;
}

void *crocodile_projectile_thread(void *arg) {
    // Implementation for projectile thread
    // Similar to grenade threads, but with specific river lane characteristics
    return NULL;
}

void draw_crocodile(WINDOW *win, Entity *crocodile) {
    for (int i = 0; i < CROCODILE_HEIGHT; i++) {
        for (int j = 0; j < CROCODILE_WIDTH; j++) {
            if(crocodile->y + i < MAP_HEIGHT && crocodile->y + i > 0 &&
                crocodile->x + j < MAP_WIDTH && crocodile->x + j > 0){
                    wattron(win, COLOR_PAIR(map[crocodile->y + i][crocodile->x + j]));
                    mvwaddch(win, crocodile->y + i, crocodile->x + j, crocodile->sprite[i][j]);
                    wattroff(win, COLOR_PAIR(map[crocodile->y + i][crocodile->x + j]));
                }
        }
    }
    
    pthread_mutex_lock(&render_mutex);
    wrefresh(win);
    pthread_mutex_unlock(&render_mutex);
}

void clear_crocodile(WINDOW *win, Entity *crocodile) {
    for (int i = 0; i < CROCODILE_HEIGHT; i++) {
        for (int j = 0; j < CROCODILE_WIDTH; j++) {
            if(crocodile->y + i < MAP_HEIGHT && crocodile->y + i > 0 &&
                crocodile->x + j < MAP_WIDTH && crocodile->x + j > 0){
                    wattron(win, COLOR_PAIR(map[crocodile->y + i][crocodile->x + j]));
                    mvwaddch(win, crocodile->y + i, crocodile->x + j, ' ');
                    wattroff(win, COLOR_PAIR(map[crocodile->y + i][crocodile->x + j]));
                }
        }
    }
    pthread_mutex_lock(&render_mutex);
    wrefresh(win);
    pthread_mutex_unlock(&render_mutex);
}
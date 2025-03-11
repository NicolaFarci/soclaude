// crocodile.h
#ifndef CROCODILE_H
#define CROCODILE_H

#include "game.h"
#include "buffer.h"

#define CROC_HEIGHT 2
#define CROC_WIDTH 6
#define NUM_STREAMS 8
#define STREAM_HEIGHT 2
#define STREAM_START_Y 9 // Prima riga del fiume dopo il prato
#define STREAM_END_Y (STREAM_START_Y + (NUM_STREAMS * STREAM_HEIGHT)) // Ultima riga del fiume

// Struttura per rappresentare un coccodrillo
typedef struct {
    Entity entity;
    int stream_id;
    bool has_frog;
} Crocodile;

// Struttura per rappresentare un flusso
typedef struct {
    int direction; // 1 = destra, -1 = sinistra
    int speed;     // microseconds di attesa tra movimenti
    pthread_t generator_tid;
    bool active;
} Stream;

// Parametri per il thread del coccodrillo
typedef struct {
    CircularBuffer *buffer;
    int stream_id;
    int direction;
    int speed;
    int start_x;
} CrocodileArgs;

// Parametri per il thread generatore di coccodrilli
typedef struct {
    CircularBuffer *buffer;
    int stream_id;
} StreamGeneratorArgs;

// Prototipi delle funzioni
void init_streams();
void *crocodile_thread(void *arg);
void *stream_generator_thread(void *arg);
void draw_crocodile(WINDOW *win, Entity *croc);
void clear_crocodile(WINDOW *win, Entity *croc);
int get_stream_from_y(int y);
bool is_on_crocodile(Entity *frog, Crocodile *crocs, int num_crocs);
void kill_stream_threads();

extern Stream streams[NUM_STREAMS];
extern pthread_mutex_t crocs_mutex;
extern Crocodile active_crocs[BUFFER_SIZE]; // Array di coccodrilli attivi
extern int num_active_crocs;

#endif
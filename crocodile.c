// crocodile.c
#include "crocodile.h"
#include "map.h"
#include "buffer.h"
#include <stdlib.h>
#include <time.h>

Stream streams[NUM_STREAMS];
pthread_mutex_t crocs_mutex = PTHREAD_MUTEX_INITIALIZER;
Crocodile active_crocs[BUFFER_SIZE];
int num_active_crocs = 0;

// Inizializza i flussi con direzioni alternate e velocità casuali
void init_streams() {
    srand(time(NULL));
    
    // Decide casualmente la direzione del primo flusso
    int first_dir = (rand() % 2 == 0) ? 1 : -1;
    
    for (int i = 0; i < NUM_STREAMS; i++) {
        streams[i].direction = (i % 2 == 0) ? first_dir : -first_dir;
        // Velocità tra 100000 e 300000 microseconds (da più veloce a più lento)
        streams[i].speed = 100000 + (rand() % 200000);
        streams[i].active = true;
    }
}

// Thread per generare coccodrilli su un flusso specifico
void *stream_generator_thread(void *arg) {
    StreamGeneratorArgs *sargs = (StreamGeneratorArgs *)arg;
    CircularBuffer *buffer = sargs->buffer;
    int stream_id = sargs->stream_id;
    int dir = streams[stream_id].direction;
    int speed = streams[stream_id].speed;
    
    // Libera la memoria dell'argomento
    free(sargs);
    
    while (1) {
        // Esce se il gioco è terminato
        pthread_mutex_lock(&game_state_mutex);
        if (game_state == GAME_QUITTING || game_state == GAME_WIN) {
            pthread_mutex_unlock(&game_state_mutex);
            pthread_exit(NULL);
        }
        pthread_mutex_unlock(&game_state_mutex);

        // Attende durante la pausa
        pthread_mutex_lock(&game_state_mutex);
        if (game_state == GAME_PAUSED) {
            pthread_mutex_unlock(&game_state_mutex);
            usleep(100000);
            continue;
        }
        pthread_mutex_unlock(&game_state_mutex);

        // Determina la posizione di partenza in base alla direzione
        int start_x = (dir == 1) ? 0 : MAP_WIDTH - CROC_WIDTH;
        
        // Crea i parametri per il thread del coccodrillo
        CrocodileArgs *cargs = malloc(sizeof(CrocodileArgs));
        cargs->buffer = buffer;
        cargs->stream_id = stream_id;
        cargs->direction = dir;
        cargs->speed = speed;
        cargs->start_x = start_x;
        
        // Crea il thread del coccodrillo
        pthread_t croc_tid;
        pthread_create(&croc_tid, NULL, crocodile_thread, cargs);
        pthread_detach(croc_tid);
        
        // Attende un intervallo casuale prima di generare il prossimo coccodrillo
        // Tra 2 e 4 secondi
        int wait_time = 2000000 + (rand() % 2000000);
        usleep(wait_time);
    }
    
    return NULL;
}

// Thread per gestire un singolo coccodrillo
void *crocodile_thread(void *arg) {
    CrocodileArgs *args = (CrocodileArgs *)arg;
    CircularBuffer *buffer = args->buffer;
    int stream_id = args->stream_id;
    int direction = args->direction;
    int speed = args->speed;
    int start_x = args->start_x;
    
    // Determina la posizione Y in base al flusso
    int y = STREAM_START_Y + (stream_id * STREAM_HEIGHT);
    
    // Crea il messaggio per il coccodrillo
    Message msg;
    msg.type = MSG_CROCODILE;
    msg.entity.type = ENTITY_CROCODILE;
    msg.entity.x = start_x;
    msg.entity.y = y;
    msg.entity.width = CROC_WIDTH;
    msg.entity.height = CROC_HEIGHT;
    msg.entity.dx = direction;
    msg.entity.speed = speed;
    
    // Forma del coccodrillo
    char sprite[CROC_HEIGHT][CROC_WIDTH] = {
        {'~', '~', '(', ')', '~', '~'},
        {'=', '=', '=', '=', '=', '='}
    };
    
    for (int i = 0; i < CROC_HEIGHT; i++) {
        for (int j = 0; j < CROC_WIDTH; j++) {
            msg.entity.sprite[i][j] = sprite[i][j];
        }
    }
    
    // Aggiunge il coccodrillo all'array dei coccodrilli attivi
    pthread_mutex_lock(&crocs_mutex);
    if (num_active_crocs < BUFFER_SIZE) {
        active_crocs[num_active_crocs].entity = msg.entity;
        active_crocs[num_active_crocs].stream_id = stream_id;
        active_crocs[num_active_crocs].has_frog = false;
        num_active_crocs++;
    }
    pthread_mutex_unlock(&crocs_mutex);
    
    // Invia il coccodrillo al buffer
    buffer_push(buffer, msg);
    
    // Libera la memoria degli argomenti
    free(args);
    
    // Condizione di uscita dal ciclo in base alla direzione
    bool exit_condition;
    if (direction > 0) {
        exit_condition = msg.entity.x >= MAP_WIDTH;
    } else {
        exit_condition = msg.entity.x + CROC_WIDTH <= 0;
    }
    
    // Muove il coccodrillo finché non esce dallo schermo
    while (!exit_condition) {
        // Controllo per la pausa
        pthread_mutex_lock(&game_state_mutex);
        if (game_state == GAME_PAUSED) {
            pthread_mutex_unlock(&game_state_mutex);
            usleep(100000);
            continue;
        }
        pthread_mutex_unlock(&game_state_mutex);
        
        // Controllo per uscita o vittoria
        pthread_mutex_lock(&game_state_mutex);
        if (game_state == GAME_QUITTING || game_state == GAME_WIN) {
            pthread_mutex_unlock(&game_state_mutex);
            
            // Rimuove il coccodrillo dall'array
            pthread_mutex_lock(&crocs_mutex);
            for (int i = 0; i < num_active_crocs; i++) {
                if (active_crocs[i].entity.x == msg.entity.x && 
                    active_crocs[i].entity.y == msg.entity.y) {
                    // Sposta l'ultimo elemento nella posizione corrente
                    if (i < num_active_crocs - 1) {
                        active_crocs[i] = active_crocs[num_active_crocs - 1];
                    }
                    num_active_crocs--;
                    break;
                }
            }
            pthread_mutex_unlock(&crocs_mutex);
            
            pthread_exit(NULL);
        }
        pthread_mutex_unlock(&game_state_mutex);
        
        // Attesa basata sulla velocità
        usleep(speed);
        
        // Aggiorna la posizione
        msg.entity.x += direction;
        
        // Aggiorna il coccodrillo nel buffer
        buffer_push(buffer, msg);
        
        // Aggiorna il coccodrillo nell'array
        pthread_mutex_lock(&crocs_mutex);
        for (int i = 0; i < num_active_crocs; i++) {
            if (active_crocs[i].entity.x == msg.entity.x - direction && 
                active_crocs[i].entity.y == msg.entity.y) {
                active_crocs[i].entity.x = msg.entity.x;
                break;
            }
        }
        pthread_mutex_unlock(&crocs_mutex);
        
        // Aggiorna la condizione di uscita
        if (direction > 0) {
            exit_condition = msg.entity.x >= MAP_WIDTH;
        } else {
            exit_condition = msg.entity.x + CROC_WIDTH <= 0;
        }
    }
    
    // Rimuove il coccodrillo dall'array
    pthread_mutex_lock(&crocs_mutex);
    for (int i = 0; i < num_active_crocs; i++) {
        if (active_crocs[i].entity.y == msg.entity.y && 
            ((direction > 0 && active_crocs[i].entity.x >= MAP_WIDTH - CROC_WIDTH) ||
             (direction < 0 && active_crocs[i].entity.x <= 0))) {
            // Sposta l'ultimo elemento nella posizione corrente
            if (i < num_active_crocs - 1) {
                active_crocs[i] = active_crocs[num_active_crocs - 1];
            }
            num_active_crocs--;
            break;
        }
    }
    pthread_mutex_unlock(&crocs_mutex);
    
    return NULL;
}

// Disegna un coccodrillo sullo schermo
void draw_crocodile(WINDOW *win, Entity *croc) {
    // Disegna solo le parti visibili del coccodrillo
    int start_x = (croc->x < 0) ? 0 : croc->x;
    int end_x = (croc->x + croc->width > MAP_WIDTH) ? MAP_WIDTH : croc->x + croc->width;
    
    for (int i = 0; i < croc->height; i++) {
        for (int j = start_x - croc->x; j < end_x - croc->x; j++) {
            int x_pos = croc->x + j;
            int y_pos = croc->y + i;
            
            if (x_pos >= 0 && x_pos < MAP_WIDTH && y_pos >= 0 && y_pos < MAP_HEIGHT) {
                wattron(win, COLOR_PAIR(6)); // Colore del coccodrillo
                mvwaddch(win, y_pos, x_pos, croc->sprite[i][j]);
                wattroff(win, COLOR_PAIR(6));
            }
        }
    }
    
    pthread_mutex_lock(&render_mutex);
    wrefresh(win);
    pthread_mutex_unlock(&render_mutex);
}

// Cancella un coccodrillo dallo schermo
void clear_crocodile(WINDOW *win, Entity *croc) {
    // Pulisce solo le parti visibili del coccodrillo
    int start_x = (croc->x < 0) ? 0 : croc->x;
    int end_x = (croc->x + croc->width > MAP_WIDTH) ? MAP_WIDTH : croc->x + croc->width;
    
    for (int i = 0; i < croc->height; i++) {
        for (int j = start_x - croc->x; j < end_x - croc->x; j++) {
            int x_pos = croc->x + j;
            int y_pos = croc->y + i;
            
            if (x_pos >= 0 && x_pos < MAP_WIDTH && y_pos >= 0 && y_pos < MAP_HEIGHT) {
                wattron(win, COLOR_PAIR(map[y_pos][x_pos]));
                mvwaddch(win, y_pos, x_pos, ' ');
                wattroff(win, COLOR_PAIR(map[y_pos][x_pos]));
            }
        }
    }
    
    pthread_mutex_lock(&render_mutex);
    wrefresh(win);
    pthread_mutex_unlock(&render_mutex);
}

// Determina su quale flusso si trova una determinata coordinata Y
int get_stream_from_y(int y) {
    if (y < STREAM_START_Y || y >= STREAM_END_Y) {
        return -1; // Non in nessun flusso
    }
    
    return (y - STREAM_START_Y) / STREAM_HEIGHT;
}

// Controlla se la rana è su un coccodrillo
bool is_on_crocodile(Entity *frog, Crocodile *crocs, int num_crocs) {
    int stream_id = get_stream_from_y(frog->y);
    
    // Se la rana non è in un flusso, non è su un coccodrillo
    if (stream_id == -1) {
        return false;
    }
    
    // Cerca i coccodrilli nel flusso corrente
    for (int i = 0; i < num_crocs; i++) {
        if (crocs[i].stream_id == stream_id) {
            // Controlla se la rana è sul coccodrillo
            if (frog->x >= crocs[i].entity.x && 
                frog->x + frog->width <= crocs[i].entity.x + crocs[i].entity.width) {
                return true;
            }
        }
    }
    
    return false;
}

// Termina tutti i thread dei flussi
void kill_stream_threads() {
    for (int i = 0; i < NUM_STREAMS; i++) {
        if (streams[i].active) {
            pthread_cancel(streams[i].generator_tid);
            streams[i].active = false;
        }
    }
}
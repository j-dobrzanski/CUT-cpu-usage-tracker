#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "../include/ready_data.h"
#include "../include/data_manager.h"

#define MAX_BUFFER_SIZE 7 /* max number of print-ready data in buffer */

/* Keeps single list of percentages in order cpu, cpu1, cpu2, ..., up to payload size */
typedef struct Ready_data_element{
    size_t payload_size;
    double* payload[]; /* We can use FAM, because we don't know the value of payload_size */
} Ready_data_element;

/* Keeps basic actual info of buffer and manages it and manages thread-needed variables */
struct Ready_data_manager{
    size_t max_size;
    size_t current_size;
    pthread_mutex_t mutex;
    pthread_cond_t can_produce;
    pthread_cond_t can_consume;
    Ready_data_element* table[MAX_BUFFER_SIZE];
};

Ready_data* ready_data_create(void);
void ready_data_destroy(Ready_data* read_data);
bool ready_data_full(const Ready_data* ready_data);
bool ready_data_empty(const Ready_data* ready_data);
void ready_data_add(Ready_data*, double data[], size_t elem_number);
double* ready_data_get(Ready_data* ready_data);
void ready_data_lock(Ready_data* ready_data);
void ready_data_unlock(Ready_data* ready_data);
void ready_data_call_producer(Ready_data* ready_data);
void ready_data_call_consumer(Ready_data* ready_data);
void ready_data_wait_for_producer(Ready_data* ready_data);
void ready_data_wait_for_consumer(Ready_data* ready_data);


/* Constructor of data manager - allocates sapce in memory and initializes fields */
Ready_data* ready_data_create(void){
    Ready_data* ready_data = malloc(sizeof(*ready_data));
    if(ready_data == NULL){
        return NULL;
    }

    *ready_data = (Ready_data){.max_size = MAX_BUFFER_SIZE,
                                .current_size = 0,
                                .mutex = PTHREAD_MUTEX_INITIALIZER,
                                .can_produce = PTHREAD_COND_INITIALIZER,
                                .can_consume = PTHREAD_COND_INITIALIZER,
                                .table = {0}
                                };

    return ready_data;
}

/* Destructor of data manager- frees all elements in buffer and destroys thread-connected variables before freeing data manager */
void ready_data_destroy(Ready_data* ready_data){
    for(size_t i = 0; i < ready_data->current_size; i++){
        free(ready_data->table[i]);
    }
    free(ready_data);
}

/* Checks if data buffer is full by comparing current and max size */
bool ready_data_full(const Ready_data* ready_data){
    if(ready_data == NULL){
        return false;
    }
    return ready_data->current_size == ready_data->max_size;
}

/* Checks if data buffer is empty by comparing current size to 0 */
bool ready_data_empty(const Ready_data* ready_data){
    if(ready_data == NULL){
        return false;
    }
    return ready_data->current_size == 0;
}

/* Adds new element to data buffer - in some way Ready_data_element constructor,
    and updates info in manager.
 */
void ready_data_add(Ready_data* ready_data, double data[], size_t elem_number){
    if(ready_data == NULL || data == NULL || elem_number <= 0){
        return;
    }

    if(ready_data_full(ready_data)){
        return;
    }

    Ready_data_element* new_element = malloc(sizeof(*new_element) + sizeof(*data) * elem_number);
    if(new_element == NULL){
        return;
    }
    memcpy(new_element->payload, data, sizeof(*data)*elem_number);
    new_element->payload_size = elem_number;

    ready_data->table[ready_data->current_size] = new_element;

    ready_data->current_size++;
}

double* ready_data_get(Ready_data* ready_data){
    if(ready_data_empty(ready_data)){
        return NULL;
    }
    Ready_data_element* element = ready_data->table[0];
    double* data = malloc(sizeof(*data) * element->payload_size);
    if(data == NULL){
        return NULL;
    }
    memcpy(data, element->payload, sizeof(**element->payload)*element->payload_size);
    for(size_t i = 1; i < ready_data->current_size; i++){
        ready_data->table[i-1] = ready_data->table[i];
    }
    ready_data->table[ready_data->current_size-1] = NULL;
    ready_data->current_size--;
    free(element);
    return data;
}

/* API functions to avoid giving the exact structure outside this filescope. */
void ready_data_lock(Ready_data* ready_data){
    pthread_mutex_lock(&ready_data->mutex);
}

void ready_data_unlock(Ready_data* ready_data){
    pthread_mutex_unlock(&ready_data->mutex);
}

void ready_data_call_producer(Ready_data* ready_data){
    pthread_cond_signal(&ready_data->can_produce);
}

void ready_data_call_consumer(Ready_data* ready_data){
    pthread_cond_signal(&ready_data->can_consume);
}

void ready_data_wait_for_producer(Ready_data* ready_data){
    pthread_cond_wait(&ready_data->can_consume, &ready_data->mutex);
}

void ready_data_wait_for_consumer(Ready_data* ready_data){
    pthread_cond_wait(&ready_data->can_produce, &ready_data->mutex);
}



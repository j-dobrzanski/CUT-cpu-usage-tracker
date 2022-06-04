#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "../include/raw_data.h"
#include "../include/data_manager.h"

#define MAX_BUFFER_SIZE 7 /* max number of cpu usage snapshots in buffer */

/* Structure to store and manage raw data that waits to be analyzed.
    We don't know how much space we need for every single 'data frame' (cpu usage snapshot)
    and the exact amount can be different for different frames.
    And we don't want to overestimate nedded space - that would litter memory.
    That's why we can take adventage of FAM for single frame.
    But we can have only one FAM field in struct, 
    so the idea is to have a structure to manage our 'FAM fields'.
    Important thing is keeping it like FIFO to provide data in correct order.
*/

/* After coding thought: maybe consdier changing linked-list later to Raw_data_element* list[MAX_BUFFER_SIZE] 
    that will store pointers to Raw_data_elements,
    problems: still need something like head/tail or many operations on this list to manage it
    adventages: easier destroying, *easier* data structure
*/


/* Keeps cpu usage snapshot - buffer element */
typedef struct Raw_data_element{
    struct Raw_data_element* next_element;
    size_t data_size;
    char* payload[];
} Raw_data_element;

/* Keeps basic actual info of buffer and manages it and manages thread-needed variables */
struct Raw_data_manager{
    size_t max_size;
    size_t current_size;
    Raw_data_element* head;
    Raw_data_element* tail;
    pthread_mutex_t mutex;
    pthread_cond_t can_produce;
    pthread_cond_t can_consume;
};

Raw_data* raw_data_create(void);
void raw_data_destroy(Raw_data* raw_data);
bool raw_data_full(const Raw_data* raw_data);
bool raw_data_empty(const Raw_data* raw_data);
void raw_data_add(Raw_data* raw_data, char data[]);
char* raw_data_get(Raw_data* raw_data);
void raw_data_lock(Raw_data* raw_data);
void raw_data_unlock(Raw_data* raw_data);
void raw_data_call_producer(Raw_data* raw_data);
void raw_data_call_consumer(Raw_data* raw_data);
void raw_data_wait_for_producer(Raw_data* raw_data);
void raw_data_wait_for_consumer(Raw_data* raw_data);


/* Constructor of data manager - allocates space in memory and initializes fields*/
Raw_data* raw_data_create(void){
    Raw_data* raw_data = malloc(sizeof(*raw_data));
    if(raw_data == NULL){
        return NULL;
    }

    *raw_data = (Raw_data){.max_size = MAX_BUFFER_SIZE,
                            .current_size = 0,
                            .head = NULL,
                            .tail = NULL,
                            .mutex = PTHREAD_MUTEX_INITIALIZER,
                            .can_produce = PTHREAD_COND_INITIALIZER,
                            .can_consume = PTHREAD_COND_INITIALIZER
                            };

    return raw_data;
}

/* Destructor of data manager - frees all frames in buffer and destroys thread-connected variables before freeing data manager */
void raw_data_destroy(Raw_data* raw_data){

    Raw_data_element* head = raw_data->head;
    Raw_data_element* current = raw_data->head;

    while(current != NULL){
        head = current->next_element;
        free(current);
        current = head;
    }

    pthread_cond_destroy(&raw_data->can_consume);
    pthread_cond_destroy(&raw_data->can_produce);
    pthread_mutex_destroy(&raw_data->mutex);

    free(raw_data);
}

/* Checks if data buffer is full by comparing current and max size */
bool raw_data_full(const Raw_data* raw_data){
    if(raw_data == NULL){
        return false;
    }
    return raw_data->current_size == raw_data->max_size;
}

/* Checks if data buffer is empty by cimparing current size to 0 */
bool raw_data_empty(const Raw_data* raw_data){
    if(raw_data == NULL){
        return false;
    }
    return raw_data->current_size == 0;
}

/* Adds new cpu usage snapshot to data buffer - in some way Raw_data_element constructor,
    and updates info in manager.
 */
void raw_data_add(Raw_data* raw_data, char data[]){
    if(data == NULL){
        return;
    }

    if(raw_data_full(raw_data)){
        return;
    }

    /* Raw_data_element constructing and initializing variables */
    size_t data_size = strlen(data) + 1; /* strlen() gives 'visual' number - we need one more fo '\0' */
    Raw_data_element* new_element = malloc(sizeof(*new_element) + data_size * sizeof(char));
    if(new_element == NULL){
        return;
    }

    new_element->next_element = NULL;
    new_element->data_size = data_size;
    memcpy(new_element->payload, data, data_size);

    /* If buffer is empty than afeter adding element head = tail, otherwise tail stays the same, and head moves to the new element*/
    if(raw_data->head == NULL){
        raw_data->head = new_element;
        raw_data->tail = new_element;
    }
    else{
        raw_data->head->next_element = new_element;
        raw_data->head = new_element;
    }

    raw_data->current_size++;
}

/* Gets cpu usage snapshot from data buffer - in some way Raw_data_element destructor,
    and updates info in manager.
 */
char* raw_data_get(Raw_data* raw_data){
    if(raw_data_empty(raw_data)){
        return NULL;
    }

    Raw_data_element* old_element = raw_data->tail;

    char* data = malloc(sizeof(char) * old_element->data_size);
    if(data == NULL){
        return NULL;
    }
    memcpy(data, old_element->payload, old_element->data_size);

    /* New tail is element after element taken, but if it's null, then the buffer is empty and head is null too */
    raw_data->tail = old_element->next_element;
    if(raw_data->tail == NULL){
        raw_data->head = NULL;
    }
    free(old_element);
    raw_data->current_size--;

    return data;
}


/* API functions to avoid giving the exact structure outside this filescope. */
void raw_data_lock(Raw_data* raw_data){
    pthread_mutex_lock(&raw_data->mutex);
}

void raw_data_unlock(Raw_data* raw_data){
    pthread_mutex_unlock(&raw_data->mutex);
}

void raw_data_call_producer(Raw_data* raw_data){
    pthread_cond_signal(&raw_data->can_produce);
}

void raw_data_call_consumer(Raw_data* raw_data){
    pthread_cond_signal(&raw_data->can_consume);
}

void raw_data_wait_for_producer(Raw_data* raw_data){
    pthread_cond_wait(&raw_data->can_consume, &raw_data->mutex);
}

void raw_data_wait_for_consumer(Raw_data* raw_data){
    pthread_cond_wait(&raw_data->can_produce, &raw_data->mutex);
}

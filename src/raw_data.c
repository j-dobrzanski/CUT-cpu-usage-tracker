#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "raw_data.h"

#define MAX_BUFFER_SIZE 7

typedef struct Raw_data_element{
    struct Raw_data_element* next_element;
    size_t data_size;
    char* payload[];
} Raw_data_element;

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
void raw_data_add(Raw_data* raw_data, char data[], size_t data_size);
char* raw_data_get(Raw_data* raw_data);
void raw_data_lock(Raw_data* raw_data);
void raw_data_unlock(Raw_data* raw_data);
void raw_data_call_producer(Raw_data* raw_data);
void raw_data_call_consumer(Raw_data* raw_data);
void raw_data_wait_for_producer(Raw_data* raw_data);
void raw_data_wait_for_consumer(Raw_data* raw_data);


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

bool raw_data_full(const Raw_data* raw_data){
    if(raw_data == NULL){
        return false;
    }
    return raw_data->current_size == raw_data->max_size;
}

bool raw_data_empty(const Raw_data* raw_data){
    if(raw_data == NULL){
        return false;
    }
    return raw_data->current_size == 0;
}

void raw_data_add(Raw_data* raw_data, char data[], size_t data_size){
    if(raw_data_full(raw_data)){
        return;
    }
    
    Raw_data_element* new_element = malloc(sizeof(*new_element) + data_size * sizeof(char));
    if(new_element == NULL){
        return;
    }
    
    new_element->next_element = NULL;
    new_element->data_size = data_size;
    memcpy(new_element->payload, data, data_size);

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

    raw_data->tail = old_element->next_element;
    if(raw_data->tail == NULL){
        raw_data->head = NULL;
    }
    free(old_element);

    return data;
}

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

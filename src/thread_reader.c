#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sysinfo.h>
#include "../include/raw_data.h"
#include "../include/threads.h"


#define SINLGE_BUFFER_SIZE 16

void* reader(void* arg);

/* From documentation of kernel we know that in /proc/stat after info about cpus we get line with info about interrupts
    starting with intr, so we can just read this file until we get 'i', and that's what read_data(FILE* ptr) does.
    Problem with unknown data length is dealt with by making new bigger table of chars containing additional SINGLE_BUFFER_SIZE memory.
    Old list is copied indirectly to the new, bigger list of the same name, and reading the file is continued.     
*/
static char* read_data(FILE* ptr){
    char* table = malloc(sizeof(char) * SINLGE_BUFFER_SIZE);
    if(table == NULL){
        return NULL;
    }
    char c = 0;
    c = (char)fgetc(ptr);
    if(c == EOF){
        return NULL;
    }
    size_t buffer_number = 1;
    size_t inside_buffer = 0;
    while(c != 'i'){
        if(inside_buffer < buffer_number * SINLGE_BUFFER_SIZE){
            table[inside_buffer] = c;
        }
        else{
            char* temp_table = malloc(sizeof(char) * SINLGE_BUFFER_SIZE * buffer_number);
            temp_table = memcpy(temp_table, table, buffer_number*SINLGE_BUFFER_SIZE);
            free(table);
            buffer_number += 1;
            table = malloc(sizeof(char) * SINLGE_BUFFER_SIZE * buffer_number);
            table = memcpy(table, temp_table, SINLGE_BUFFER_SIZE * (buffer_number - 1));
            table[inside_buffer] = c;
        }
        c = (char)fgetc(ptr);
        inside_buffer += 1;
        if(c == EOF){
            return NULL;
        }
    }
    if(inside_buffer < buffer_number * SINLGE_BUFFER_SIZE){
        table[inside_buffer] = '\0';
    }
    else{
        char* temp_table = malloc(sizeof(char) * SINLGE_BUFFER_SIZE * buffer_number);
        if(temp_table == NULL){
            return NULL;
        }
        temp_table = memcpy(temp_table, table, buffer_number*SINLGE_BUFFER_SIZE);
        if(temp_table == NULL){
            return NULL;
        }
        free(table);
        buffer_number += 1;
        table = malloc(sizeof(char) * SINLGE_BUFFER_SIZE * buffer_number);
        if(table == NULL){
            return NULL;
        }
        table = memcpy(table, temp_table, SINLGE_BUFFER_SIZE * (buffer_number - 1));
        if(table == NULL){
            return NULL;
        }
        table[inside_buffer] = '\0';
    }
    return table;
}

/* Reads data from /proc/stat and than adds that into the buffer */
void* reader(void* arg){
    Raw_data* const raw_data = *(Raw_data**)arg;
    while(true){
        FILE* const ptr = fopen("/proc/stat", "r");
        if(ptr == NULL){
            return NULL;
        }

        char* data = read_data(ptr);

        /* locking raw_data as we will manipulate now on that */
        raw_data_lock(raw_data);

        if(raw_data_full(raw_data)){
            raw_data_wait_for_consumer(raw_data);
        }

        raw_data_add(raw_data, data);

        raw_data_call_consumer(raw_data);

        raw_data_unlock(raw_data);

        fclose(ptr);
    }

    return NULL;
}

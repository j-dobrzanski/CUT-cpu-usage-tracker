#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sysinfo.h>
#include "../include/raw_data.h"
#include "../include/threads.h"

/* 
    Very theoretical value - every line we want to parse consists of (according to linux kernel documentation):
        - cpu or cpuN string
        - 10 numbers of unknown length
    so we don't really know how many chars we need to store those lines,
    however we know that biggest value we can store in C is 
    2^64-1 (for 64-bit system and 2^32-1 for 32-but system, but we take worse case)
    and 19 < log_10(2^64) < 20. Because of that we can assume that each of these numbers is at most 20 chars long
    (however it would be nice to check in some way every time that this assumtion was correct).
    that gives us at most 20*10=200 chars for numbers, 10 chars for spaces and some chars for cpuN.
    Because that last value is definitely under 46 chars long we can say safely say 
    that maximum length of the line is below 256 chars.
 */
#define MAX_LINE_LENGTH 256 

static char** read_data(FILE* ptr);
static char* transform_data(char* table[]);
void* analyzer(void* arg);

static size_t proc_number;

static char** read_data(FILE* const ptr){
    char** table = malloc(sizeof(char*) * proc_number);
    if(table == NULL){
        return NULL;
    }

    char* temporary_string = malloc(sizeof(char) * MAX_LINE_LENGTH);
    if(temporary_string == NULL){
        return NULL;
    }

    for(size_t i = 0; i < proc_number; i++){
        temporary_string = fgets(temporary_string, MAX_LINE_LENGTH, ptr);
        if(temporary_string == NULL){
            return NULL;
        }
        size_t line_length = strlen(temporary_string) + 1;
        table[i] = malloc(sizeof(char) * line_length);
        if(table[i] == NULL){
            return NULL;
        }
        table[i] = memcpy(table[i], temporary_string, line_length);
    }

    free(temporary_string);

    return table;
}

static char* transform_data(char* table[]){
    size_t total_length = 0;
    for(size_t i = 0; i < proc_number; i++){
        total_length += strlen(table[i]) + 1;
    }
    char* data = malloc(sizeof(char) * total_length);
    if(data == NULL){
        return NULL;
    }

    size_t partial_sum = 0;
    for(size_t i = 0; i < proc_number; i++){
        size_t length = strlen(table[i]) + 1;
        char* ret = memcpy(&(data[partial_sum]), table[i], length);
        if(ret == NULL){
            return NULL;
        }
        partial_sum += length;

    }
    return data;
}

void* analyzer(void* arg){
    Raw_data* const raw_data = *(Raw_data**)arg;
    while(true){
        FILE* const ptr = fopen("/proc/stat", "r");
        if(ptr == NULL){
            return NULL;
        }
        int temp_procs = get_nprocs_conf();
        assert(temp_procs >= 0);
        proc_number = (size_t)temp_procs + 1;

        char** table_data = read_data(ptr);
        char* data = transform_data(table_data);

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

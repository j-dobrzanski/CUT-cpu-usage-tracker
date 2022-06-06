#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sysinfo.h>
#include <threads.h>
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
    /* New table to keepp /proc/stat data of basic size */
    char* table = malloc(sizeof(char) * SINLGE_BUFFER_SIZE);
    if(table == NULL){
        return NULL;
    }

    /* Getting first char of data to heve it initialized for while loop */
    char c = 0;
    c = (char)fgetc(ptr);
    if(c == EOF){
        return NULL;
    }

    /* Variables to keep track of: */
    size_t buffer_number = 1; /* Current size of buffer expressed in SINGLE_BUFFER_SIZE */
    size_t inside_buffer = 0; /* Index of current char to write in buffer */

    while(c != 'i'){
        /* If index is inside buffer size, than just save it */
        if(inside_buffer < buffer_number * SINLGE_BUFFER_SIZE){
            table[inside_buffer] = c;
        }
        /* Otherwise we need to dynamically increase allocated buffer memory; 
            new temp_table of size table, copy table into temp_table, free table, 
            allocate more memory to table and increase buffer_number, copy temp_table into table, save current char, free temp_table.  
        */
        else{
            char* temp_table = malloc(sizeof(char) * SINLGE_BUFFER_SIZE * buffer_number);
            memcpy(temp_table, table, buffer_number*SINLGE_BUFFER_SIZE);
            free(table);
            buffer_number += 1;
            table = malloc(sizeof(char) * SINLGE_BUFFER_SIZE * buffer_number);
            memcpy(table, temp_table, SINLGE_BUFFER_SIZE * (buffer_number - 1));
            table[inside_buffer] = c;
            free(temp_table);
        }
        /* Getting next char */
        c = (char)fgetc(ptr);
        inside_buffer += 1;
        if(c == EOF){
            return NULL;
        }
    }
    /* We want to indicate end of data, so let's make it string and terminate it with '\0'; same procedure as a few lines higher */
    if(inside_buffer < buffer_number * SINLGE_BUFFER_SIZE){
        table[inside_buffer] = '\0';
    }
    else{
        char* temp_table = malloc(sizeof(char) * SINLGE_BUFFER_SIZE * buffer_number);
        if(temp_table == NULL){
            return NULL;
        }
        memcpy(temp_table, table, buffer_number*SINLGE_BUFFER_SIZE);
        free(table);
        buffer_number += 1;
        table = malloc(sizeof(char) * SINLGE_BUFFER_SIZE * buffer_number);
        if(table == NULL){
            return NULL;
        }
        memcpy(table, temp_table, SINLGE_BUFFER_SIZE * (buffer_number - 1));
        table[inside_buffer] = '\0';
        free(temp_table);
    }
    return table;
}

/* Reads data from /proc/stat and than adds that into the buffer */
void* reader(void* arg){
    Raw_data* const raw_data = *(Raw_data**)arg; /* Casting arg onto our data struct pointer */
    int reader_handler = 0; /* Variable to keep track of signal handler */
    while(reader_handler == 0){
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
        free(data);
        fclose(ptr);

        /* No point in reading the file too often - it will always give 0 
            Aonther option is sleep() from unistd.h, but it isn't C standard nor included in pthread.h
        */
        thrd_sleep(&(struct timespec){.tv_sec=1}, NULL); 

        /* Checking if signal was caught */
        sig_lock();
        reader_handler = signal_handler;
        sig_unlock();
    }
    /* Without calling consumer after it may freeze while catching SIGTERM. Reason:
        if cpu isn't used much program runs fast and most of it's time running is waiting because of thrd_sleep( )
        (however there is no point in running it then without sleep, as differences are too small to give reasonable data),
        then the highest probability of catching SIGTERM is while thrd_sleep() and while analyzer() waits for new element in raw_data,
        it means that analyzer() is left hanging in that state.
     */
    raw_data_lock(raw_data);
    raw_data_call_consumer(raw_data);
    raw_data_unlock(raw_data);
    return NULL;
}

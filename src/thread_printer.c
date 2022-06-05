#include <stdio.h>
#include <stdlib.h>
#include "../include/ready_data.h"
#include "../include/threads.h"

void* printer(void* arg);

static void print_data(double data[]){
    size_t counter = 0;
    // printf("\e[1;1H\e[2J");
    printf("\033[H\033[J");
    while(data[counter] >= 0){
        if((ssize_t)data[counter] == 0){
            printf("Cpu: %.2f%%\n", data[counter + 1]*100);
        }
        else{
            printf("Cpu%.0f: %.2f%%\n", data[counter]-1, data[counter + 1]*100);
        }
        counter += 2;
    }
    return;
}

void* printer(void* arg){
    Ready_data* ready_data = *(Ready_data**)arg;

    while(true){
        /* Getting data to print */
        ready_data_lock(ready_data);

        if(ready_data_empty(ready_data)){
            ready_data_wait_for_producer(ready_data);
        }

        double* printable_data = ready_data_get(ready_data);

        ready_data_call_producer(ready_data);

        ready_data_unlock(ready_data);

        /* Printing data */
        print_data(printable_data);
        free(printable_data);
    }

    return NULL;
}




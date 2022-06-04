#include <stdio.h>
#include <stdlib.h>
#include "../include/ready_data.h"
#include "../include/threads.h"

void* printer(void* arg);

static void print_data(double* data){
    size_t counter = 0;
    printf("\n");
    while(data[counter] != -1){
        printf("Cpu%.0f: %.2f\n", data[counter], data[counter + 1]);
        counter += 2;
    }
    return;
}

void* printer(void* arg){
    Ready_data* ready_data = (Ready_data*)arg;

    while(true){
        fflush(stdout);
        /* Getting data to print */
        ready_data_lock(ready_data);

        if(ready_data_empty(ready_data)){
            ready_data_wait_for_proucer(ready_data);
        }

        double* printable_data = ready_data_get(ready_data);

        ready_data_call_producer(ready_data);

        ready_data_unlock(ready_data);

        /* Printing data */
        print_data(printable_data);
        free(printable_data);
    }
}




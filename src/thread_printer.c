#include <stdio.h>
#include <stdlib.h>
#include "../include/ready_data.h"
#include "../include/threads.h"

void* printer(void* arg);

static void print_data(double data[]){
    size_t counter = 0;
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
    int printer_handler = 0;
    while(printer_handler == 0){
        /* Getting data to print */
        ready_data_lock(ready_data);

        if(ready_data_empty(ready_data)){
            ready_data_wait_for_producer(ready_data);
        }

        double* printable_data = ready_data_get(ready_data);
        if(printable_data == NULL){
            break;
        }

        ready_data_call_producer(ready_data);

        ready_data_unlock(ready_data);

        /* Printing data */
        print_data(printable_data);
        free(printable_data);

        sig_lock();
        printer_handler = signal_handler;
        sig_unlock();
    }

    return NULL;
}




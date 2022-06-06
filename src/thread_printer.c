#include <stdio.h>
#include <stdlib.h>
#include "../include/ready_data.h"
#include "../include/threads.h"

void* printer(void* arg);

static void print_data(const double data[const]){
    size_t counter = 0;

    printf("\033[H\033[J"); /* Some *magic* code to get to highest line and clear all lines below - it clears terminal - I assume all linux terminals use ANSI esacpe characters*/
    
    while(data[counter] >= 0){ /* Checks for terminating -1 */
        
        if((ssize_t)data[counter] == 0){ /* in this line we get a small, positive, integer number, as it is N from cpuN */
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
    Ready_data*const ready_data = *(Ready_data**)arg;
    int printer_handler = 0; /* Variable to keep track of signal handler */
    while(printer_handler == 0){
        /* Getting data to print */
        ready_data_lock(ready_data);

        if(ready_data_empty(ready_data)){
            ready_data_wait_for_producer(ready_data);
        }

        double*const printable_data = ready_data_get(ready_data);
        if(printable_data == NULL){
            break;
        }

        ready_data_call_producer(ready_data);

        ready_data_unlock(ready_data);

        /* Printing data */
        print_data(printable_data);
        free(printable_data);

        /* Checking if signal was caught */
        sig_lock();
        printer_handler = signal_handler;
        sig_unlock();
    }

    return NULL;
}




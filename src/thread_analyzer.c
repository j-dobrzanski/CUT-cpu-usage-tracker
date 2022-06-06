#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>
#include "../include/raw_data.h"
#include "../include/ready_data.h"
#include "../include/threads.h"
#include "../include/data_manager.h"


#define FIELDS_NUMBER 11 /* Number of fields each line - specified by kernel documentation: cpuN + 10 after cpuN */
#define BUFFER_SIZE 3 /* we have to dynamically extend float* list to contain all of cpus */ 

void* analyzer(void* arg);

/* Enums to get rid of *magic numbers* while calculating needed values from cpu fields */
enum CPU_FIELDS_NAMES{
    CPU_N,
    USER,
    NICE,
    SYSTEM,
    IDLE,
    IOWAIT,
    IRQ,
    SOFTIRQ,
    STEAL
};

enum BASIC_VALUES_NAMES{
    VALUE_N,
    VALUE_IDLE,
    VALUE_TOTAL,
    SIZE
};

/* Function to calculate current Idle and Total that are needed for later calculations in differences */
static size_t* calculate_basic(const size_t list[const]){
    size_t Idle = list[IDLE] + list[IOWAIT]; /* idle + iowait */
    size_t NonIdle = list[USER] + list[NICE] + list[SYSTEM] + list[IRQ] + list[SOFTIRQ] + list[STEAL]; /* user + nice + system + irq + softirq + steal */
    size_t Total = Idle + NonIdle;
    size_t* ret = malloc(sizeof(*ret) * SIZE);
    ret[VALUE_N] = list[CPU_N];
    ret[VALUE_IDLE] = Idle;
    ret[VALUE_TOTAL] = Total;
    return ret;
}

/* Function to parse raw data into tokens and saving them into table of numbers - one row for each line, lines are counted into cpu_number.
    While parsing it we are taking adventage of each line structure defined in documentation.
*/
static size_t** data_parse_and_analyze(char* data, size_t* const cpu_number){
    char*const delimeter = " \n"; /* tokens in /proc/stat are separated by ' ' or '\n' */
    char* token = strtok(data, delimeter);
    *cpu_number = 0;

    /* Allocating buffer of initial size */
    size_t** parsed_data = malloc(sizeof(*parsed_data)*BUFFER_SIZE); 
    if(parsed_data == NULL){
        return NULL;
    }

    size_t buffer_number = 1; /* Initially we have parsed_data consisting of 1 buffers */
    
    while(token != NULL){

        /* variables in order as in line: (cpu)N, user, nice, system, idle, iowait, irq, softirq, steal, guest, guestnice */
        size_t fields[FIELDS_NUMBER] = {0};


        /* strtoumax() gives 0 when there is no conversion, so we want to differentiate cpu from cpu0 by adding 1 to every number from cpuN */
        if(strcmp(token, "cpu") == 0){
            fields[0] = 0;
        }
        else{
            fields[0] = strtoumax(&token[3], NULL, 10) + 1; /* &token[3] beacuse number starts only after 3 chars o 'cpu' */
        }
        if(errno != 0){
            return NULL;
        }
        
        for(size_t i = 1; i < FIELDS_NUMBER; i++){
            char* temp_tok = strtok(NULL, delimeter);

            if(temp_tok == NULL){
                return NULL;
            }
            
            fields[i] = strtoumax(temp_tok, NULL, 10);
            if(errno != 0){
                return NULL;
            }
        }

        size_t* data_calc = calculate_basic(fields); /* [(cpu)N, Idle, Total] */

        /* Dynamically increasing memory for our data*/
        if(*cpu_number < buffer_number * BUFFER_SIZE){
            parsed_data[*cpu_number] = data_calc;
        }
        else{
            size_t** temp_parsed_data = malloc(sizeof(*temp_parsed_data) * buffer_number * BUFFER_SIZE);
            memcpy(temp_parsed_data, parsed_data, sizeof(*parsed_data)*buffer_number * BUFFER_SIZE);
            free(parsed_data);
            buffer_number++;
            parsed_data = malloc(sizeof(*parsed_data) * buffer_number * BUFFER_SIZE);
            memcpy(parsed_data, temp_parsed_data, sizeof(*parsed_data) * (buffer_number - 1) * BUFFER_SIZE);
            free(temp_parsed_data);
            parsed_data[*cpu_number] = data_calc;
        }
        (*cpu_number)++;
        token = strtok(NULL, delimeter);
    }

    return parsed_data;
}

/* Function to calculate current cpu usage based on current and last /proc/stat snapshot */
static double* calculate_percentage(size_t*const*const old_data, size_t*const*const new_data, const size_t old_cpu_number, const size_t new_cpu_number, size_t*const max_cpu){
    size_t max_cpu_number = 0; /* Cpu number can change during program runtime, so we want to know max to stay in data bounds */
    if(old_cpu_number < new_cpu_number){
        max_cpu_number = old_cpu_number;
    }
    else{
        max_cpu_number = new_cpu_number;
    }
    double*const percentage_list = malloc(sizeof(*percentage_list)*(2*max_cpu_number+1)); /* for every line we keep cpuN and percentage and -1 terminating data */
    if(percentage_list == NULL){
        return NULL;
    }

    /* Variables to store current indexes to read data */
    size_t old_cpu_counter = 0;
    size_t new_cpu_counter = 0;
    *max_cpu = 0; /* We want to keep here number of all matched cpuN to know output data size */
    while(old_cpu_counter < old_cpu_number && new_cpu_counter < new_cpu_number && (*max_cpu) < max_cpu_number){
        if(old_data[old_cpu_counter][0] == new_data[new_cpu_counter][0]){
            const size_t* old_list = old_data[old_cpu_counter];
            const size_t* new_list = new_data[new_cpu_counter];
            percentage_list[2*(*max_cpu)] = (double)old_list[0];
            size_t totald = new_list[2] - old_list[2]; /* Total - PrevTotal */
            size_t idled = new_list[1] - old_list[1]; /* Ile - PrevIdle */
            double CPU_percentage = 0;
            if(totald <= idled || totald == 0){ /* we don't want to calculate some weird cases, especially when totald = 0 what forces us to /0 */
                CPU_percentage = 0;
            }
            else{
                CPU_percentage = (double)(totald - idled)/(double)totald;
            }
            percentage_list[2*(*max_cpu) + 1] = CPU_percentage;
            (*max_cpu)++;
            old_cpu_counter++;
            new_cpu_counter++;
        }
        else if(old_data[old_cpu_counter][0] < new_data[new_cpu_counter][0]){
            old_cpu_counter++;
        }
        else{
            new_cpu_counter++;
        }
    }
    /* Value to terminate data - we don't have to keep it's size */
    percentage_list[2*(*max_cpu)] = -1;
    return percentage_list;
}

/* After first data snapshot we don't have last data, so we want to simulate it to match new data */
static size_t** data_get_empty(const size_t cpu_number, size_t*const new_data[]){
    size_t** data = malloc(sizeof(*data) * cpu_number);
    if(data == NULL){
        return NULL;
    }
    for(size_t i = 0; i < cpu_number; i++){
        data[i] = malloc(sizeof(**data)*SIZE);
        if(data[i] == NULL){
            return NULL;
        }
        data[i][VALUE_N] = new_data[i][VALUE_N];
        data[i][VALUE_IDLE] = 0;
        data[i][VALUE_TOTAL] = 0;
    }
    return data;
}

/* Function to free data - easier to manage in code */
static void data_free(size_t** data, const size_t data_length){
    for(size_t i = 0; i < data_length; i++){
        free(data[i]);
    }
    free(data);
}

void* analyzer(void* arg){
    /* It will crash if someone would try to change it too much */
    if(FIELDS_NUMBER < 9){
        return NULL;
    }
    Data_manager* const manager = (Data_manager*)arg; /* We need to pass 2 pointers to this function, but pthread needs it to take only one parameter void*, so we pass pointer to structure keeping these two pointers */
    Raw_data* const raw_data = manager->raw_data;
    Ready_data* const ready_data = manager->ready_data;

    size_t new_cpu_number = 0;
    size_t old_cpu_number = 0;

    /* Getting initial data */

    raw_data_lock(raw_data);

    if(raw_data_empty(raw_data)){
        raw_data_wait_for_producer(raw_data);
    }

    char* data = raw_data_get(raw_data);

    raw_data_call_producer(raw_data);

    raw_data_unlock(raw_data);

    size_t** new_data = data_parse_and_analyze(data, &new_cpu_number);
    old_cpu_number = new_cpu_number;
    size_t** old_data = data_get_empty(new_cpu_number, new_data);
    
    free(data);
    int analyzer_handler = 0; /* Variable to keep track of signal handler */
    double* percentage_list = NULL;

    /* Main part of thread */
    while(analyzer_handler == 0){

        /* Calculating percentages */
        size_t number_of_cpus = 0;
        percentage_list = calculate_percentage(old_data, new_data, old_cpu_number, new_cpu_number, &number_of_cpus);
        
        /* Adding results to ready_data structure */
        ready_data_lock(ready_data);

        if(ready_data_full(ready_data)){
            ready_data_wait_for_consumer(ready_data);
        }

        ready_data_add(ready_data, percentage_list, 2*number_of_cpus+1);

        ready_data_call_consumer(ready_data);

        ready_data_unlock(ready_data);

        /* Getting new data */
        raw_data_lock(raw_data);

        if(raw_data_empty(raw_data)){
            raw_data_wait_for_producer(raw_data);
        }

        data = raw_data_get(raw_data);
        if(data == NULL){
            break;
        }

        raw_data_call_producer(raw_data);

        raw_data_unlock(raw_data);

        /* Analyzing new data */
        data_free(old_data, old_cpu_number);
        old_data = new_data;
        old_cpu_number = new_cpu_number;
        new_data = data_parse_and_analyze(data, &new_cpu_number);
        free(data);
        free(percentage_list);

        /* Checking if signal was caught */
        sig_lock();
        analyzer_handler = signal_handler;
        sig_unlock();
    }

    /* Some variables life-span in this thread is longer than one while loop, so after that we need to take care of freeing them,
        moreover loop may end before full one-time-execution, so we need to consider this too
     */
    free(percentage_list);
    data_free(old_data, old_cpu_number);
    data_free(new_data, new_cpu_number);

    /* Same problem as in thread_reader.c - it doesn't call consumer again before getting out of loop, and by that time printer() already waits for new data. */
    ready_data_lock(ready_data);
    ready_data_call_consumer(ready_data);
    ready_data_unlock(ready_data);
    return NULL;
}

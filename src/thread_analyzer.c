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

static size_t** parse_and_analyze_data(char* data, size_t* const cpu_number){
    char* delimeter = " \n";
    char* token = strtok(data, delimeter);
    *cpu_number = 0;

    size_t** parsed_data = malloc(sizeof(*parsed_data)*BUFFER_SIZE); 
    if(parsed_data == NULL){
        return NULL;
    }

    size_t buffer_number = 1; /* Initially we have parsed_data consisting of 1 buffers */
    
    while(token != NULL){

        /* variables in order as in line: (cpu)N, user, nice, system, idle, iowait, irq, softirq, steal, guest, guestnice */
        size_t fields[FIELDS_NUMBER] = {0};

        char* endptr;

        if(strcmp(token, "cpu") == 0){
            fields[0] = 0;
        }
        else{
            fields[0] = strtoumax(&token[3], &endptr, 10) + 1;
        }
        if(errno != 0){
            return NULL;
        }
        
        for(size_t i = 1; i < FIELDS_NUMBER; i++){
            char* temp_tok = strtok(NULL, delimeter);

            if(temp_tok == NULL){
                return NULL;
            }
            
            fields[i] = strtoumax(temp_tok, &endptr, 10);
            if(errno != 0){
                return NULL;
            }
        }

        size_t* data_calc = calculate_basic(fields); /* [(cpu)N, Idle, Total] */

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


static double* calculate_percentage(size_t*const*const old_data, size_t*const*const new_data, const size_t old_cpu_number, const size_t new_cpu_number, size_t* max_cpu){
    double* percentage_list = NULL;
    size_t max_cpu_number = 0;
    if(old_cpu_number < new_cpu_number){
        max_cpu_number = old_cpu_number;
    }
    else{
        max_cpu_number = new_cpu_number;
    }
    percentage_list = malloc(sizeof(*percentage_list)*(2*max_cpu_number+1));
    if(percentage_list == NULL){
        return NULL;
    }

    size_t old_cpu_counter = 0;
    size_t new_cpu_counter = 0;
    *max_cpu = 0;
    while(old_cpu_counter < old_cpu_number && new_cpu_counter < new_cpu_number && (*max_cpu) < max_cpu_number){
        if(old_data[old_cpu_counter][0] == new_data[new_cpu_counter][0]){
            const size_t* old_list = old_data[old_cpu_counter];
            const size_t* new_list = new_data[new_cpu_counter];
            percentage_list[2*(*max_cpu)] = (double)old_list[0];
            size_t totald = new_list[2] - old_list[2]; /* Total - PrevTotal */
            size_t idled = new_list[1] - old_list[1]; /* Ile - PrevIdle */
            double CPU_percentage = 0;
            if(totald <= idled || totald == 0){
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
    percentage_list[2*(*max_cpu)] = -1;
    return percentage_list;
}

static size_t** get_empty_data(size_t cpu_number, size_t* new_data[]){
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

static void free_data(size_t** data, size_t data_length){
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
    Data_manager* manager = (Data_manager*)arg;
    Raw_data* raw_data = manager->raw_data;
    Ready_data* ready_data = manager->ready_data;

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

    size_t** new_data = parse_and_analyze_data(data, &new_cpu_number);
    old_cpu_number = new_cpu_number;
    size_t** old_data = get_empty_data(new_cpu_number, new_data);
    
    free(data);
    int analyzer_handler = 0;
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
        free_data(old_data, old_cpu_number);
        old_data = new_data;
        old_cpu_number = new_cpu_number;
        new_data = parse_and_analyze_data(data, &new_cpu_number);
        free(data);
        free(percentage_list);

        sig_lock();
        analyzer_handler = signal_handler;
        sig_unlock();
    }

    free(percentage_list);
    free_data(old_data, old_cpu_number);
    free_data(new_data, new_cpu_number);

    ready_data_lock(ready_data);
    ready_data_call_consumer(ready_data);
    ready_data_unlock(ready_data);
    return NULL;
}

#include <pthread.h>
#include <string.h>
#include "../include/threads.h"
#include "../include/data_manager.h"
#include "../include/raw_data.h"
#include "../include/ready_data.h"


int main(void){

    /* Part resposible for signal handling */
    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = term; /* There is recursive macro disabled by clang */
    sigaction(SIGTERM, &action, NULL);

    /* Main part that creates data structures and manages threads */
    Raw_data* raw_data = raw_data_create();
    Ready_data* ready_data = ready_data_create();

    pthread_t thread_reader, thread_analyzer, thread_printer;
    pthread_create(&thread_reader, NULL, reader, (void*)&raw_data);
    pthread_create(&thread_analyzer, NULL, analyzer, (void*)&(Data_manager){.ready_data = ready_data, .raw_data = raw_data});
    pthread_create(&thread_printer, NULL, printer, (void*)&ready_data);

    pthread_join(thread_reader, NULL);
    pthread_join(thread_analyzer, NULL);
    pthread_join(thread_printer, NULL);

    raw_data_destroy(raw_data);
    ready_data_destroy(ready_data);

    return 0;
}

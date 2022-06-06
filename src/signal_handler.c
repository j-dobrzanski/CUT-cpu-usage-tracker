#include <signal.h>
#include <pthread.h>
#include "../include/threads.h"

volatile sig_atomic_t signal_handler;
pthread_mutex_t lock;

void term(int signum);
void sig_lock(void);
void sig_unlock(void);

/* We want all of the threads to have access to signal_handler, but we don't want them to interfere with it being assigned value by term function */
void term(int signum){
    pthread_mutex_lock(&lock);
    signal_handler = signum; /* As it is used to catch SIGTERM it will get that value */
    pthread_mutex_unlock(&lock);
}

void sig_lock(void){
    pthread_mutex_lock(&lock);
}

void sig_unlock(void){
    pthread_mutex_unlock(&lock);
}

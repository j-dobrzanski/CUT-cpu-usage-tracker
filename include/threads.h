#include <signal.h>

void* reader(void* arg);
void* analyzer(void* arg);
void* printer(void* arg);

extern volatile sig_atomic_t signal_handler;
extern pthread_mutex_t lock;

void term(int signum);
void sig_lock(void);
void sig_unlock(void);

#include <stddef.h>
#include <stdbool.h>

typedef struct Raw_data_manager Raw_data;

Raw_data* raw_data_create(void);
void raw_data_destroy(Raw_data* raw_data);
bool raw_data_full(const Raw_data* raw_data);
bool raw_data_empty(const Raw_data* raw_data);
void raw_data_add(Raw_data* raw_data, char data[], size_t data_size);
char* raw_data_get(Raw_data* raw_data);
void raw_data_lock(Raw_data* raw_data);
void raw_data_unlock(Raw_data* raw_data);
void raw_data_call_producer(Raw_data* raw_data);
void raw_data_call_consumer(Raw_data* raw_data);
void raw_data_wait_for_producer(Raw_data* raw_data);
void raw_data_wait_for_consumer(Raw_data* raw_data);
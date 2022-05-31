#include <stddef.h>
#include <stdbool.h>

typedef struct Ready_data_manager Ready_data;

Ready_data* ready_data_create(void);
void ready_data_destroy(Ready_data* read_data);
bool ready_data_full(const Ready_data* ready_data);
bool ready_data_empty(const Ready_data* ready_data);
void ready_data_add(Ready_data*, float data[], size_t elem_number);
float* ready_data_get(Ready_data* ready_data);
void ready_data_lock(Ready_data* ready_data);
void ready_data_unlock(Ready_data* ready_data);
void ready_data_call_producer(Ready_data* ready_data);
void ready_data_call_consumer(Ready_data* ready_data);
void ready_data_wait_for_proucer(Ready_data* ready_data);
void ready_data_wait_for_consumer(Ready_data* ready_data);

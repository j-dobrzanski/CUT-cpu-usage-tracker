#include "../include/ready_data.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

static void ready_data_test_create(){

    Ready_data* ready_data = ready_data_create();
    assert(ready_data != NULL);
    assert(ready_data_empty(ready_data) == true);
    assert(ready_data_full(ready_data) == false);

    ready_data_destroy(ready_data);
}

static void ready_data_test_add_single(){
    
    {
        Ready_data* ready_data = ready_data_create();
        float payload[] = {1.0, 10.5, 11.5};
        size_t elem_number = 3;
        ready_data_add(ready_data, payload, elem_number);

        assert(ready_data_empty(ready_data) == false);
        assert(ready_data_full(ready_data) == false);

        ready_data_destroy(ready_data);
    }

    {
        Ready_data* ready_data = ready_data_create();
        float* payload = NULL;
        ready_data_add(ready_data, payload, 0);

        assert(ready_data_empty(ready_data) == true);
        assert(ready_data_full(ready_data) == false);

        ready_data_destroy(ready_data);
    }

}

static void ready_data_test_fill(){
    Ready_data* ready_data = ready_data_create();

    float payload[] = {1.0, 10.5, 11.5};
    size_t elem_number = 3;

    while(ready_data_full(ready_data) == false){
        ready_data_add(ready_data, payload, elem_number);
        assert(ready_data_empty(ready_data) == false);            
    }
    assert(ready_data_full(ready_data) == true);

    ready_data_destroy(ready_data);
}

static void ready_data_test_fill_and_empty(){
    Ready_data* ready_data = ready_data_create();

    float payload[] = {1.0, 10.5, 11.5};
    size_t data_size = 3;

    while(ready_data_full(ready_data) == false){
        ready_data_add(ready_data, payload, data_size);
        assert(ready_data_empty(ready_data) == false);            
    }
    assert(ready_data_full(ready_data) == true);

    while(ready_data_empty(ready_data) == false){
        float* got_payload = ready_data_get(ready_data);
        assert(memcmp(payload, got_payload, data_size) == 0);
        free(got_payload);
    }
    assert(ready_data_empty(ready_data) == true);
    assert(ready_data_full(ready_data) == false);

    ready_data_destroy(ready_data);
}

int main(){
    ready_data_test_create();
    ready_data_test_add_single();
    ready_data_test_fill();
    ready_data_test_fill_and_empty();
    return 0;
}

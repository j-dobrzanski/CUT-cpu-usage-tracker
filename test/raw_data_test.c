#include "../include/raw_data.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

static void raw_data_test_create(){

    Raw_data* raw_data = raw_data_create();
    assert(raw_data != NULL);
    assert(raw_data_empty(raw_data) == true);
    assert(raw_data_full(raw_data) == false);

    raw_data_destroy(raw_data);
}

static void raw_data_test_add_single(){
    
    {
        Raw_data* raw_data = raw_data_create();
        char payload[] = "Ala ma kota";
        raw_data_add(raw_data, payload);

        assert(raw_data_empty(raw_data) == false);
        assert(raw_data_full(raw_data) == false);

        raw_data_destroy(raw_data);
    }

    {
        Raw_data* raw_data = raw_data_create();
        char* payload = NULL;
        raw_data_add(raw_data, payload);

        assert(raw_data_empty(raw_data) == true);
        assert(raw_data_full(raw_data) == false);

        raw_data_destroy(raw_data);
    }

    {
        Raw_data* raw_data = raw_data_create();
        char payload[] = "";
        raw_data_add(raw_data, payload);

        assert(raw_data_empty(raw_data) == false);
        assert(raw_data_full(raw_data) == false);

        raw_data_destroy(raw_data);
    }
}

static void raw_data_test_fill(){
    Raw_data* raw_data = raw_data_create();

    char payload[] = "Ala ma kota";

    while(raw_data_full(raw_data) == false){
        raw_data_add(raw_data, payload);
        assert(raw_data_empty(raw_data) == false);            
    }
    assert(raw_data_full(raw_data) == true);

    raw_data_destroy(raw_data);
}

static void raw_data_test_fill_and_empty(){
    Raw_data* raw_data = raw_data_create();

    char payload[] = "Ala ma kota";
    size_t data_size = strlen(payload) + 1;

    while(raw_data_full(raw_data) == false){
        raw_data_add(raw_data, payload);
        assert(raw_data_empty(raw_data) == false);            
    }
    assert(raw_data_full(raw_data) == true);

    while(raw_data_empty(raw_data) == false){
        char* got_payload = raw_data_get(raw_data);
        assert(memcmp(payload, got_payload, data_size) == 0);
        free(got_payload);
    }
    assert(raw_data_empty(raw_data) == true);
    assert(raw_data_full(raw_data) == false);

    raw_data_destroy(raw_data);
}

int main(){
    raw_data_test_create();
    raw_data_test_add_single();
    raw_data_test_fill();
    raw_data_test_fill_and_empty();
    return 0;
}

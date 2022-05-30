main_file = src/main.c
src_files = src/raw_data.c
test_files = test/raw_data_test.c
test_outs = $(notdir $(test_files:%=bin/%.o))



CC = clang 
CFLAGS = -Weverything

# CC = gcc 
# CFLAGS = -Wall -Wextra

main: $(src_files)
	$(CC) $(main_file) $(src_files) $(CFLAGS) -o main.out

debug: $(test_files)
	$(CC) $(src_files) $(CFLAGS) $< -o bin/$(test_outs)

clean:
	rm -f *.o
	rm -f test/*.o	

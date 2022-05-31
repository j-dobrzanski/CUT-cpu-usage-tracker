main_file = src/main.c
src_files = src/raw_data.c src/thread_analyzer.c
test_files = test/raw_data_test.c
test_outs = $(notdir $(test_files:%=bin/%.o))



CC = clang 
CFLAGS = -Weverything -ggdb3

# CC = gcc 
# CFLAGS = -Wall -Wextra

main: $(src_files)
	$(CC) $(main_file) $(src_files) $(CFLAGS) -o main.out

%.o: %.c
	$(CC) $(src_files) $(CFLAGS) $< -o bin/$(notdir $@)

clean:
	rm -f *.o
	rm -f bin/*.o

main_file = src/main.c
src_files = src/signal_handler.c src/raw_data.c src/thread_reader.c src/ready_data.c src/thread_analyzer.c src/thread_printer.c
test_outs = $(notdir $(test_files:%=bin/%.o))



CC = clang 
CFLAGS = -Weverything -ggdb3

# CC = gcc 
# CFLAGS = -Wall -Wextra

main: $(src_files)
	$(CC) $(main_file) $(src_files) $(CFLAGS) -o main.out -lpthread

%.o: %.c
	$(CC) $(src_files) $(CFLAGS) $< -o bin/$(notdir $@)

clean:
	rm -f *.o
	rm -f bin/*.o

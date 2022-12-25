.PHONY = all run clean
CC = gcc
FLAGS = -Wall -Werror -g

all: latency

# run:
# 	./benchmark
# 	rm socket clisocket servsocket

latency: latency.o
	$(CC) latency.o $(FLAGS) -o latency

latency.o: latency.c
	$(CC) $(FLAGS) -c latency.c

clean:
	rm -f *.a *.o *.so *.gch file.txt latency
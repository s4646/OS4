.PHONY = all run clean
CC = gcc
FLAGS = -Wall -Werror -g

all: latency

run:
	./latency

latency: latency.o
	$(CC) latency.o $(FLAGS) -lpthread -o latency

latency.o: latency.c
	$(CC) $(FLAGS) -c latency.c

clean:
	rm -f *.a *.o *.so *.gch file.txt latency
.PHONY = all run clean
CC = gcc
FLAGS = -Wall -Werror -g

all: fcntl

# run:
# 	./benchmark
# 	rm socket clisocket servsocket

fcntl: fcntl.o
	$(CC) fcntl.o $(FLAGS) -o fcntl

fcntl.o: fcntl.c
	$(CC) $(FLAGS) -c fcntl.c

# fcntl_helper: fcntl_helper.o
# 	$(CC) fcntl_helper.o $(FLAGS) -o fcntl_helper

# fcntl_helper.o: fcntl_helper.c
# 	$(CC) $(FLAGS) -c fcntl_helper.c

clean:
	rm -f *.a *.o *.so *.gch file.txt fcntl
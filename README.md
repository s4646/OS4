**Don't forget to compile the code using `make` command**

**Usage:**
```ruby
ubuntu@ubuntu-ASUS-TUF:~/Documents/OS4$ make
gcc -Wall -Werror -g -c latency.c
gcc latency.o -Wall -Werror -g -lpthread -o latency
ubuntu@ubuntu-ASUS-TUF:~/Documents/OS4$ make run
./latency
Wake a task using signal	- 0.438380355 seconds
Wake a task using lock		- 0.233320356 seconds
ubuntu@ubuntu-ASUS-TUF:~/Documents/OS4$ make clean
rm -f *.a *.o *.so *.gch file.txt latency
ubuntu@ubuntu-ASUS-TUF:~/Documents/OS4$
```
#include <time.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>
#include <pthread.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/types.h>

struct args
{
    int fd; int *count; pthread_mutex_t *lock;
};

int generate_file()
{
    int fd = open("file.txt", O_CREAT|O_WRONLY|O_TRUNC, 0644);
    if (fd == -1)
    {
        perror("Error: open");
        exit(1);
    }
    int num_written;
    if ((num_written = write(fd, "ABCDE", 5)) == -1)
    {
        perror("Error: write");
        exit(1);
    }
    close(fd);
    return 0;
}

int fcntl_locking(char *path, long *sec, long *nsec) // lock
{
    int fd = open(path, O_RDONLY);
    struct timespec *tv  = mmap(NULL, sizeof(struct timespec), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    struct timespec *tv2 = mmap(NULL, sizeof(struct timespec), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    bool *finished = mmap(NULL, sizeof(bool), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    *finished = false;

    if (!fork()) // child process
    {
        struct flock init, lock;
        lock.l_start   = 0;
        lock.l_whence  = SEEK_SET;
        lock.l_len     = 0;
        
        fcntl(fd, F_GETLK, &init);
        
        if (init.l_type == F_RDLCK || init.l_type == F_WRLCK)
        {
            printf("Error: file is already locked!\n");
            exit(1);
        }
        
        clock_gettime(CLOCK_REALTIME, tv);
        clock_settime(CLOCK_REALTIME, tv); // start time measure
        
        for (size_t i = 0; i < 1000000; i++)
        {
            lock.l_type = F_RDLCK;
            if (fcntl(fd, F_SETLKW, &lock) == -1)
            {
                perror("Error: fcntl F_SETKLW");
                exit(1);
            }
            
            lock.l_type = F_UNLCK;
            if (fcntl(fd, F_SETLKW, &lock) == -1)
            {
                perror("Error: fcntl F_SETLKW");
                exit(1);
            }
        }
        *finished = true;
        exit(0);
    }
    else
    {
        char c;
        struct flock lock;
        lock.l_start   = 0;
        lock.l_whence  = SEEK_SET;
        lock.l_len     = 0;

        while(!(*finished))
        {

            lock.l_type = F_WRLCK; // set default lock type to read
        
            if (fcntl(fd, F_GETLK, &lock) == -1)
            {
                perror("Error: fcntl F_GETLK");
                exit(1);
            }

            if (lock.l_type == F_RDLCK) // file is read-locked
            {
                continue;
            }
            else                        // file is not locked
            {
                if (read(fd, &c, 1) == -1)
                {
                    perror("Error: read");
                    exit(1);
                }
                if (lseek(fd, 0, SEEK_SET) == -1)
                {
                    perror("Error: lseek");
                    exit(1);
                }
            }
        }
        
        clock_gettime(CLOCK_REALTIME, tv2);
        clock_settime(CLOCK_REALTIME, tv2); // end time measure
        
        if (tv->tv_nsec > tv2->tv_nsec)
        {
            *sec += ((long)tv2->tv_sec-(long)tv->tv_sec) - 1;
            *nsec += 999999999-(long)tv->tv_nsec+(long)tv2->tv_nsec;
        }
        else
        {
            *sec += (long)tv2->tv_sec-(long)tv->tv_sec;
            *nsec += (long)tv2->tv_nsec-(long)tv->tv_nsec;
        }
        
        close(fd);
        munmap(tv,  sizeof(struct timespec));
        munmap(tv2, sizeof(struct timespec));
        munmap(finished, sizeof(bool));
    }
    return 0;
}

int pipe_locking(char *path, long *sec, long *nsec) // signal
{
    int fd = open(path, O_RDONLY);
    struct timespec *tv  = mmap(NULL, sizeof(struct timespec), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    struct timespec *tv2 = mmap(NULL, sizeof(struct timespec), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    int pfd[2];
    if (pipe(pfd) == -1)
    {
        perror("Error: pipe");
        exit(1);
    }

    if(!fork()) // child process
    {
        char c;

        clock_gettime(CLOCK_REALTIME, tv);
        clock_settime(CLOCK_REALTIME, tv); // start time measure

        for (size_t i = 0; i < 1000000; i++)
        {
            c = 'L';                       // lock file
            if (write(pfd[1], &c, 1) == -1)
            {
                perror("Error: write");
                exit(1);
            }
            c = 'O';                       // open file
            if (write(pfd[1], &c, 1) == -1)
            {
                perror("Error: write");
                exit(1);
            }
        }

        if (write(pfd[1], "C", 1) == -1)  // send close signal to parent
        {
            perror("Error: write");
            exit(1);
        }

        exit(0);
    }
    else // parent process
    {
        char c, cc;
        while(1)
        {
            if (read(pfd[0], &c, 1) == -1)
            {
                perror("Error: read");
                exit(1);
            }
            
            if (c == 'C')   // task is finished
            {
                break;
            }
            if (c == 'O')   // file is open
            {
                if (read(fd, &cc, 1) == -1)
                {
                    perror("Error: read");
                    exit(1);
                }
                if (lseek(fd, 0, SEEK_SET) == -1)
                {
                    perror("Error: lseek");
                    exit(1);
                }
            }
            if (c == 'L')   // file is locked
            {
                continue;
            }
        }

        clock_gettime(CLOCK_REALTIME, tv2);
        clock_settime(CLOCK_REALTIME, tv2); // end time measure

        if (tv->tv_nsec > tv2->tv_nsec)
        {
            *sec += ((long)tv2->tv_sec-(long)tv->tv_sec) - 1;
            *nsec += 999999999-(long)tv->tv_nsec+(long)tv2->tv_nsec;
        }
        else
        {
            *sec += (long)tv2->tv_sec-(long)tv->tv_sec;
            *nsec += (long)tv2->tv_nsec-(long)tv->tv_nsec;
        }
        
        close(fd);
        close(pfd[0]);
        close(pfd[1]);
        munmap(tv,  sizeof(struct timespec));
        munmap(tv2, sizeof(struct timespec));
    }

    return 0;
}

void* read_data_thread(void *a)
{
    char c;
    while (1)
    {   
        if (*(((struct args*)a)->count) >= 1000000)
        {
            break;
        }

        if (read(((struct args*)a)->fd, &c, 1) == -1)
        {
            perror("Error: read");
            exit(1);
        }

        if (lseek(((struct args*)a)->fd, 0, SEEK_SET) == -1)
        {
            perror("Error: lseek");
            exit(1);
        }
    }
    return NULL;
}

void* lock_data_thread(void *a)
{
    while (*(((struct args*)a)->count) < 1000000)
    {
        pthread_mutex_lock(((struct args*)a)->lock);
        *(((struct args*)a)->count) += 1;
        pthread_mutex_unlock(((struct args*)a)->lock);
    }
    return NULL;
}

int mutex_locking(char *path, long *sec, long *nsec) // lock
{
    struct args a;
    pthread_mutex_t lock;
    int fd = open(path, O_RDONLY);
    struct timespec tv, tv2;
    pthread_t thread1, thread2;
    int count = 0;
    
    a.fd = fd;
    a.count = &count;
    a.lock = &lock;

    if (pthread_mutex_init(&lock, NULL) != 0) // init lock
    {
        printf("mutex init has failed\n");
        exit(1);
    }

    clock_gettime(CLOCK_REALTIME, &tv);
    clock_settime(CLOCK_REALTIME, &tv); // start time measure

    pthread_create(&thread1, NULL, &read_data_thread, &a); /* create  */
    pthread_create(&thread2, NULL, &lock_data_thread, &a); /* threads */

    pthread_join(thread1, NULL); /* wait for */
    pthread_join(thread2, NULL); /* threads  */

    pthread_mutex_destroy(&lock); // destroy lock

    clock_gettime(CLOCK_REALTIME, &tv2);
    clock_settime(CLOCK_REALTIME, &tv2); // end time measure

    if (tv.tv_nsec > tv2.tv_nsec)
    {
        *sec += ((long)tv2.tv_sec-(long)tv.tv_sec) - 1;
        *nsec += 999999999-(long)tv.tv_nsec+(long)tv2.tv_nsec;
    }
    else
    {
        *sec += (long)tv2.tv_sec-(long)tv.tv_sec;
        *nsec += (long)tv2.tv_nsec-(long)tv.tv_nsec;
    }
    
    close(fd);
    return 0;
}

int main()
{
    long sec_signal = 0, nsec_signal = 0;
    long sec_lock = 0, nsec_lock = 0;
    generate_file();
    fcntl_locking("file.txt", &sec_lock, &nsec_lock);
    pipe_locking("file.txt", &sec_signal, &nsec_signal);
    mutex_locking("file.txt", &sec_lock, &nsec_lock);
    
    printf("Wake a task using signal\t- %ld.%ld\n", sec_signal, nsec_signal);
    printf("Wake a task using lock\t\t- %ld.%ld\n", sec_lock, nsec_lock);
    
    return 0;
}
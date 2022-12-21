#include <time.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/types.h>

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

int main()
{
    generate_file();
    
    int fd = open("file.txt", O_RDONLY);
    struct timespec *tv  = mmap(NULL, sizeof(struct timespec), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    struct timespec *tv2 = mmap(NULL, sizeof(struct timespec), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);

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

        return 0;
    }
    else
    {
        wait(NULL);
        
        clock_gettime(CLOCK_REALTIME, tv2);
        clock_settime(CLOCK_REALTIME, tv2); // end time measure
        
        long sec = (long)tv2->tv_sec-(long)tv->tv_sec;
        long nsec = (tv->tv_nsec > tv2->tv_nsec) ? 999999999-(long)tv->tv_nsec+(long)tv2->tv_nsec : (long)tv2->tv_nsec-(long)tv->tv_nsec;
        
        printf("Wake a task using signal - %ld.%ld\n", sec, nsec);
        close(fd);
        munmap(tv,  sizeof(struct timespec));
        munmap(tv2, sizeof(struct timespec));
    }
    
    return 0;
}
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>

#include "type.h"

#define SEM_KEY 1122334455

int *timer_count, *data_index_max;
int shmid[32], Bitmap_id, timer_id, shm_fd, user_id;
int controller_pid;
pid_t childpid1;

Requirement *shm_data[32];
bitmap *shm_bitmap;

typedef struct
{
    pthread_mutex_t mutex_child;
    pthread_cond_t cond;
} share_mut_cond;

share_mut_cond *shm_mut_cond;

typedef struct
{
    unsigned short data_time[32];
    char name[32][10];
}user;
user *user_info;

void signalHandler(int signum)
{
    int i;
    for (i=0; i<32; i++)
    {
        shmdt(shm_data[i]);
        shmctl(shmid[i], IPC_RMID, NULL);
    }
    shmdt(shm_bitmap);
    shmctl(Bitmap_id, IPC_RMID, NULL);
    shmdt(timer_count);
    shmctl(timer_id, IPC_RMID, NULL);
    shmdt(user_info);
    shmctl(user_id, IPC_RMID, NULL);
    munmap(shm_mut_cond, sizeof(share_mut_cond));
    close(shm_fd);
    exit(signum);
}

void timer_handler(int signum)
{
    ++(*timer_count);
}

void timer()
{
    struct sigaction sa;
    struct itimerval timer;
    memset (&sa, 0, sizeof (sa));
    sa.sa_handler = &timer_handler;
    sigaction (SIGVTALRM, &sa, NULL);
    timer.it_value.tv_sec = 1;
    timer.it_value.tv_usec = 0;
    timer.it_interval.tv_sec = 1;
    timer.it_interval.tv_usec = 0;
    setitimer (ITIMER_VIRTUAL, &timer, NULL);
}

int read_txt(Requirement *data, int index, char data_name[])
{
    FILE *file = fopen(data_name, "r");
    char buffer[100], *tok;
    while (fgets(buffer, sizeof(buffer), file) != NULL)
    {
        /*set data in share memory*/
        tok = strtok(buffer, " ");
        tok = strtok(NULL, " ");
        strcpy(user_info->name[index], tok);
        tok = strtok(NULL, " ");
        tok = strtok(NULL, " ");
        int type = atoi(tok);
        if(type == 0) data[index].type = NORMAL;
        else if (type == 1) data[index].type = DISABLE;
        tok = strtok(NULL, " ");
        tok = strtok(NULL, " ");
        user_info->data_time[index] = atoi(tok);
        tok = strtok(NULL, " ");
        tok = strtok(NULL, " ");
        data[index].start = atoi(tok);
        tok = strtok(NULL, " ");
        tok = strtok(NULL, " ");
        data[index].destination = atoi(tok);
        tok = strtok(NULL, " ");
        tok = strtok(NULL, " ");
        if ((data[index].destination - data[index].start) > 0)
            data[index].direction = UP;
        else
            data[index].direction = DOWN;
        data[index].state = WAIT;
        memset(buffer, 0, sizeof(buffer));
        index++;
    }
    fclose(file);
    return index;
}

void parentfunc(Requirement *data, int data_max)
{
    int old_count = -2, kill2control, i;
    while(1)
    {
        if (*timer_count != old_count)
        {
            i = 0;
            kill2control = 0;
            old_count = *timer_count;
            while (i < data_max)
            {
                if (user_info->data_time[i] == old_count)
                {
                    int i_bitmap = 0;
                    while ((shm_bitmap->one & (1 << i_bitmap)) && i_bitmap < 32)
                    {
                        i_bitmap++;
                    }

                    if (i_bitmap < 32)
                    {
                        shm_bitmap->one |= 1 << i_bitmap;
                   
                        shm_data[i]->start = data[i].start;
                        shm_data[i]->destination = data[i].destination;
                        shm_data[i]->direction = data[i].direction;
                        shm_data[i]->state = data[i].state;
                        shm_bitmap->one_to_zero |= 1 << i_bitmap; 
                        kill2control = 1; /* signal to controller , controller need to renew*/
                    }
                }
                i++;
            }
            if (kill2control)
                kill(controller_pid, SIGUSR1);
        }
    }
}

void childfunc()
{
    int i, j;
    FILE *file1 = fopen("finish.txt", "w");
    fclose(file1);
    file1 = fopen("finish.txt", "a");
    while(1)
    {
        pthread_mutex_lock(&shm_mut_cond->mutex_child);

        /* wait controller signal */
        pthread_cond_wait(&shm_mut_cond->cond, &shm_mut_cond->mutex_child);

        /* receive controller signal */
        /* renew I Check bitmap and O to I bitmap*/
        i = 0;
        while (i < 32)
        {
            if (shm_bitmap->zero_to_one & 1 << i)
            {
                if (shm_data[i]->state == ARRIVED)
                {
                    shm_bitmap->zero_to_one ^= 1 << i;
                    shm_bitmap->one ^= 1 << i;
                    printf("[time : %d] Passenger: %s\n", *timer_count, user_info->name[i]);
                    printf("            Service: Leave the Elevator\n");
                    printf("            Floor: %hu\n", shm_data[i]->destination);
                    printf("-----------------------------------------\n");
                    // printf("[time : %d] %s from location %hu to destination %hu finish !\n", *timer_count, user_info->name[i], shm_data[i]->start, shm_data[i]->destination);
                    // fprintf(file1, "[time : %d] %s from location %hu to destination %hu finish !\n", *timer_count, user_info->name[i], shm_data[i]->start, shm_data[i]->destination);
                    fprintf(file1, "[time : %d] Passenger: %s, Need: From floor %hu to floor %hu , State: Finish !\n", *timer_count, user_info->name[i], shm_data[i]->start, shm_data[i]->destination);
                }
                else if (shm_data[i]->state == INSIDE)
                {
                    shm_bitmap->zero_to_one ^= 1 << i;
                    // printf("[time : %d] %s in elevator\n", *timer_count, user_info->name[i]);
                    printf("[time : %d]  Passenger: %s\n", *timer_count, user_info->name[i]);
                    printf("            Service: Enter the Elevator\n");
                    printf("            Floor: %hu\n", shm_data[i]->start);
                    printf("-----------------------------------------\n");
                }
            }
            i++;
        }
        pthread_mutex_unlock(&shm_mut_cond->mutex_child);
    }
}

int main(int argc, char *argv[])
{
    int i, data_index = 0;
    signal(SIGINT, signalHandler);
    if (argc != 3)
    {
        printf("./IO [pid] [data.txt]\n");
        return -1;
    }
    controller_pid = atoi(argv[1]);
    char data_name[20];
    strcpy(data_name, argv[2]);
    Requirement data[64];
    /* create share memory */
    key_t key_data[32], key_bitmap = 1111, key_timer = 4444, key_userinfo = 5555;

    user_id = shmget(key_userinfo, sizeof(user), IPC_CREAT | 0666);
    user_info = (user *)shmat(user_id, NULL, 0);

    timer_id = shmget(key_timer, sizeof(int), IPC_CREAT | 0666);
    timer_count = (int *)shmat(timer_id, NULL, 0);
    *timer_count = -1;
    timer();
    shm_fd = shm_open("/sharememory", O_RDWR, 0666);
    shm_mut_cond = mmap(NULL, sizeof(share_mut_cond), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    for (i=1; i <= 32; i++)
    {
        key_data[i-1] = i;
        shmid[i-1] = shmget(key_data[i-1], sizeof(Requirement),IPC_CREAT | 0666);
        shm_data[i-1] = (Requirement*)shmat(shmid[i-1], NULL, 0);
    }

    Bitmap_id = shmget(key_bitmap, sizeof(bitmap),IPC_CREAT | 0666);
    shm_bitmap = (bitmap *)shmat(Bitmap_id, NULL, 0);

    data_index = read_txt(data, data_index, data_name);
    fflush(stdin);
    
    childpid1 = fork();
    if (childpid1 >= 0) /* fork succeeded */
    {
        if (childpid1 == 0)
            childfunc();
        else
        {
            parentfunc(data, data_index);
        }
    }
    return 0;
}
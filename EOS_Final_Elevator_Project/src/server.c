//**********include libraries***********//
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
#include "sockop.h"
#include "type.h"

//**********Parameter Define***********//
#define SEM_KEY 1122334455
#define BUFFER_SIZE 1024
#define SERVICE "tcp"
#define NUMOFTHREADS 100

//**********Parameter Define***********//
//timer
int *timer_count, *data_index_max;

//descriptor
int controller_pid, server_fd, client_fd;
pid_t childpid1;

//share memory
int shmid[32], Bitmap_id, timer_id, shm_fd, user_id;
Requirement *shm_data[32];
bitmap *shm_bitmap;
int *shm_flags; //shm_flags[0]: open, shm_flags[1]: close

//mutex condition
typedef struct {
    pthread_mutex_t mutex_child;
    pthread_cond_t cond;
} share_mut_cond;
share_mut_cond *shm_mut_cond;
int kill2control = 0;
pthread_mutex_t data_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t open_close_mutex = PTHREAD_MUTEX_INITIALIZER;

//user data 
typedef struct {
    unsigned short data_time[32];
    char name[32][10];
} user;
user *user_info;
Requirement data[64];
int data_index = 0;

// thread id
int total_thread_count=0;
__thread int current_thread_id = 0;

//flags
int openflag = 0, closeflag = 0;

//**********sigint handler**********//
void signalHandler(int signum) 
{
    pthread_cancel(childpid1);
    pthread_join(childpid1, NULL);
    for (int i = 0; i < 32; i++) 
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
    shmdt(shm_flags);
    shmctl(shm_fd, IPC_RMID, NULL);
    munmap(shm_mut_cond, sizeof(share_mut_cond));
    close(shm_fd);
    close(server_fd);
    if (client_fd != -1) {
        close(client_fd);
    }
    exit(signum);
}
//**********sigint handler**********//

//**********timer functions**********//
void timer_handler(int signum) 
{
    ++(*timer_count);
}

void timer() 
{
    struct sigaction sa;
    struct itimerval timer;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = &timer_handler;
    sigaction(SIGVTALRM, &sa, NULL);
    timer.it_value.tv_sec = 1;
    timer.it_value.tv_usec = 0;
    timer.it_interval.tv_sec = 1;
    timer.it_interval.tv_usec = 0;
    setitimer(ITIMER_VIRTUAL, &timer, NULL);
}
//**********timer functions**********//

//**********socket functions**********//
int read_socket_data(Requirement *data, int *data_index, char buffer[]) 
{
    printf("-----------------------------------------\n");
    printf("Received message: %s\n", buffer);
    char *tok;
    tok = strtok(buffer, " ");
    tok = strtok(NULL, " ");
    printf("Name: %s ", tok);
    strcpy(user_info->name[*data_index], tok);
    tok = strtok(NULL, " ");
    tok = strtok(NULL, " ");
    printf("Type: %s ", tok);
    int type = atoi(tok);
    if (type == 0)
        data[*data_index].type = NORMAL;
    else if (type == 1)
        data[*data_index].type = DISABLE;
    // tok = strtok(NULL, " ");
    // tok = strtok(NULL, " ");
    // printf("Time: %s ", tok);
    // user_info->data_time[*data_index] = atoi(tok);
    tok = strtok(NULL, " ");
    tok = strtok(NULL, " ");
    printf("Start: %s ", tok);
    data[*data_index].start = atoi(tok);
    tok = strtok(NULL, " ");
    tok = strtok(NULL, " ");
    printf("Destination: %s ", tok);
    data[*data_index].destination = atoi(tok);
    tok = strtok(NULL, " ");
    tok = strtok(NULL, " ");
    printf("Direction: %s \n", tok);
    if ((data[*data_index].destination - data[*data_index].start) > 0)
        data[*data_index].direction = UP;
    else
        data[*data_index].direction = DOWN;
    data[*data_index].state = WAIT;
    (*data_index)++;
    return 0;
}
//**********socket functions**********//

//**********thread functions**********//
void* thread_function(void* client_fd_ptr)
{
    //socket
    int client_fd = *(int*)client_fd_ptr;
    current_thread_id = client_fd;
    printf("Dealing Sharememory in thread #%d\n", current_thread_id);
    char received_message[BUFFER_SIZE], msg[BUFFER_SIZE];
    while (1) 
    {
        int bytes_read = read(client_fd, received_message, BUFFER_SIZE);
        if (bytes_read > 0) 
        {
            received_message[bytes_read] = '\0'; // Null-terminate the string
            read_socket_data(data, &data_index, received_message);
            int i = data_index - 1;
            kill2control = 0;
            while (1) 
            {
                pthread_mutex_lock(&data_mutex);
                int i_bitmap = 0;
                while ((shm_bitmap->one & (1 << i_bitmap)) && i_bitmap < 32) {
                    i_bitmap++;
                }
                if (i_bitmap < 32) {
                    shm_bitmap->one |= 1 << i_bitmap;
                    shm_data[i]->start = data[i].start;
                    shm_data[i]->destination = data[i].destination;
                    shm_data[i]->direction = data[i].direction;
                    shm_data[i]->state = data[i].state;
                    shm_bitmap->one_to_zero |= 1 << i_bitmap;
                    kill2control = 1; /* signal to controller , controller need to renew */
                }
                // Add debug print statements
                printf("Copied data to shared memory: start=%d, destination=%d, direction=%d\n", shm_data[i]->start, shm_data[i]->destination, shm_data[i]->direction);
                printf("-----------------------------------------\n");
                pthread_mutex_unlock(&data_mutex);
                break;
            }
            if (kill2control)
                kill(controller_pid, SIGUSR1);

            while (1) 
            {
                pthread_mutex_lock(&shm_mut_cond->mutex_child);
                /* wait controller signal */
                pthread_cond_wait(&shm_mut_cond->cond, &shm_mut_cond->mutex_child);
                /* receive controller signal */
                /* renew I Check bitmap and O to I bitmap */
                i = 0;
                while (i < 32) 
                {
                    if (shm_bitmap->zero_to_one & 1 << i) 
                    {
                        if (shm_data[i]->state == ARRIVED) 
                        {
                            shm_bitmap->zero_to_one ^= 1 << i;
                            shm_bitmap->one ^= 1 << i;
                            printf("[Info] Passenger: %s\n", user_info->name[i]);
                            printf("            Thread ID: %d\n",current_thread_id);
                            printf("            Service: Leave the Elevator\n");
                            printf("            Floor: %hu\n", shm_data[i]->destination);
                            shm_flags[1] = 1; // set Arriveflag
                            char *message = "leave";
                            write(client_fd, message, strlen(message));
                            printf("Send %s to client #%d\n",message ,current_thread_id);
                            printf("-----------------------------------------\n");
                        } 
                        else if (shm_data[i]->state == INSIDE) 
                        {
                            shm_bitmap->zero_to_one ^= 1 << i;
                            printf("[Info]  Passenger: %s\n", user_info->name[i]);
                            printf("            Service: Enter the Elevator\n");
                            printf("            Floor: %hu\n", shm_data[i]->start);
                            shm_flags[0] = 1; // set Insideflag
                            char *message = "enter";
                            write(client_fd, message, strlen(message));
                            printf("Send %s to client #%d\n",message ,current_thread_id);
                            printf("-----------------------------------------\n");
                        }
                    }
                    i++;
                }
                pthread_mutex_unlock(&shm_mut_cond->mutex_child);
            }    
        }
        else 
        {
            printf("Can't read data\n");
        }
    }
}
//**********thread functions**********//

void handle_request() 
{
    int i;
    while (1) 
    {
        char buffer[BUFFER_SIZE];
        client_fd = accept(server_fd, NULL, NULL);
        if (client_fd < 0) 
        {
            perror("accept");
            continue;
        }

        //allocate client_fd pointer 
        int *client_fd_ptr = malloc(sizeof(int));
        if (client_fd_ptr == NULL)
            errexit("Memory allocation error\n");
        *client_fd_ptr = client_fd;
        int thread_nums = 0;
        pthread_t thread[NUMOFTHREADS];
        pthread_create(&thread[thread_nums++], NULL, thread_function, (void *)client_fd_ptr); 
        pthread_detach(thread[thread_nums - 1]);
    }
}
//**********child & parent functions**********//

//main function
int main(int argc, char *argv[]) 
{
    signal(SIGINT, signalHandler);
    if (argc != 3) 
    {
        printf("./server [pid] [PORT]\n");
        return -1;
    }
    controller_pid = atoi(argv[1]);
    char port[20];
    strcpy(port, argv[2]);
    
    // create share memory
    key_t key_data[32], key_bitmap = 1111, key_timer = 4444, key_userinfo = 5555, key_flags = 6666;

    user_id = shmget(key_userinfo, sizeof(user), IPC_CREAT | 0666);
    user_info = (user *)shmat(user_id, NULL, 0);

    timer_id = shmget(key_timer, sizeof(int), IPC_CREAT | 0666);
    timer_count = (int *)shmat(timer_id, NULL, 0);
    *timer_count = 0;
    timer();
    shm_fd = shm_open("/sharememory", O_RDWR, 0666);
    shm_mut_cond = mmap(NULL, sizeof(share_mut_cond), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    for (int i = 1; i <= 32; i++) {
        key_data[i - 1] = i;
        shmid[i - 1] = shmget(key_data[i - 1], sizeof(Requirement), IPC_CREAT | 0666);
        shm_data[i - 1] = (Requirement *)shmat(shmid[i - 1], NULL, 0);
    }

    Bitmap_id = shmget(key_bitmap, sizeof(bitmap), IPC_CREAT | 0666);
    shm_bitmap = (bitmap *)shmat(Bitmap_id, NULL, 0);

    int flags_id = shmget(key_flags, 2 * sizeof(int), IPC_CREAT | 0666);
    shm_flags = (int *)shmat(flags_id, NULL, 0);
    shm_flags[0] = 0;
    shm_flags[1] = 0;

    // create socket and listen for connections
    server_fd = passivesock(port, SERVICE, 32);

    handle_request();
    return 0;
}

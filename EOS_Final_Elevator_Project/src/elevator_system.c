//standard library
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

//signal & threads
#include <signal.h>
#include <pthread.h>

//modules
#include "bitmap.h"
#include "elevator_tool.h"
#include "device_output.h"
#include "kernel_tool.h"

int notify_flag = 0;
int shmid[32], BitShmid, shm_fd, shmid_Ele[2];

void Elevator_Controller(ElevatorState **Ele);
void sigint_handler(int sig);
void notify_handler (int signo, siginfo_t *info, void *context);

/**************** Elevator Controller ****************/
void Elevator_Controller(ElevatorState **Ele)
{
    printf("**************Elevator Activate**************\n");
    unsigned short destination;
    int index, one_count, one_index[32];
    bool useFlag = false; //determine elevator is been used or not

    while(1)
    {
        if(notify_flag) //new request
        {
            // fflush(stdout);
            if(!useFlag) useFlag = true;
            notify_flag = false;
            if(shm_bitmap->one_to_zero > 0) // The elevator is empty, no one gone out
            {
                if(!(Ele[0]->state==STOP && Ele[1]->state==STOP))
                {   
                    find_all_bitmap_1_index(shm_bitmap->one_to_zero, &one_count, one_index);
                    shm_bitmap->one_to_zero = 0;
                    for(int i = 0; i < one_count; i++)
                    {
                        index = one_index[i]; 
                        AllocLongerDest(p_Requirement[index], Ele);    
                    }
                }
                else
                    shm_bitmap->one_to_zero = 0; // All the passenger is out
            }
        }

        if(useFlag)
        {
            if(Ele[0]->state == STOP)
            {
                destination = ForSTOPchoseDest(Ele[0], Ele[1]);
                if(destination != 0)
                {
                    if(destination == Ele[0]->location) // the elevator arrived the destination
                    {
                        Ele[0]->state = OPEN;
                    }
                    else // control the direction
                    {
                        Ele[0]->destination = destination;
                        Ele[0]->state = MOVE;
                        if((destination-Ele[0]->location) > 0) Ele[0]->direction = UP;
                        else if((destination-Ele[0]->location) < 0) Ele[0]->direction = DOWN;
                    }
                    // printf("elevator1:destination is %d\n",Ele[0]->destination);
                    printf("[INFO] Elevator 1: Set destination %d\n",Ele[0]->destination);
                    printf("-----------------------------------------\n");
                }
            }
            if(Ele[1]->state == STOP)
            {
                destination = ForSTOPchoseDest(Ele[1], Ele[0]);
                if(destination != 0)
                { 
                    if(destination == Ele[1]->location)
                    {
                        Ele[1]->state = OPEN;
                    }
                    else
                    {
                        Ele[1]->destination = destination;
                        Ele[1]->state = MOVE;
                        if((destination-Ele[1]->location) > 0) Ele[1]->direction = UP;
                        else if((destination-Ele[1]->location) < 0) Ele[1]->direction = DOWN;
                    }
                    printf("[INFO] Elevator 2: Set destination %d \n",Ele[1]->destination);
                    printf("-----------------------------------------\n");
                }
            }      
        }
    }
}

void sigint_handler(int sig)
{
    printf("Elevator[%d] is Done\n", getpid());
    
	/* remove semaphore */
    semctl (sem_device, 0, IPC_RMID, 0);
    
    /* remove shared memory */
    for(int i=0; i<32; i++)
    {
        shmdt(p_Requirement[i]);
        shmctl(shmid[i], IPC_RMID, NULL);
    }
    shmdt(shm_bitmap);
    shmctl(BitShmid, IPC_RMID, NULL);
    shmctl(shmid_Ele[0], IPC_RMID, NULL);
    shmctl(shmid_Ele[1], IPC_RMID, NULL);
    
    munmap(shared_resource, sizeof(SharedResource));
    close(shm_fd);
	exit(0);
}

void notify_handler (int signo, siginfo_t *info, void *context)
{
    notify_flag = 1;
    printf("[INFO] Get New Request\n");
    printf("-----------------------------------------\n");
}

int main(int argc, char *argv[])
{
    signal(SIGINT, sigint_handler);   
    pid_t child1, child2;
    ElevatorState* Ele[2];

    /**** signal initialization ****/
    struct sigaction sa;
    memset(&sa, 0, sizeof (sa));
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = &notify_handler;
    sigaction (SIGUSR1, &sa, NULL);

    /**** create share memory ****/
    p_Requirement = create_shm(shmid);
    shm_bitmap = create_shm_Bit(&BitShmid);
    Ele[0] = create_shm_Ele(&shmid_Ele[0], 1313, 1);
    Ele[1] = create_shm_Ele(&shmid_Ele[1], 3113, 2);

    child1 = fork();
    child2 = fork();

    /* child1 process */
    if (child1 != 0 && child2 == 0) 
    {
        /* create semaphore for device output 1 */
        create_semaphore(&sem_device, SEM_KEY_DEVICE);
        // elevator1_device_output(Ele[0]);
    }
    /* child2 process */
    else if (child1 == 0 && child2 != 0) 
    {
        /* create semaphore for device output 2 */
        create_semaphore(&sem_device, SEM_KEY_DEVICE);
        // elevator2_device_output(Ele[1]);
    }
    /* parent process */
    else if(child1 != 0 && child2 != 0)
    {
        printf("********** Controller PID: %d **********\n", getpid());

        pthread_t thread[2];
        unsigned short destination;

        /* create semaphore for ele */
        create_semaphore(&sem_ele, SEM_KEY_ELE);

        for(int i = 0; i < 2; i++){
            pthread_create(&thread[i], NULL, elevator, (void *)Ele[i]);
            pthread_detach(thread[i]);
        }
        Elevator_Controller(Ele);
    }
    else
    {
        /* to solve the problem that can't use ctrl-c */
        /* if one of process or thread finishes, we can't use SIGINT(ctrl-c) */
        while(1)
            usleep(1000000);
    }
    return 0;
}


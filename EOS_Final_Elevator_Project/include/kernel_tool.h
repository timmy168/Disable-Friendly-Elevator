#ifndef KERNELTOOL
#define KERNELTOOL

#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>

#include "type.h"

#define SEM_MODE 0666

SharedResource *shared_resource;

// Accquire Semaphore
int P (int s);

// Release Semaphore
int V (int s);

// create shared memory of passenger's requirement
Requirement** create_shm(int* shmid);

// create a bit map that contains passenger's imformation
bitmap* create_shm_Bit(int* shmid);

// create a share memory of elevator's state
ElevatorState* create_shm_Ele(int* shmid, key_t key_bitmap, unsigned short id);

//create semaphore
int create_semaphore(int* sem, int key);

//create mutex lock
SharedResource* create_mutex_lock(int shm_fd);

#endif
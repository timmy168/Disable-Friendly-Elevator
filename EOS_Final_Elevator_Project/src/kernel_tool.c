#include "kernel_tool.h"

// Accquire Semaphore
int P (int s)
{
    struct sembuf sop;
    sop.sem_num = 0;
    sop.sem_op = -1;
    sop.sem_flg = 0;
}

// Release Semaphore
int V(int s)
{
    struct sembuf sop;
    sop.sem_num = 0;
    sop.sem_op = 1;
    sop.sem_flg = 0;
}

// Create Semaphore
int create_semaphore(int* sem, int key)
{
    *sem = semget(key, 1, IPC_CREAT | SEM_MODE);
    semctl(*sem, 0, SETVAL, 1);
}

// Create Shared Memory of the Passenger's Requirement
Requirement** create_shm(int* shmid)
{
    int i;
    //maximun 32 request
    key_t key_data[32], key_bitmap = 1111; //data's index and shared memory's index 
    Requirement **shm_data = (Requirement**)malloc(sizeof(Requirement*)*32);

    for (i=1; i <= 32; i++)
    {
        key_data[i-1] = i;
        //sharedmemory ID
        shmid[i-1] = shmget(key_data[i-1], sizeof(Requirement), IPC_CREAT | 0666);
        shm_data[i-1] = (Requirement*)shmat(shmid[i-1], NULL, 0);
    }
    return shm_data;
}

bitmap* create_shm_Bit(int* shmid)
{
    bitmap* shm_bitmap;
    key_t key_bitmap = 1111;

    *shmid = shmget(key_bitmap, sizeof(bitmap), IPC_CREAT | 0666);
    shm_bitmap = (bitmap *)shmat(*shmid, NULL, 0);
    /* set bitmap initial value */
    shm_bitmap->one_to_zero = 0;
    shm_bitmap->one = 0;
    shm_bitmap->zero_to_one = 0;

    return shm_bitmap;
}

ElevatorState* create_shm_Ele(int* shmid, key_t key_bitmap, unsigned short id)
{
    ElevatorState* shm_ele;
    *shmid = shmget(key_bitmap, sizeof(ElevatorState), IPC_CREAT | 0666);
    shm_ele = (ElevatorState *)shmat(*shmid, NULL, 0);
    shm_ele->state = STOP;
    shm_ele->direction = NONE;
    //initial at 5th floor
    shm_ele->location = 5;
    shm_ele->destination = 5;
    shm_ele->id = id;
    shm_ele->people = 0;

    return shm_ele;
}

SharedResource* create_mutex_lock(int shm_fd)
{
    shm_fd = shm_open("/sharememory", O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, sizeof(SharedResource));
    shared_resource = mmap(NULL, sizeof(SharedResource), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    pthread_mutexattr_t mutex_attr;
    pthread_mutexattr_init(&mutex_attr);
    pthread_mutexattr_setpshared(&mutex_attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&shared_resource->mutex, &mutex_attr);
    
    pthread_condattr_t cond_attr;
    pthread_condattr_init(&cond_attr);
    pthread_condattr_setpshared(&cond_attr, PTHREAD_PROCESS_SHARED);
    pthread_cond_init(&shared_resource->cond, &cond_attr);

    return shared_resource;
}
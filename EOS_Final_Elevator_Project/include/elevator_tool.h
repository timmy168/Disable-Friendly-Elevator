#ifndef ELEVATOR
#define ELEVATOR
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>
#include <pthread.h>
#include <sys/mman.h>
#include <fcntl.h>

#include "type.h"
#include "bitmap.h"
#include "kernel_tool.h"

#define SEM_KEY_ELE 2030
#define CAPACITY 10

//check the elevator's capacity
bool check_space[32];

//handle new request
bool NewRequest[32];

bool getNewRequest;

//elevator semaphore
int sem_ele;

Requirement **p_Requirement;
bitmap *shm_bitmap;

bool isFurther(unsigned short des, ElevatorState ele);
bool isFull(ElevatorState* ele);
bool get_people(ElevatorState* ele, bool* get_passage);
bool putdown_people(ElevatorState* ele, bool *passage);
void *elevator(void *parm);
int TravelCost(PassengerType type, unsigned short des, Direction dir, ElevatorState ele);
bool AllocLongerDest(Requirement* peo, ElevatorState **ele);
unsigned short ForSTOPchoseDest(ElevatorState *main, ElevatorState *another); 

#endif
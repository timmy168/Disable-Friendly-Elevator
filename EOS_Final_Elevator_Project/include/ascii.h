/*Implement the ascii art*/
#ifndef ASCII
#define ASCII
#include <stdio.h>
#include <unistd.h> // For usleep()

void clearScreen();
//normal
void printOpenDoorFrame1();
void printOpenDoorFrame2();
void printOpenDoorFrame3();
void printOpenDoorFrame4();
void door();
//disable
void printOpenDoorFrame1_d();
void printOpenDoorFrame2_d();
void printOpenDoorFrame3_d();
void printOpenDoorFrame4_d();
void printOpenDoorFrame5_d();
void enter_d();
void leave_d();

#endif
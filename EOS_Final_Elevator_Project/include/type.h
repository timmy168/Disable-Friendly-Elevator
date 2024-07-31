/*Defines the self define enum and struct*/

#ifndef TYPE
#define TYPE

// Elevator Status
typedef enum{STOP, MOVE, OPEN} ElevatorStatus;

// Passenger Status
typedef enum{WAIT, INSIDE, ARRIVED} PassengerStatus;

// Passenger Type (Normal People v.s. iProfessor Wu, Bing-Fe)
typedef enum{NORMAL, DISABLE} PassengerType;

// Direction
typedef enum{NONE, UP, DOWN} Direction;

// The Current State Of the Elevator
typedef struct{
    ElevatorStatus state;
    Direction direction;
    unsigned short location;
    unsigned short destination;
    unsigned short id;
    unsigned short people;
}ElevatorState;

// The Requirement of the Passenger, include destination, starting floor, Passenger State, and Direction
typedef struct{
    unsigned short destination;
    unsigned short start;
    PassengerStatus state;
    PassengerType type;
    Direction direction;
}Requirement;

// Bit 
// one: The floor that contains passenger's requirements
// one_to_zero: The floor that the passenger gone out from the elevator
// zero_to_one: The floor that the passenger gone in to the elevator
// When the unsigned int is 1 => The event happened
// Using find_all_bitmap_1_index() to know the envent's status
typedef struct{
    unsigned int one;
    unsigned int one_to_zero;
    unsigned int zero_to_one;
}bitmap;

// Mutex and Share Condition
typedef struct
{
    pthread_mutex_t mutex;
    pthread_cond_t cond;
}SharedResource;

#endif
#include "elevator_tool.h"

//Check the passenger's destination is further than elevator's destination or not
bool isFurther(unsigned short destination, ElevatorState ele)
{
    int distance = destination - ele.destination;
    
    if(ele.direction == UP && distance > 0) return true; //going upstairs, cost > 0 => further
    else if (ele.direction == DOWN && distance < 0) return true; //going downstairs, cost < 0 => further
    else if (ele.direction == NONE) return false;
    else return false;
}

bool isFull(ElevatorState* ele)
{
    if(ele->people == CAPACITY) return true;
    else return false;
}

// let the passenger go in the elevator
bool get_people(ElevatorState* ele, bool* passenger)
{
    //ret: whether someone go into the elevator or not
    bool ret = false; 

    // bitmap operation
    int index, one_count, one_index[32];

    // Find the floor that the passenger intend to go in or out
    find_all_bitmap_1_index(shm_bitmap->one, &one_count, one_index);

    //**********critical session**********//
    //accquire semaphore, ensure there is only one thread using the shared resourece 
    P(sem_ele);

    //Traverse all the floor that passenger intend to go in or out
    for(int i = 0; i < one_count; i++)
    {
        // get the floor that the passenger would go in
        index = one_index[i];

        // check the passenger's waiting floor is the same as the elevator floor or not 
        if((p_Requirement[index]->start==ele->location) && (p_Requirement[index]->state==WAIT) && !isFull(ele))
        {
            // If the elevator is not moving (No one is using)
            // or the passenger intend's direction is the same as the elevator
            // let the passenger go in
            if((ele->direction==NONE) || (p_Requirement[index]->direction==ele->direction))
            {
                //if no oneuse the elevator, set the direction of the elevator is the same as the passenger
                if(ele->direction == NONE) ele->direction = p_Requirement[index]->direction;

                //passenger go inside
                p_Requirement[index]->state = INSIDE;
                if(p_Requirement[index]->type == NORMAL) ele->people += 1;
                else if(p_Requirement[index]->type == DISABLE) ele->people += 3;

                //sucessfully get the passenger at the floor contains request
                passenger[index] = 1;

                //set zero_to_one to 1 
                set_bitmap_1(&shm_bitmap->zero_to_one, index);

                // printf("[SERVE] Elevator %d: Get people\n", ele->id);

                // If the passsenger's desitantion is furder than the elevator's destination, update the destination 
                if(isFurther(p_Requirement[index]->destination, *ele))
                    ele->destination = p_Requirement[index]->destination;

                //set ret => true, means someone enter the elevator
                ret = true;
            }
        }
    }
    //**********critical session**********//

    // Release semaphore
    V(sem_ele);

    return ret;
}

// let the passenger go out the elevator
bool putdown_people(ElevatorState* ele, bool *passenger)
{
    //ret: whether someone go into the elevator or not
    bool ret = false;

    // bitmap operation
    int index, one_count, one_index[32];

    // Find the floor that the passenger intend to go in or out
    find_all_bitmap_1_index(shm_bitmap->one, &one_count, one_index);

    //accquire semaphore, ensure there is only one thread using the shared resourece 
    P(sem_ele);

    //**********critical session**********//
    //Traverse all the floor that passenger intend to go in or out
    for(int i = 0; i < one_count; i++)
    {
        
        //If the passenger intend's direction is the same as the elevator current location
        // let the passenger go out
        if(passenger[index] && p_Requirement[index]->destination == ele->location)
        {
            //passenger go out
            p_Requirement[index]->state = ARRIVED;

            //sucessfully outdown the passenger at the floor contains request
            passenger[index] = 0;
            if(p_Requirement[index]->type == NORMAL) ele->people -= 1;
            else if(p_Requirement[index]->type == DISABLE) ele->people -= 3;

            // set_bitmap_0(&shm_bitmap->I, index);

            set_bitmap_1(&shm_bitmap->zero_to_one, index);

            // printf("[SERVE] Elevator %d: Put people\n", ele->id);

            //set ret => true, means someone leave the elevator
            ret = true;
        }
    }
    //**********critical session**********//

    // release semaphore
    V(sem_ele);
    return ret;
}

// elevator operation function, updates the elevator status
void *elevator(void *parm)
{
    ElevatorState *Ele = (ElevatorState*)parm;

    //determine which elvator
    short ID = Ele->id;

    //Elevator Passenger opeates max 32 request
    bool passenger[32] = {0}, getPeoF, putPeoF, full;

    // Time
    short TimeFlag;
    struct timespec request_time, remain_time; //request sleeping time and remain sleepting time

    memset(&request_time, 0, sizeof(struct timespec));
    request_time.tv_nsec = 0;

    //shared memory descriptor
    int shm_fd;
    SharedResource *shm_mut_cond;
    shm_mut_cond = create_mutex_lock(shm_fd);
    
    while(1)
    {
        //When the elevator is moving
        if(Ele->state != STOP)
        {  
            //check whether the elvator is letting people in our out
            full = isFull(Ele);
            getPeoF = get_people(Ele, passenger);
            putPeoF = putdown_people(Ele, passenger);

            //some one is entering or leaving
            if(getPeoF || putPeoF)
            {
                Ele->state = OPEN;
                printf("[STATE] Elevator %d: Open at %d\n", ID, Ele->location);
                if(getPeoF) printf("[SERVE] Elevator %d: Get people\n", Ele->id);
                if(putPeoF) printf("[SERVE] Elevator %d: Put people\n", Ele->id);
                printf("-----------------------------------------\n");

                /* Refresh data of some body get out the car and wake up IO */
                pthread_mutex_lock(&shm_mut_cond->mutex);
                pthread_mutex_unlock(&shm_mut_cond->mutex);
                pthread_cond_signal(&shm_mut_cond->cond);

                request_time.tv_sec = 6;
                TimeFlag = nanosleep(&request_time, &remain_time); 
                if (TimeFlag == -1)
                    nanosleep(&remain_time, &remain_time);

                /* If need stop */
                if(Ele->location == Ele->destination)
                {                   
                    printf("[STATE] Elevator %d: Stop at %d\n", ID, Ele->location);
                    printf("-----------------------------------------\n");
                    Ele->state = STOP;
                    Ele->direction = NONE;
                }   
                else
                {
                    Ele->state = MOVE;
                }       
            }
        }

        //Update the elevator's state
        if(Ele->state == MOVE)
        {
            char direction_string[5];
            if(Ele->direction==UP) strcpy(direction_string, "Up");
            else if(Ele->direction==DOWN) strcpy(direction_string, "Down");
            else if(Ele->direction==NONE) strcpy(direction_string, "None");
            printf("[STATE] Elevator %d: Current Floor is %d, Going %s\n", ID, Ele->location, direction_string);
            if(Ele->direction == UP) Ele->location += 1; //elevator go up
            else if(Ele->direction == DOWN) Ele->location -= 1; //elevator go down
            request_time.tv_sec = 3;
            TimeFlag = nanosleep(&request_time, &remain_time); 
            if (TimeFlag == -1)
                nanosleep(&remain_time, &remain_time); 
            if(Ele->location == Ele->destination)
            {
                Ele->direction = NONE;
            }  
        }
    } // end of while
}

//calculate the moving cost of the passenger to get to the destination
//destination: Passenger's destination
//direction: Passenger's direction
int TravelCost(PassengerType type, unsigned short destination, Direction direction, ElevatorState ele)
{
    
    int cost;
    int ele_distance_cost, passenger_distance_cost, type_factor;

    ele_distance_cost = ele.destination - ele.location;
    passenger_distance_cost = destination - ele.location;
    if(type==NORMAL) type_factor = 2;
    else if(type==DISABLE) type_factor = 1;

    //the elvator is at the desired location
    if(passenger_distance_cost == 0) return 0;

    //elevator up
    else if(ele.direction == UP)
    {
        //passenger and elevator same direction
        if(direction == UP && passenger_distance_cost > 0) return passenger_distance_cost * type_factor;
        else if(direction != UP && passenger_distance_cost > 0) 
            return (abs(destination - ele.destination) + ele_distance_cost) * type_factor; 
        else
        {
            passenger_distance_cost = abs(destination - ele.destination) + ele_distance_cost;
            return passenger_distance_cost * type_factor;
        }
    }
    else if(ele.direction == DOWN)
    {
        if(direction == DOWN && passenger_distance_cost < 0) return abs(passenger_distance_cost);
        else if(direction != DOWN && passenger_distance_cost < 0) 
        {
            return (abs(destination - ele.destination) + abs(ele_distance_cost)) * type_factor;
        }
        else
        {
            passenger_distance_cost = abs(destination - ele.destination) + abs(ele_distance_cost);
            return passenger_distance_cost * type_factor;
        }
    }
    else return abs(passenger_distance_cost) * type_factor;
}

// Took two elevator in concern
bool AllocLongerDest(Requirement* peo, ElevatorState **ele)
{
    bool flag1, flag2;
    int cost_1, cost_2, cost;

    //check the pasenger's current location is further than both elevator's current floor
    //elevator 1, longer than passenger's starting position
    flag1 = isFurther(peo->start, *ele[0]);
    //elevator 2, longer than passenger's starting position
    flag2 = isFurther(peo->start, *ele[1]);

    // calculate two elevators current desination to passengers
    cost_1 = TravelCost(peo->type, peo->start, peo->direction, *ele[0]);
    cost_2 = TravelCost(peo->type, peo->start, peo->direction, *ele[1]);
    cost = -1;

    if(flag1 && flag2)
    {
        if(cost_1 <= cost_2 && ele[0]->state != STOP && ele[0]->direction != NONE)
        {
            ele[0]->destination = peo->start; 
            cost = cost_1;
            printf("[INFO] Elevator 1 allocte longer destination to %d\n", peo->start);
            printf("-----------------------------------------\n");
        } 
        else if(cost_1 > cost_2 && ele[1]->state != STOP && ele[1]->direction != NONE)
        {
            ele[1]->destination = peo->start; 
            cost = cost_2;
            printf("[INFO] Elevator 2 allocte longer destination to %d\n", peo->start);
            printf("-----------------------------------------\n");
        } 
    }
    else if(flag1 == true && flag2 == false)
    {
        if(cost_1 < cost_2 && ele[0]->state != STOP && ele[0]->direction != NONE)
        {
            ele[0]->destination = peo->start; 
            cost = cost_1;
            printf("[INFO] Elevator 1 allocte longer destination to %d\n", peo->start);
            printf("-----------------------------------------\n");
        } 
    }
    else if(flag1 == false && flag2 == true)
    {
        if(cost_2 < cost_1 && ele[1]->state != STOP && ele[1]->direction != NONE)
        {
            ele[1]->destination = peo->start;  
            cost = cost_2;
            printf("[INFO] Elevator 2 allocte longer destination to %d\n", peo->start);
            printf("-----------------------------------------\n");
        } 
    }

    if(cost == 0) return true;
    else return false;
}


//choose the stop floor of the elavtor, take two elevator in concern
unsigned short ForSTOPchoseDest(ElevatorState *main_ele, ElevatorState *another_ele)
{
    //maxCost: Current Elevator's max cost
    //currentCost: Current Passenger's TravelCost
    //otherCost: Other Elevator's cost
    int maxCost = 0, currentCost = 0, otherCost;
    unsigned short destination = 0;
    int index, one_count, one_index[32];

    find_all_bitmap_1_index(shm_bitmap->one, &one_count, one_index); // find all the floor that contains request
    for(int i = 0; i < one_count; i++)
    {
        index = one_index[i];
        if(p_Requirement[index]->state == WAIT)
        {
            //calculate travel cost
            currentCost = TravelCost(p_Requirement[index]->type, p_Requirement[index]->start, p_Requirement[index]->direction, *main_ele);
            
            /* For the request of same floor */
            // If the passenger's floor and direction is different with other elevator
            // let the main elevator to go there
            if(currentCost == 0 && 
                (another_ele->destination != p_Requirement[index]->start || another_ele->direction != p_Requirement[index]->direction))
            {
                main_ele->direction = p_Requirement[index]->direction;
                return p_Requirement[index]->start;
            }
            //if cuurentCost > maxCost and another elevator won't stop at the same floor
            else if(currentCost > maxCost && another_ele->destination != p_Requirement[index]->start)
            {
                otherCost = TravelCost(p_Requirement[index]->type, p_Requirement[index]->start, p_Requirement[index]->direction, *another_ele);
                //If other elevator's cost is bigger or cost is moving but not same
                if((otherCost > currentCost) ||
                    (otherCost == currentCost && another_ele->state != MOVE))
                {
                    destination = p_Requirement[index]->start;
                    maxCost = currentCost;
                }
            }
        }
    }
    return destination;
}
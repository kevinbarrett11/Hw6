#include "hw6.h"
#include <stdio.h>
#include<pthread.h>
#include <time.h>
#include <stdlib.h>


struct Elevator {
    pthread_mutex_t lock;  //a lock so nothing else takes control of that elevator
    pthread_barrier_t barrierOpen, barrierClosed;   //stores barriers for each elevator so that the global elevator data isnt overwritten

    int current_floor;
    int direction; //direction elevator is going aka up or down
    int to_floor;  //destination of where elevator is going to pick up or drop off
    int occupancy;  //how many passengers are in elevator, default being 0
    enum { ELEVATOR_ARRIVED, ELEVATOR_OPEN, ELEVATOR_CLOSED } state;
}elevators[ELEVATORS];


//initilizes all the variables when main calls scheduler init for all 4 elevators which is assigned via main.c or test1 ect
void scheduler_init() {
    for (int i = 0; i < ELEVATORS; i++) {  //elevators is assigned via main.c or test which changes between 4 and 1
        elevators[i].current_floor =  random() % ELEVATORS; //sets all elevators floors to random so that it can prossess passnegers all throughout building faster
        elevators[i].direction = -1;  //default direction to -1
        elevators[i].occupancy = 0;  //default elevator empty
        elevators[i].state = ELEVATOR_ARRIVED;  //default state to arrived
        pthread_mutex_init(&elevators[i].lock, 0);  //mutex lock to prevent critical data from being changed
        pthread_barrier_init(&elevators[i].barrierOpen, NULL, 2);  //barrier to pass control to passanger or elevator
        pthread_barrier_init(&elevators[i].barrierClosed, NULL, 2);  //closes barrier
        elevators[i].to_floor = 0;  //sets default to floor to zero because they arnt going anywhere yet until passenger request
    }
}


void passenger_request(int passenger, int from_floor, int to_floor,
                       void(*enter)(int, int),
                       void(*exit)(int, int))
{

    //attempt to get closets elevator to passenger to run
//    int elevatorDistance[4] = {0 , 0 ,0, 0};
//
//    for(int i = 0; i < 3; i++){
//        if( elevators[i].occupancy== 1){
//            elevatorDistance[i] = 99;
//        } else {
//            if (elevators[i].current_floor > from_floor){
//              elevatorDistance[i] =  elevators[i].current_floor - from_floor;
//            } else
//            elevatorDistance[i] = from_floor - elevators[i].current_floor;
//        }
//    }
//
//
//    int closest = 0;
//    for(int j = 1; j < 3; j++){
//        if(elevatorDistance[j] < elevatorDistance[closest]){
//            closest = j;
//        }
//    }
//    for (int l = 0; l < 3; l++){
//        elevatorDistance[l] = 0;
//    }
//    elevator = closest;

    int elevator = random() % ELEVATORS;

    //if elevator full then choose different elevator
    while (elevators[elevator].occupancy == 1)
    {
        elevator = random() % ELEVATORS;
    }

    pthread_mutex_lock(&elevators[elevator].lock); //locks critical data btween this and unlock

    elevators[elevator].to_floor = from_floor; //tells the elvator what floor it has to go to pick someone up

    //passes control to passneger to allow them to get on
    pthread_barrier_wait(&elevators[elevator].barrierOpen);
    if (elevators[elevator].occupancy == 0)
    {
        enter(passenger, elevator);
        elevators[elevator].occupancy++;  //increase
    }
    elevators[elevator].to_floor = to_floor;  //changes value to the floor of where passenger is going
    pthread_barrier_wait(&elevators[elevator].barrierClosed);


    //passes control to passenger then lets passenger to exit
    pthread_barrier_wait(&elevators[elevator].barrierOpen);
    if (elevators[elevator].occupancy == 1)
    {
        exit(passenger, elevator);
        elevators[elevator].occupancy--;
    }
    elevators[elevator].to_floor = -1;
    pthread_barrier_wait(&elevators[elevator].barrierClosed);  //passes control back to elevator

    pthread_mutex_unlock(&elevators[elevator].lock);  //unlocks critical data
}

void elevator_ready(int elevator, int at_floor,
                    void(*move_direction)(int, int),
                    void(*door_open)(int), void(*door_close)(int)) {

    if (elevators[elevator].to_floor == at_floor && elevators[elevator].state == ELEVATOR_ARRIVED) {
        door_open(elevator);  //opens the door for passenger
        elevators[elevator].state = ELEVATOR_OPEN;  //sets elevator state to open
        pthread_barrier_wait(&elevators[elevator].barrierOpen); //gives passenger time to get on
        pthread_barrier_wait(&elevators[elevator].barrierClosed);  //returns control to elevator
    }
    else if (elevators[elevator].state == ELEVATOR_OPEN) {
        door_close(elevator);  //if the elevator is open it closes it assuimging that passenger had time to get on
        elevators[elevator].state = ELEVATOR_CLOSED;  //changes elevator state to closed
    }
    else {
        if (at_floor == 0 || at_floor == FLOORS - 1)
            elevators[elevator].direction *= -1;
        if (elevators[elevator].to_floor != -1) {
            move_direction(elevator, elevators[elevator].direction);
            elevators[elevator].current_floor = at_floor + elevators[elevator].direction;
        }
        elevators[elevator].state = ELEVATOR_ARRIVED;

    }
}
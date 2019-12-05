#include "hw6.h"
#include <stdio.h>
#include<pthread.h>



pthread_mutex_t lock;
pthread_barrier_t barr;

int current_floor;
int direction;
int occupancy;
enum {ELEVATOR_ARRIVED=1, ELEVATOR_OPEN=2, ELEVATOR_CLOSED=3} state;

void scheduler_init() {
    current_floor=0;
    direction=-1;
    occupancy=0;
    state=ELEVATOR_ARRIVED;
    pthread_mutex_init(&lock,0);
    pthread_barrier_init(&barr,NULL,2);
}


void passenger_request(int passenger, int from_floor, int to_floor,
                       void (*enter)(int, int),
                       void(*exit)(int, int))
{
    // wait for the elevator to arrive at our origin floor, then get in
pthread_barrier_wait(&barr);
    pthread_mutex_lock(&lock);

        if(current_floor == from_floor && state == ELEVATOR_OPEN && occupancy==0) {
            enter(passenger, 0);  //change this to look for next elevator
            occupancy++;
        }

        pthread_mutex_unlock(&lock);


    // wait for the elevator at our destination floor, then get out
pthread_barrier_wait(&barr);
    pthread_mutex_lock(&lock);

        if(current_floor == to_floor && state == ELEVATOR_OPEN) {
            exit(passenger, 0);
            occupancy--;
        }

        pthread_mutex_unlock(&lock);
    
}

void elevator_ready(int elevator, int at_floor,
                    void(*move_direction)(int, int),
                    void(*door_open)(int), void(*door_close)(int)) {
    if(elevator!=0) return;



    pthread_mutex_lock(&lock);
    if(state == ELEVATOR_ARRIVED) {
        door_open(elevator);
        state=ELEVATOR_OPEN;
        pthread_barrier_wait(&barr);
    }
    else if(state == ELEVATOR_OPEN) {
        door_close(elevator);
        state=ELEVATOR_CLOSED;
    }
    else {
        if(at_floor==0 || at_floor==FLOORS-1)
            direction*=-1;
        move_direction(elevator,direction);
        current_floor=at_floor+direction;
        state=ELEVATOR_ARRIVED;
    }
    pthread_mutex_unlock(&lock);
}

static struct Passenger {
    int id;
    int from_floor;
    int to_floor;
    int in_elevator;
    enum {WAITING, ENTERED, EXITED} state;
} passengers[PASSENGERS];

static struct Elevator {
    int seqno, last_action_seqno; // these two are used to enforce control rules
    int floor;
    int open;
    int passengers;
    int trips;
    int passengerNum;
} elevators[ELEVATORS];

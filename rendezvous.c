#define SIMULATOR


#ifndef SIMULATOR
   #include <kilolib.h>
    #include <avr/io.h>  // for microcontroller register defs
    #include "sync.h"
    USERDATA myData;
    USERDATA *mydata = &myData;
#else
    #include <math.h>
    #include <kilombo.h>
    #include <stdio.h> // for printf
    #include "rendezvous.h"
    REGISTER_USERDATA(USERDATA)
#endif



/* --------------------------------------------------------------------------------------------------- */
/* -------------------------------------------MESSAGE PASSING----------------------------------------- */
/* --------------------------------------------------------------------------------------------------- */

message_t *message_tx()
{
    
    if (mydata->tail != mydata->head)   // Queue is not empty
    {
        return &mydata->message[mydata->head];
    }
    return &mydata->nullmessage;
}
 
void message_tx_success() {
    if (mydata->tail != mydata->head) {  // Queue is not empty
        if (mydata->copies == 2)
        {
            mydata->head++;
            mydata->copies = 0;
            mydata->head = mydata->head % QUEUE;
        }   
        else
        {
            mydata->copies++;
        }
    }
}

char isQueueFull()
{
    return (mydata->tail +1) % QUEUE == mydata->head;
}


char in_interval(uint8_t distance)
{
    if (distance <= 100)
        return 1;
    return 0;
}

uint8_t exists_nearest_neighbor(uint8_t id)
{
    uint8_t i;
    for (i=0; i<mydata->num_neighbors; i++)
    {
        if (mydata->nearest_neighbors[i].id == id)
            return i;
    }
    return i;
}

void recv_sharing(uint8_t *payload, uint8_t distance)
{
    //if (payload[ID] == mydata->my_id  || payload[ID] == 0 || !in_interval(distance) ) return;
    if (payload[ID] == mydata->my_id) return;
    
    uint8_t i = exists_nearest_neighbor(payload[ID]);
    if (i >= mydata->num_neighbors) // The id has never received
    {
        if (mydata->num_neighbors < MAX_NUM_NEIGHBORS)
        {
            i = mydata->num_neighbors;
            mydata->num_neighbors++;
            mydata->nearest_neighbors[i].num = 0;
            
        }
    }

    mydata->nearest_neighbors[i].id = payload[ID];
    mydata->nearest_neighbors[i].distance = distance;
    mydata->nearest_neighbors[i].clock = payload[CLOCK];
    mydata->nearest_neighbors[i].leader_id = payload[LEADER_ID];
    mydata->nearest_neighbors[i].timestamp = kilo_ticks;
}


void message_rx(message_t *m, distance_measurement_t *d)
{
    uint8_t dist = estimate_distance(d);
    
    if (m->type == NORMAL && m->data[MSG] !=NULL_MSG)
    {
        recv_sharing(m->data, dist);
        switch (m->data[MSG])
        {
            /*case JOIN:
                recv_joining(m->data);
                break;
            case MOVE:
                recv_move(m->data);
                break;
            case ELECT:
                recv_elect(m->data);
                break;*/
        }
    }
}


char enqueue_message(uint8_t m)
{
    if (!isQueueFull())
    {
        mydata->message[mydata->tail].data[MSG] = m;
        mydata->message[mydata->tail].data[ID] = mydata->my_id;
        mydata->message[mydata->tail].data[CLOCK] = mydata->clock;
        mydata->message[mydata->tail].data[LEADER_ID] = mydata->leader_id;

        mydata->message[mydata->tail].type = NORMAL;
        mydata->message[mydata->tail].crc = message_crc(&mydata->message[mydata->tail]);
        mydata->tail++;
        mydata->tail = mydata->tail % QUEUE;
        return 1;
    }
    return 0;
}

void send_sharing()
{
    if (mydata->now >= mydata->nextShareSending  && !isQueueFull())
    {
        enqueue_message(SHARE);
        mydata->nextShareSending = mydata->now + SHARING_TIME;
    }
}

void purgeNeighbors(void)
{
  int8_t i;

  for (i = mydata->num_neighbors-1; i >= 0; i--) 
    if (kilo_ticks - mydata->nearest_neighbors[i].timestamp  > 64) //32 ticks = 1 s
    {
	    mydata->nearest_neighbors[i] = mydata->nearest_neighbors[mydata->num_neighbors-1];
	    mydata->num_neighbors--;
        mydata->leader_id = mydata->my_id;
    }
}

/* --------------------------------------------------------------------------------------------------- */
/* --------------------------------------SYNC CLOCK AND LEADER---------------------------------------- */
/* --------------------------------------------------------------------------------------------------- */

uint8_t colors[] = {
  RGB(1,1,1),  //0 - grey
  RGB(2,0,0),  //1 - red
  RGB(2,1,0),  //2 - orange
  RGB(2,2,0),  //3 - yellow
  RGB(1,2,0),  //4 - yellowish green
  RGB(0,2,0),  //5 - green
  RGB(0,1,1),  //6 - cyan
  RGB(1,1,2),  //7 - violet
  RGB(0,0,2),  //8 - blue
  RGB(1,0,1),  //9 - purple
  RGB(3,3,3)   //10  - bright white (leader color)
};

char isLeader()
{
    if (mydata->leader_id == mydata->my_id && mydata->num_neighbors > 0)
        return 1;
    return 0;
}

/**
 * Updates the clock and the leader_id based off neighbor data.
 **/
void update_data()
{
    uint8_t i;

    char isSafe = 1;

    for (i = 0; i < mydata->num_neighbors; i++)
    {
        if (mydata->nearest_neighbors[i].clock != mydata->clock)
            isSafe = 0;
        if (mydata->nearest_neighbors[i].clock != UINT8_MAX && mydata->nearest_neighbors[i].clock > mydata->clock)
            mydata->clock = mydata->nearest_neighbors[i].clock;
        if (mydata->nearest_neighbors[i].leader_id > mydata->leader_id)
            mydata->leader_id = mydata->nearest_neighbors[i].leader_id;
    }

    if (isSafe)
    {
        if (mydata->clock == UINT8_MAX)
            mydata->clock = 0;
        else
            mydata->clock++;
    }
}

/* --------------------------------------------------------------------------------------------------- */
/* ----------------------------------------------MOVEMENT--------------------------------------------- */
/* --------------------------------------------------------------------------------------------------- */

void set_motion(motion_t new_motion)
{
    if (mydata->motion_state != new_motion)
    {
        mydata->motion_state = new_motion;

        switch(mydata->motion_state) {
            case STOP:
                set_motors(0,0);
                break;
            case FORWARD:
                spinup_motors();
                #ifndef SIMULATOR
                    set_motors(kilo_straight_left, kilo_straight_right);
                #else
                    set_motors(kilo_turn_left, kilo_turn_right);
                #endif
                break;
            case LEFT:
                spinup_motors();
                set_motors(kilo_turn_left, 0); 
                break;
            case RIGHT:
                spinup_motors();
                set_motors(0, kilo_turn_right); 
                break;
        }
    }
}

void acquire_target()
{
    // Do nothing if existing target is still in range
    uint8_t i;
    for(i = 0; i < mydata->num_neighbors; i++)
    {
        if (mydata->nearest_neighbors[i].id == mydata->target_id)
        {
            mydata->target_index = i; // Set the index again just to be safe
            return;
        }
    }

    // Acquire a new target as the target doesn't exist anymore or if target_id is equal to my_id
    uint8_t closest_distance = UINT8_MAX;
    uint8_t closest_id = mydata->my_id;
    uint8_t closest_index = 0;
    for (i = 0; i < mydata->num_neighbors; i++)
    {
        if (mydata->nearest_neighbors[i].id == mydata->leader_id) // If leader is a neighbor, it's the target
        {
            mydata->target_id = mydata->nearest_neighbors[i].id;
            mydata->target_index = i;
            mydata->prev_distance = mydata->nearest_neighbors[i].distance;
            return;
        }

        if (mydata->nearest_neighbors[i].distance < closest_distance)
        {
            closest_distance = mydata->nearest_neighbors[i].distance;
            closest_id = mydata->nearest_neighbors[i].id;
            closest_index = i;
        }
    }

    if (closest_id != mydata->my_id) // If no leader found, then set target to id of closest neighbor
    {
        mydata->target_id = closest_id;
        mydata->target_index = closest_index;
        mydata->prev_distance = closest_distance;
    }
}

void set_motion_random()
{
    if (rand_soft() & 1)
        set_motion(RIGHT);
    else
        set_motion(LEFT);
}

/**
 * Moves towards leader if near. Otherwise, move towards furthest neighbor.
 **/
void move_closer_to_neighbor()
{
    if (kilo_ticks > (mydata->last_state_update + 64))
    {
        mydata->last_state_update = kilo_ticks;

        if (isLeader()) // Don't move if leader
        {
            set_motion(STOP);
            return;        
        }
        else if (mydata->num_neighbors == 0) // Default turn right if no neighbors
        {
            set_motion(RIGHT);
            return;
        }
        else // Move toward target
        {
            acquire_target(); // Attempts to acquire target

            if (mydata->target_id != mydata->my_id && mydata->target_index < mydata->num_neighbors)
            {
                if (mydata->nearest_neighbors[mydata->target_index].distance > 50)
                {
                    if (mydata->prev_distance < mydata->nearest_neighbors[mydata->target_index].distance)
                    {
                        if (mydata->motion_state == LEFT)
                            set_motion(RIGHT);
                        else if (mydata->motion_state == RIGHT)
                            set_motion(LEFT);
                        else if (mydata->motion_state == STOP)
                            set_motion_random();
                    }
                    else
                        set_motion_random();

                }
                else
                {
                    mydata->target_index = mydata->my_id; // Reset target
                    set_motion(STOP);
                }
            }

            mydata->prev_distance = mydata->nearest_neighbors[mydata->target_index].distance;
        }
    }
}

/* --------------------------------------------------------------------------------------------------- */
/* ---------------------------------------------MAIN LOOP--------------------------------------------- */
/* --------------------------------------------------------------------------------------------------- */

void loop()
{
    delay(30);

    purgeNeighbors();
    update_data();
    send_sharing();
    
    if (!isLeader())
        set_color(colors[mydata->clock%10]);
    else
        set_color(colors[10]);

    move_closer_to_neighbor();

    mydata->now++;
}

void setup() {
    rand_seed(rand_hard());
    mydata->my_id = rand_soft();
    
    mydata->num_neighbors = 0;
    mydata->message_sent = 0,
    mydata->now = 0,
    mydata->nextShareSending = SHARING_TIME,
    mydata->time_active = 0;
    mydata->red = 0,
    mydata->green = 0,
    mydata->blue = 0,
    mydata->send_token = 0;

    mydata->nullmessage.data[MSG] = NULL_MSG;
    mydata->nullmessage.crc = message_crc(&mydata->nullmessage);
    
    mydata->token = rand_soft() < 128  ? 1 : 0;
    mydata->blue = mydata->token;
    mydata->head = 0;
    mydata->tail = 0;
    mydata->copies = 0;

    mydata->message_sent = 1;

    mydata->clock = 0;
    mydata->leader_id = mydata->my_id;

    mydata->motion_state = STOP;
    mydata->last_state_update = kilo_ticks;
    mydata->target_id = mydata->my_id;
    mydata->target_index = 0;
    mydata->prev_distance = 0;
}

#ifdef SIMULATOR
/* provide a text string for the simulator status bar about this bot */
static char botinfo_buffer[10000];
char *botinfo(void)
{
    char *p = botinfo_buffer;
    p += sprintf (p, "ID: %d   LeaderID:%d  TargetID:%d\n", mydata->my_id, mydata->leader_id, mydata->target_id);
    p += sprintf (p, "Clock: %d\n", mydata->clock);
    p += sprintf (p, "Num Neighbors: %d\n", mydata->num_neighbors);
    
    return botinfo_buffer;
}
#endif

int main() {
    kilo_init();
    kilo_message_tx = message_tx;
    kilo_message_tx_success = message_tx_success;
    kilo_message_rx = message_rx;
    kilo_start(setup, loop);

#ifdef SIMULATOR
    SET_CALLBACK(botinfo, botinfo);
    SET_CALLBACK(reset, setup);
#endif

    return 0;
}

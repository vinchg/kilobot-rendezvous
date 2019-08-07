

#define MAX_NUM_NEIGHBORS 10
#define SHARING_TIME 10
#define TOKEN_TIME 103


//PAYLOAD
#define MSG 0
#define ID 1
#define CLOCK 2
#define LEADER_ID 3
#define RESET 4

#define ACTIVE 0

#define QUEUE 4


#ifndef M_PI
#define M_PI 3.141592653589793238462643383279502884197169399375105820974944
#endif

typedef enum { NULL_MSG,
    SHARE,
    JOIN,
    LEAVE,
    MOVE
} message_type;  // MESSAGES

typedef enum {
    STOP,
    FORWARD,
    LEFT,
    RIGHT
} motion_t;

typedef struct{
    uint8_t id;
    uint8_t distance;
    uint8_t num;
    uint8_t num_cooperative;
    
    uint8_t clock;
    uint8_t leader_id;
    uint32_t timestamp;

} nearest_neighbor_t;

typedef struct
{
    uint8_t my_id;
    message_t message[QUEUE];
    message_t nullmessage;
    
    uint8_t num_neighbors;
    uint8_t message_sent;
    uint16_t now;
    uint16_t nextShareSending;
    uint8_t time_active;
    nearest_neighbor_t nearest_neighbors[MAX_NUM_NEIGHBORS];
    char send_token;
    uint8_t green;
    uint8_t red;
    uint8_t blue;
    int8_t token;
    int8_t head, tail;
    int8_t copies;

    uint8_t clock;
    uint8_t leader_id;

    uint8_t motion_state;
    uint32_t last_state_update;
    uint8_t target_id;
    uint8_t target_index;
    uint8_t prev_distance;
} USERDATA;
#ifndef MESSAGES_H
#define MESSAGES_H


#define MAX_PROVINCES 10




#define MSG_REGISTER_PROVINCE        100


#define MSG_PROVINCE_REPORT          101


#define MSG_REQUEST_IDLE_DISTRIBUTOR 102


#define MSG_IDLE_DISTRIBUTOR         103


#define MSG_NEW_DISTRIBUTOR          104


#define MSG_TERMINATE                105


#define MSG_INIT_DISTRIBUTOR         110
#define MSG_DISTRIBUTOR_READY        111
#define MSG_DISTRIBUTOR_STATUS       112



typedef struct {
    int tid;                  
    int remaining_requests;   
    int idle_distributors;   
} ProvinceStatus;

#endif

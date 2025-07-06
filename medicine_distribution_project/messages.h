#ifndef MESSAGES_H
#define MESSAGES_H

// Maximum number of provinces
#define MAX_PROVINCES 10

// ==== Message Tags for PVM Communication ====

// Messages for Master
#define MSG_REGISTER_PROVINCE        100
#define MSG_PROVINCE_REPORT          101
#define MSG_REQUEST_IDLE_DISTRIBUTOR 102
#define MSG_IDLE_DISTRIBUTOR         103
#define MSG_NEW_DISTRIBUTOR          104
#define MSG_TERMINATE                105

// Messages for Distributors
#define MSG_INIT_DISTRIBUTOR         110
#define MSG_DISTRIBUTOR_READY        111
#define MSG_DISTRIBUTOR_STATUS       112
#define MSG_ASSIGN_TASK              113
#define MSG_TASK_COMPLETE            114

// ==== Data Structures ====

typedef struct {
    int tid;                  // Task ID
    int remaining_requests;   // Number of remaining requests
    int idle_distributors;    // Number of idle distributors
} ProvinceStatus;

typedef struct {
    int distributor_id;
    int province_id;
    int status;  // 0 = idle, 1 = busy
} DistributorStatus;

typedef struct {
    int task_id;
    int destination_type;  // 0 = pharmacy, 1 = clinic, 2 = hospital
    int province_id;
} DistributionTask;

#endif // MESSAGES_H
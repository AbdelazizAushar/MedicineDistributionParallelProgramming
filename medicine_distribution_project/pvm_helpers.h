#ifndef PVM_HELPERS_H
#define PVM_HELPERS_H

#include <pvm3.h>
#include "messages.h" 

// Initialization
int pvm_init(const char* task_name);

// Integer messaging
int send_int(int dest_tid, int tag, int value);
int recv_int(int* sender_tid, int tag);

int send_int_message(int dest_tid, int tag, int* data, int count);
int recv_int_message(int src_tid, int tag, int* buffer, int count);

// Signal messaging
int send_signal(int dest_tid, int tag);
int recv_signal(int src_tid, int tag);

// Broadcast
int broadcast_int(int* tids, int count, int tag, int value);

// Role detection
int determine_role(int argc, char* argv[]);

#endif

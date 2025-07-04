#ifndef PVM_HELPERS_H
#define PVM_HELPERS_H

#include <pvm3.h>
#include "message.h" 


int pvm_init(const char* task_name);


int send_int_message(int dest_tid, int tag, int* data, int count);


int recv_int_message(int src_tid, int tag, int* buffer, int count);


int send_signal(int dest_tid, int tag);

int recv_signal(int src_tid, int tag);

int broadcast_int(int* tids, int count, int tag, int value);

int determine_role(int argc, char* argv[]);

#endif 
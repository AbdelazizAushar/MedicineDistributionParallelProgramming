#include <stdio.h>
#include "pvm_helpers.h"

// Initialize the PVM task and register its TID
int pvm_init(const char* task_name) {
    int tid = pvm_mytid();
    if (tid < 0) {
        fprintf(stderr, "? Failed to register with PVM\n");
        return -1;
    }
    printf("? Task %s registered with tid=%d\n", task_name, tid);
    return tid;
}

// Send an integer array
int send_int_message(int dest_tid, int tag, int* data, int count) {
    pvm_initsend(PvmDataDefault);
    pvm_pkint(data, count, 1);
    int err = pvm_send(dest_tid, tag);
    if (err < 0) {
        fprintf(stderr, "? Failed to send message with tag=%d to tid=%d\n", tag, dest_tid);
    }
    return err;
}

// Receive an integer array
int recv_int_message(int src_tid, int tag, int* buffer, int count) {
    int sender_tid = pvm_recv(src_tid, tag);
    if (sender_tid < 0) {
        fprintf(stderr, "? Failed to receive message with tag=%d\n", tag);
        return -1;
    }
    int n = pvm_upkint(buffer, count, 1);
    if (n != count) {
        fprintf(stderr, "?? Number of integers received (%d) does not match expected (%d)\n", n, count);
    }
    return sender_tid;
}

// Send a signal with no data
int send_signal(int dest_tid, int tag) {
    pvm_initsend(PvmDataDefault);
    int err = pvm_send(dest_tid, tag);
    if (err < 0) {
        fprintf(stderr, "? Failed to send signal with tag=%d to tid=%d\n", tag, dest_tid);
    }
    return err;
}

// Receive a signal with no data
int recv_signal(int src_tid, int tag) {
    int sender_tid = pvm_recv(src_tid, tag);
    if (sender_tid < 0) {
        fprintf(stderr, "? Failed to receive signal with tag=%d\n", tag);
        return -1;
    }
    return sender_tid;
}

// Broadcast an integer value to a list of tids
int broadcast_int(int* tids, int count, int tag, int value) {
    int i;
	for (i = 0; i < count; i++) {
        int err = send_int_message(tids[i], tag, &value, 1);
        if (err < 0) {
            fprintf(stderr, "?? Failed to broadcast to tid=%d\n", tids[i]);
            return err;
        }
    }
    return 0;
}

// Example role detection based on argc/argv (you can adjust logic here)
int determine_role(int argc, char* argv[]) {
    if (argc < 2) {
        return 0; // Default to Master
    }
    if (strcmp(argv[1], "master") == 0) return 0;
    if (strcmp(argv[1], "province") == 0) return 1;
    if (strcmp(argv[1], "distributor") == 0) return 2;
    return -1; // Unknown role
}
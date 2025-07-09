#include <stdio.h>
#include <string.h>
#include <pvm3.h>
#include "pvm_helpers.h"

int pvm_init(const char* task_name) {
    int tid;

    tid = pvm_mytid();
    if (tid < 0) {
        fprintf(stderr, "[PVM] Failed to register with PVM\n");
        return -1;
    }

    printf("[PVM] Task '%s' registered with tid=%d\n", task_name, tid);
    return tid;
}


int send_int(int dest_tid, int tag, int value) {
    int err;

    pvm_initsend(PvmDataDefault);
    pvm_pkint(&value, 1, 1);
    err = pvm_send(dest_tid, tag);

    if (err < 0) {
        fprintf(stderr, "[PVM] Failed to send int (tag=%d) to tid=%d\n", tag, dest_tid);
    }

    return err;
}

int recv_int(int* sender_tid, int tag) {
    int cc, tid, value, result;

    cc = pvm_recv(-1, tag);
    if (cc < 0) {
        fprintf(stderr, "[PVM] Failed to receive int (tag=%d)\n", tag);
        return -1;
    }
	pvm_bufinfo(cc, (int*)0, (int*)0, &tid);
    result = pvm_upkint(&value, 1, 1);

    if (result < 0) {
        return -1;
    }

    if (sender_tid != 0) {
        *sender_tid = tid;
    }

    return value;
}

int send_int_message(int dest_tid, int tag, int* data, int count) {
    int err;

    pvm_initsend(PvmDataDefault);
    pvm_pkint(data, count, 1);
    err = pvm_send(dest_tid, tag);

    if (err < 0) {
        fprintf(stderr, "[PVM] Failed to send int array (tag=%d) to tid=%d\n", tag, dest_tid);
    }

    return err;
}


int recv_int_message(int src_tid, int tag, int* buffer, int count) {
    int sender_tid, unpacked;

    sender_tid = pvm_recv(src_tid, tag);
    if (sender_tid < 0) {
        fprintf(stderr, "[PVM] Failed to receive int array (tag=%d)\n", tag);
        return -1;
    }

    unpacked = pvm_upkint(buffer, count, 1);
    if (unpacked != count) {
        fprintf(stderr, "[PVM] Mismatch in received int array size (expected %d, got %d)\n", count, unpacked);
    }

    return sender_tid;
}


int send_signal(int dest_tid, int tag) {
    int err;

    pvm_initsend(PvmDataDefault);
    err = pvm_send(dest_tid, tag);

    if (err < 0) {
        fprintf(stderr, "[PVM] Failed to send signal (tag=%d) to tid=%d\n", tag, dest_tid);
    }

    return err;
}


int recv_signal(int src_tid, int tag) {
    int sender_tid;

    sender_tid = pvm_recv(src_tid, tag);
    if (sender_tid < 0) {
        fprintf(stderr, "[PVM] Failed to receive signal (tag=%d)\n", tag);
        return -1;
    }

    return sender_tid;
}

int broadcast_int(int* tids, int count, int tag, int value) {
    int i, err;

    for (i = 0; i < count; i++) {
        err = send_int(tids[i], tag, value);
        if (err < 0) {
            fprintf(stderr, "[PVM] Failed to broadcast to tid=%d\n", tids[i]);
            return err;
        }
    }

    return 0;
}


int determine_role(int argc, char* argv[]) {
    if (argc < 2) {
        return 0; /* Default to master */
    }

    if (strcmp(argv[1], "master") == 0) {
        return 0;
    } else if (strcmp(argv[1], "province") == 0) {
        return 1;
    } else if (strcmp(argv[1], "distributor") == 0) {
        return 2;
    }

    return -1;
}

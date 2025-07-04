// province.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "province.h"
#include "messages.h"
#include "pvm_helpers.h"

// Province internal state
static int province_id = -1;
static int num_distributors = 0;
static int* distributor_tids = NULL;
static int remaining_requests = 0;
static int idle_distributors = 0;

// Initialize the province coordinator communicator and state
int province_init(int p_id, int distributors) {
    province_id = p_id;
    num_distributors = distributors;

    distributor_tids = (int*) malloc(sizeof(int) * num_distributors);
    if (!distributor_tids) {
        fprintf(stderr, "? Failed to allocate memory for distributors\n");
        return -1;
    }

    // Initialize all distributors as idle for simplicity
    for (int i = 0; i < num_distributors; i++) {
        distributor_tids[i] = -1;  // Will be set when distributors register or assigned
    }

    remaining_requests = 0;  // will be updated when receiving from master
    idle_distributors = num_distributors;

    printf("? Province coordinator %d initialized with %d distributors\n", province_id, num_distributors);
    return 0;
}

// Send a status report (remaining requests and idle distributors) to the master
void send_report_to_master() {
    ProvinceStatus status;
    status.tid = pvm_mytid();
    status.remaining_requests = remaining_requests;
    status.idle_distributors = idle_distributors;

    int master_tid = -1;  // Assuming we get master tid from somewhere or PVM spawn parent
    master_tid = pvm_parent();

    if (master_tid < 0) {
        fprintf(stderr, "? Cannot find master to send report\n");
        return;
    }

    pvm_initsend(PvmDataDefault);
    pvm_pkint(&province_id, 1, 1);
    pvm_pkint(&status.remaining_requests, 1, 1);
    pvm_pkint(&status.idle_distributors, 1, 1);
    pvm_send(master_tid, MSG_PROVINCE_REPORT);

    printf("? Province %d sent report to master: remaining=%d, idle=%d\n", province_id, remaining_requests, idle_distributors);
}

// Receive a new distributor tid reassigned by the master
void receive_reassigned_distributor(int new_distributor_tid) {
    // Add distributor tid to the array (expand if needed)
    distributor_tids = (int*) realloc(distributor_tids, sizeof(int) * (num_distributors + 1));
    if (!distributor_tids) {
        fprintf(stderr, "? Failed to allocate memory for new distributor\n");
        return;
    }
    distributor_tids[num_distributors] = new_distributor_tid;
    num_distributors++;
    idle_distributors++;

    printf("? Province %d received new distributor tid=%d, total now %d\n", province_id, new_distributor_tid, num_distributors);
}

// Main event loop to handle messages from master and distributors
void province_run() {
    int msg_tag, sender_tid;
    int buffer[10];  // example buffer for incoming data

    while (1) {
        sender_tid = pvm_recv(-1, -1); // receive any message from anyone with any tag
        if (sender_tid < 0) {
            fprintf(stderr, "? Province %d failed to receive message\n", province_id);
            continue;
        }

        pvm_upkint(buffer, 10, 1);  // unpack up to 10 integers, depending on message

        pvm_bufinfo(sender_tid, &msg_tag, NULL);

        switch (msg_tag) {
            case MSG_REQUEST_IDLE_DISTRIBUTOR:
                // Master or distributor requesting idle distributor count or similar
                send_report_to_master();
                break;

            case MSG_NEW_DISTRIBUTOR:
                // Master sends new distributor tid to add
                receive_reassigned_distributor(buffer[0]);
                break;

            case MSG_TERMINATE:
                printf("? Province %d received terminate signal\n", province_id);
                return; // exit loop and finish

            default:
                printf("? Province %d received unknown message tag %d\n", province_id, msg_tag);
        }
    }
}
// Clean up and free allocated resources
void province_finalize() {
    if (distributor_tids) {
        free(distributor_tids);
        distributor_tids = NULL;
    }
    printf("? Province %d finalized and resources cleaned up\n", province_id);
}

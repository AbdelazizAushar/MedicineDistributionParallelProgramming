#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "province.h"
#include "message.h"
#include "pvm_helpers.h"

// Province internal state
static int province_id = -1;
static int num_distributors = 0;
static int* distributor_tids = NULL;
static int remaining_requests = 0;
static int idle_distributors = 0;

// Initialize the province coordinator communicator and state
int province_init(int p_id, int distributors, int total_requests) {
    int i;  /* declare variables at start of block */

    province_id = p_id;
    num_distributors = distributors;
    remaining_requests = total_requests;

    distributor_tids = (int*) malloc(sizeof(int) * num_distributors);
    if (!distributor_tids) {
        fprintf(stderr, "[Province %d] Failed to allocate memory for distributors\n", province_id);
        return -1;
    }

    for (i = 0; i < num_distributors; i++) {
        distributor_tids[i] = -1;  /* Initially unknown/disconnected */
    }

    idle_distributors = num_distributors;

    printf("[Province %d] Initialized with %d distributors and %d total requests\n", 
           province_id, num_distributors, remaining_requests);
    return 0;
}

// Send a status report (remaining requests and idle distributors) to the master
void send_report_to_master() {
    int master_tid;

    master_tid = pvm_parent();
    if (master_tid < 0) {
        fprintf(stderr, "[Province %d] Cannot find master to send report\n", province_id);
        return;
    }

    pvm_initsend(PvmDataDefault);
    pvm_pkint(&province_id, 1, 1);
    pvm_pkint(&remaining_requests, 1, 1);
    pvm_pkint(&idle_distributors, 1, 1);
    pvm_send(master_tid, MSG_PROVINCE_REPORT);

    printf("[Province %d] Sent report to master: remaining=%d, idle=%d\n", province_id, remaining_requests, idle_distributors);
}

// Receive a new distributor tid reassigned by the master
void receive_reassigned_distributor(int new_distributor_tid) {
    int *temp;

    temp = (int*) realloc(distributor_tids, sizeof(int) * (num_distributors + 1));
    if (!temp) {
        fprintf(stderr, "[Province %d] Failed to allocate memory for new distributor\n", province_id);
        return;
    }
    distributor_tids = temp;
    distributor_tids[num_distributors] = new_distributor_tid;
    num_distributors++;
    idle_distributors++;

    printf("[Province %d] Received new distributor tid=%d, total now %d\n", province_id, new_distributor_tid, num_distributors);
}

// Main event loop to handle messages from master and distributors
void province_run() {
    int bufid;
    int msg_tag;
    int sender_tid;
    int buffer[10];  /* example buffer for incoming data, adjust size as needed */
    int unpacked;

    while (1) {
        bufid = pvm_recv(-1, -1);  /* receive any message from any sender and any tag */
        if (bufid < 0) {
            fprintf(stderr, "[Province %d] Failed to receive message\n", province_id);
            continue;
        }

        sender_tid = pvm_gettid(bufid);
        pvm_bufinfo(bufid, &msg_tag, NULL);

        switch (msg_tag) {
            case MSG_REQUEST_IDLE_DISTRIBUTOR:
                /* Master requesting status report */
                send_report_to_master();
                break;

            case MSG_NEW_DISTRIBUTOR:
                /* Master sends new distributor tid (expect 1 int) */
                unpacked = pvm_upkint(buffer, 1, 1);
                if (unpacked != 1) {
                    fprintf(stderr, "[Province %d] Error unpacking new distributor tid\n", province_id);
                } else {
                    receive_reassigned_distributor(buffer[0]);
                }
                break;

            case MSG_TERMINATE:
                printf("[Province %d] Received terminate signal\n", province_id);
                return;

            default:
                printf("[Province %d] Received unknown message tag %d from TID %d\n", province_id, msg_tag, sender_tid);
                break;
        }
    }
}

// Clean up and free allocated resources
void province_finalize() {
    if (distributor_tids) {
        free(distributor_tids);
        distributor_tids = NULL;
    }
    printf("[Province %d] Finalized and resources cleaned up\n", province_id);
}

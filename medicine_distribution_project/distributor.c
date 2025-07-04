#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include "distributor.h"
#include "pvm_helpers.h"

// Static/global variables
static int distributor_id;
static int province_id;
static int province_tid;
static int average_distribution_time = 2; // default, can be overridden via init

// Initialize distributor info
int distributor_init(int d_id, int p_id) {
    int my_tid;
    int time_buffer[1];

    distributor_id = d_id;
    province_id = p_id;

    my_tid = pvm_init("distributor");
    if (my_tid < 0) return -1;

    // Receive average distribution time from province
    if (recv_int_message(-1, MSG_INIT_DISTRIBUTOR, time_buffer, 1) < 0) {
        fprintf(stderr, "? Distributor %d failed to receive init data\n", distributor_id);
        return -1;
    }

    average_distribution_time = time_buffer[0];
    printf("? Distributor %d initialized (Province %d), AvgTime=%ds\n",
           distributor_id, province_id, average_distribution_time);
    return 0;
}

// Receive a single task (simulate hospital/clinic/pharmacy delivery)
int receive_task() {
    int task_buffer[1]; // Could be a type or ID
    int sender;

    sender = recv_int_message(-1, MSG_DISTRIBUTOR_STATUS, task_buffer, 1);
    if (sender < 0) {
        fprintf(stderr, "? Distributor %d failed to receive task\n", distributor_id);
        return -1;
    }

    province_tid = sender;
    printf("?? Distributor %d received task: type=%d\n", distributor_id, task_buffer[0]);

    execute_task(&task_buffer[0]);
    return 0;
}

// Simulate task execution
void execute_task(void* task_data) {
    int task_type;
    const char* type_str;

    task_type = *((int*)task_data);
    type_str = task_type == 0 ? "Pharmacy" : task_type == 1 ? "Clinic" : "Hospital";

    printf("?? Distributor %d delivering to %s...\n", distributor_id, type_str);
    Sleep(average_distribution_time);  // simulate time taken
}

// Notify province that task is done
void notify_task_done() {
    int dummy = 0;

    send_int_message(province_tid, MSG_DISTRIBUTOR_READY, &dummy, 1);
    printf("? Distributor %d completed task and is now idle\n", distributor_id);
}

// Handle optional reassignment wait (not used in every case)
void wait_for_reassignment() {
    printf("?? Distributor %d waiting for reassignment...\n", distributor_id);
    recv_signal(-1, MSG_NEW_DISTRIBUTOR); // Block until reassigned
}

// Main loop
void distributor_run() {
    int msg_tag;
    int bufid;

    while (1) {
        bufid = pvm_recv(-1, -1);  // Wait for any message
        pvm_bufinfo(bufid, NULL, &msg_tag, NULL);

        switch (msg_tag) {
            case MSG_DISTRIBUTOR_STATUS:
                receive_task();
                notify_task_done();
                break;

            case MSG_TERMINATE:
                printf("?? Distributor %d received terminate signal\n", distributor_id);
                distributor_finalize();
                return;

            default:
                printf("?? Distributor %d received unknown tag %d\n", distributor_id, msg_tag);
                break;
        }
    }
}

// Clean-up function
void distributor_finalize() {
    pvm_exit();
    printf("?? Distributor %d finalized and exiting.\n", distributor_id);
}

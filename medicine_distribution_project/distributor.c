// distributor_main.c - Main entry point for distributor process
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <pvm3.h>
#include "distributor.h"
#include "messages.h"
#include "pvm_helpers.h"

static int distributor_id = -1;
static int province_id = -1;
static int my_tid = -1;
static int province_tid = -1;
static int is_active = 1;
static int average_time = 2;

int distributor_init(int dist_id, int prov_id) {
    distributor_id = dist_id;
    province_id = prov_id;
    
    my_tid = pvm_mytid();
    if (my_tid < 0) {
        fprintf(stderr, "[Distributor %d] Failed to get TID\n", distributor_id);
        return -1;
    }
    
    province_tid = pvm_parent();
    if (province_tid < 0) {
        fprintf(stderr, "[Distributor %d] Failed to get parent TID\n", distributor_id);
        return -1;
    }
    
    printf("[Distributor %d] Initialized for province %d (TID: %d)\n", 
           distributor_id, province_id, my_tid);
    return 0;
}

void distributor_run() {
    printf("[Distributor %d] Starting main loop\n", distributor_id);
    
    while (is_active) {
        int task_received = receive_task();
        if (task_received > 0) {
            execute_task(NULL);
            notify_task_done();
        } else if (task_received == 0) {
            // No task available, wait briefly
            wait_for_reassignment();
        } else {
            // Error or termination signal
            break;
        }
    }
    
    printf("[Distributor %d] Exiting main loop\n", distributor_id);
}

int receive_task() {
    int bufid;
    int bytes, msgtag, tid;
    
    // Non-blocking receive to check for tasks
    bufid = pvm_nrecv(-1, -1);
    if (bufid < 0) {
        return 0; // No message available
    }
    
    if (pvm_bufinfo(bufid, &bytes, &msgtag, &tid) < 0) {
        fprintf(stderr, "[Distributor %d] Failed to get buffer info\n", distributor_id);
        return -1;
    }
    
    switch (msgtag) {
        case MSG_TERMINATE:
            printf("[Distributor %d] Received termination signal\n", distributor_id);
            is_active = 0;
            return -1;
            
        case MSG_ASSIGN_TASK:
            printf("[Distributor %d] Received task assignment\n", distributor_id);
            return 1;
            
        default:
            printf("[Distributor %d] Received unknown message with tag %d\n", distributor_id, msgtag);
            return 0;
    }
}

void execute_task(void* task_data) {
    printf("[Distributor %d] Executing distribution task\n", distributor_id);
    
    // Simulate distribution work
    Sleep(average_time);
    
    printf("[Distributor %d] Task completed\n", distributor_id);
}

void notify_task_done() {
    printf("[Distributor %d] Notifying province of task completion\n", distributor_id);
    
    // Send completion notification to province
    pvm_initsend(PvmDataDefault);
    pvm_pkint(&distributor_id, 1, 1);
    pvm_send(province_tid, MSG_DISTRIBUTOR_STATUS);
}

void wait_for_reassignment() {
    // Brief wait before checking again
    Sleep(500); // 500ms
}

void distributor_finalize() {
    printf("[Distributor %d] Finalizing and cleaning up\n", distributor_id);
    // Cleanup resources if needed
}

// Main function for distributor process
int main(int argc, char* argv[]) {
    int dist_id, prov_id, avg_time;
    int my_tid;

    // Check argument count
    if (argc < 4) {
        fprintf(stderr, "[Distributor] Usage: %s <distributor_id> <province_id> <avg_time>\n", argv[0]);
        return 1;
    }

    // Parse arguments
    dist_id = atoi(argv[1]);
    prov_id = atoi(argv[2]);
    avg_time = atoi(argv[3]);
    average_time = avg_time;

    // Get my TID
    my_tid = pvm_mytid();
    if (my_tid < 0) {
        fprintf(stderr, "[Distributor] Failed to get TID\n");
        return 1;
    }

    printf("[Distributor %d] Starting with TID %d for province %d\n", dist_id, my_tid, prov_id);

    // Initialize distributor
    if (distributor_init(dist_id, prov_id) != 0) {
        return 1;
    }

    // Run main loop
    distributor_run();


    // Finalize
    distributor_finalize();

    // Exit PVM
    pvm_exit();
    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pvm3.h>
#include "province.h"
#include "messages.h"
#include "pvm_helpers.h"

/* Province internal state */
static int province_id = -1;
static int num_distributors = 0;
static int* distributor_tids = NULL;
static int* distributor_busy = NULL;  // 0 = idle, 1 = busy
static int remaining_requests = 0;
static int idle_distributors = 0;
static int master_tid = -1;
static int average_time = 2;
static int expected_distributors = 0;  // How many we're waiting for
/* Initialize the province coordinator and spawn distributors */

int province_init(int p_id, int distributors, int total_requests, int avg_time)
{
    int i;
    char args[3][20];
    char* arg_ptrs[4];
    int result;

    province_id = p_id;
    num_distributors = distributors;
    remaining_requests = total_requests;
    idle_distributors = 0; /* Will be set as distributors come online */
    average_time = avg_time;

    /* Get master TID */
    master_tid = pvm_parent();
    if (master_tid < 0) {
        fprintf(stderr, "[Province %d] Cannot find master TID\n", province_id);
        return -1;
    }

    /* Allocate memory for distributor TIDs */
    distributor_tids = (int*) malloc(sizeof(int) * num_distributors);
    if (!distributor_tids) {
        fprintf(stderr, "[Province %d] Failed to allocate memory for distributors\n", province_id);
        return -1;
    }

    /* Initialize distributor TIDs */
    for (i = 0; i < num_distributors; i++) {
        distributor_tids[i] = -1;
    }

	// In province_init(), allocate and initialize the busy array:
		distributor_busy = (int*) malloc(sizeof(int) * num_distributors);
		for (i = 0; i < num_distributors; i++) {
			distributor_busy[i] = 0;  // 0 = idle, 1 = busy
		}

    printf("[Province %d] Initialized with %d distributors and %d total requests\n",
           province_id, num_distributors, remaining_requests);

    /* Spawn distributor processes */
    printf("[Province %d] Spawning %d distributors\n", province_id, num_distributors);

    for (i = 0; i < num_distributors; i++) {
        sprintf(args[0], "%d", i);            /* distributor_id */
        sprintf(args[1], "%d", province_id); /* province_id */
        sprintf(args[2], "%d", average_time); /* average_time */

        arg_ptrs[0] = args[0];
        arg_ptrs[1] = args[1];
        arg_ptrs[2] = args[2];
        arg_ptrs[3] = NULL;

        result = pvm_spawn("distributor_process", arg_ptrs, PvmTaskDefault, "", 1, &distributor_tids[i]);

        if (result != 1) {
            fprintf(stderr, "[Province %d] Failed to spawn distributor %d\n", province_id, i);
            distributor_tids[i] = -1;
        }
        else {
            idle_distributors++;
            printf("[Province %d] Spawned distributor %d with TID %d\n",
                   province_id, i, distributor_tids[i]);
        }
    }

    return 0;
}

/* Send a status report to the master */
void send_report_to_master(void)
{
    int buffer[3];

    buffer[0] = province_id;
    buffer[1] = remaining_requests;
    buffer[2] = idle_distributors;

    /* Use helper to send integer array */
    send_int_message(master_tid, MSG_PROVINCE_REPORT, buffer, 3);

    printf("[Province %d] Sent report to master: remaining=%d, idle=%d\n",
           province_id, remaining_requests, idle_distributors);
}

/* Handle request for idle distributor from master */
void handle_idle_distributor_request(void)
{
    int i;

    if (idle_distributors > 0) {
        /* Find an idle distributor */
        for (i = 0; i < num_distributors; i++) {
            if (distributor_tids[i] > 0 && distributor_busy[i] == 0) {
                int val = distributor_tids[i];
                send_int_message(master_tid, MSG_IDLE_DISTRIBUTOR, &val, 1);

                distributor_tids[i] = -1;
                idle_distributors--;

                printf("[Province %d] Sent idle distributor to master for reassignment\n", province_id);
                return;
            }
        }
    }

    /* No idle distributors available */
    {
        int val = -1;
        send_int_message(master_tid, MSG_IDLE_DISTRIBUTOR, &val, 1);
    }
    printf("[Province %d] No idle distributors available\n", province_id);
}

/* Receive a new distributor reassigned by the master */
/* Receive a new distributor reassigned by the master */
void receive_reassigned_distributor(int new_distributor_tid)
{
    int i;
    int* temp_tids;
    int* temp_busy;

    /* Find empty slot */
    for (i = 0; i < num_distributors; i++) {
        if (distributor_tids[i] == -1) {
            distributor_tids[i] = new_distributor_tid;
            distributor_busy[i] = 0;  // Mark as idle
            idle_distributors++;
            printf("[Province %d] Received reassigned distributor TID %d in slot %d\n",
                   province_id, new_distributor_tid, i);
            return;
        }
    }

    /* Need to expand both arrays */
    temp_tids = (int*) realloc(distributor_tids, sizeof(int) * (num_distributors + 1));
    if (!temp_tids) {
        fprintf(stderr, "[Province %d] Failed to allocate memory for new distributor TIDs\n", province_id);
        return;
    }

    temp_busy = (int*) realloc(distributor_busy, sizeof(int) * (num_distributors + 1));
    if (!temp_busy) {
        fprintf(stderr, "[Province %d] Failed to allocate memory for new distributor busy array\n", province_id);
        free(temp_tids);  // Clean up the first allocation
        return;
    }

    /* Update pointers */
    distributor_tids = temp_tids;
    distributor_busy = temp_busy;
    
    /* Add new distributor */
    distributor_tids[num_distributors] = new_distributor_tid;
    distributor_busy[num_distributors] = 0;  // Mark as idle
    num_distributors++;
    idle_distributors++;

    printf("[Province %d] Received new distributor tid=%d, total now %d, idle now %d\n",
           province_id, new_distributor_tid, num_distributors, idle_distributors);
}

/* Simulate task assignment to distributors */
void assign_tasks_to_distributors(void)
{
    int i;

    if (remaining_requests > 0 && idle_distributors > 0) {
        for (i = 0; i < num_distributors; i++) {
            if (distributor_tids[i] > 0 && distributor_busy[i] == 0) {  // Available and idle
                int val = remaining_requests;
                send_int_message(distributor_tids[i], MSG_ASSIGN_TASK, &val, 1);

                remaining_requests--;
                idle_distributors--;
                distributor_busy[i] = 1;  // Mark as busy

                printf("[Province %d] Assigned task to distributor %d (TID: %d), remaining=%d\n",
                       province_id, i, distributor_tids[i], remaining_requests);
                
                if (remaining_requests == 0 || idle_distributors == 0) {
                    break;
                }
            }
        }
    }
}

/* Main event loop to handle messages */
void province_run(void)
{
    int bufid;
    int msg_tag;
    int sender_tid;
    int bytes, unpacked;
    int buffer[10];
    int report_counter = 0;
	int i;
    printf("[Province %d] Starting main event loop\n", province_id);

    /* Register with master */
    {
        int mytid = pvm_mytid();
		pvm_initsend(PvmDataDefault);
		pvm_pkint(&province_id, 1, 1);
		pvm_send(master_tid, MSG_REGISTER_PROVINCE);
    }

    while (1) {
        /* Assign tasks if we have requests and idle distributors */
        assign_tasks_to_distributors();

        /* Send periodic reports to master (every 10 iterations) */
        if (report_counter % 10 == 0) {
            send_report_to_master();
        }
        report_counter++;

        /* Non-blocking receive to check for messages */
        bufid = pvm_nrecv(-1, -1);
        if (bufid < 0) {
            /* No message, continue */
            continue;
        }

        /* Get message info */
        pvm_bufinfo(bufid, &bytes, &msg_tag, &sender_tid);

        switch (msg_tag) {
            case MSG_REQUEST_IDLE_DISTRIBUTOR:
                handle_idle_distributor_request();
                break;

            case MSG_NEW_DISTRIBUTOR:
                unpacked = pvm_upkint(buffer, 1, 1);
                if (unpacked == 1) {
                    receive_reassigned_distributor(buffer[0]);
                }
                break;

            case MSG_DISTRIBUTOR_STATUS:
			
				/* Distributor finished a task */
				pvm_upkint(buffer, 1, 1);
    
				// Find which distributor this is and mark as idle
				for (i = 0; i < num_distributors; i++) {
					if (distributor_tids[i] == sender_tid && distributor_busy[i] == 1) {
						distributor_busy[i] = 0;  // Mark as idle
						idle_distributors++;
						printf("[Province %d] Distributor %d finished task, idle count: %d\n",
							   province_id, i, idle_distributors);
						break;
					}
				}
				break;

            case MSG_TERMINATE:
                printf("[Province %d] Received terminate signal\n", province_id);
                {
                    int i;
                    /* Terminate all distributors */
                    for (i = 0; i < num_distributors; i++) {
                        if (distributor_tids[i] > 0) {
                            send_signal(distributor_tids[i], MSG_TERMINATE);
                        }
                    }
                }
                province_finalize();
                return;

            default:
                printf("[Province %d] Received unknown message tag %d from TID %d\n",
                       province_id, msg_tag, sender_tid);
                break;
        }
    }
}

/* Clean up and free allocated resources */
void province_finalize(void)
{
    if (distributor_tids) {
        free(distributor_tids);
        distributor_tids = NULL;
    }
    if (distributor_busy) {
        free(distributor_busy);
        distributor_busy = NULL;
    }
    printf("[Province %d] Finalized and resources cleaned up\n", province_id);
}

/* Main function for province process */
int main(int argc, char* argv[])
{
    int province_id_arg;
    int num_distributors_arg;
    int total_requests_arg;
    int avg_time_arg;
    int my_tid;

    /* Check argument count */
    if (argc < 5) {
        fprintf(stderr, "[Province] Usage: %s <province_id> <num_distributors> <total_requests> <avg_time>\n", argv[0]);
        return 1;
    }

    province_id_arg = atoi(argv[1]);
    num_distributors_arg = atoi(argv[2]);
    total_requests_arg = atoi(argv[3]);
    avg_time_arg = atoi(argv[4]);

    my_tid = pvm_mytid();
    printf("[Province %d] Starting with TID %d\n", province_id_arg, my_tid);

    if (province_init(province_id_arg, num_distributors_arg, total_requests_arg, avg_time_arg) != 0) {
        fprintf(stderr, "[Province %d] Initialization failed\n", province_id_arg);
        return 1;
    }

    province_run();

    province_finalize();

    return 0;
}

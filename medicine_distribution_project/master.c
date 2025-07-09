#include <stdio.h>
#include <pvm3.h>
#include "master.h"
#include "pvm_helpers.h"


ProvinceStatus province_status[MAX_PROVINCES];
int num_provinces = 0;

int master_init(int total_provinces) {
    int i;
    int sender_tid;
    int bufid;
    int bytes, msg_tag, tid;
    int province_id;

    if (total_provinces > MAX_PROVINCES) {
        fprintf(stderr, "[Master] Exceeded max number of provinces (%d)\n", MAX_PROVINCES);
        return -1;
    }

    num_provinces = total_provinces;

    printf("[Master] Waiting for %d provinces to register\n", num_provinces);

    for (i = 0; i < num_provinces; i++) {
        // Receive a message with tag MSG_REGISTER_PROVINCE from any sender
        bufid = pvm_recv(-1, MSG_REGISTER_PROVINCE);
        if (bufid < 0) {
            fprintf(stderr, "[Master] Failed to receive registration from province %d\n", i);
            return -1;
        }

        // Get message info (bytes, tag, sender tid)
        if (pvm_bufinfo(bufid, &bytes, &msg_tag, &tid) < 0) {
            fprintf(stderr, "[Master] Failed to get buffer info for province %d\n", i);
            return -1;
        }

        // Unpack the province ID (integer) from the message
		pvm_upkint(&province_id, 1, 1);
       
        // Store sender TID for this province, indexed by province_id
        if (province_id < 0 || province_id >= MAX_PROVINCES) {
            fprintf(stderr, "[Master] Invalid province ID %d received\n", province_id);
            return -1;
        }

        province_status[province_id].tid = tid;
        province_status[province_id].remaining_requests = 0;
        province_status[province_id].idle_distributors = 0;

        printf("[Master] Registered province %d with TID %d\n", province_id, tid);
    }

    printf("[Master] All provinces registered successfully\n");
    return 0;
}


void master_run() {
    int buffer[3];
    int sender_tid, bufid, bytes, msgtag, tid;
    int i, done, total_remaining;
    int province_id, remaining, idle;

    printf("[Master] Starting main coordination loop\n");

    while (1) {
        bufid = pvm_recv(-1, MSG_PROVINCE_REPORT);
		pvm_bufinfo(bufid, &bytes, &msgtag, &tid);
        sender_tid = tid;
		pvm_upkint(buffer, 3, 1);

        province_id = buffer[0];
        remaining = buffer[1];
        idle = buffer[2];


        if (province_id < 0 || province_id >= num_provinces) {
            fprintf(stderr, "[Master] Invalid province ID: %d\n", province_id);
            continue;
        }

        handle_province_report(province_id, remaining, idle);

        done = 1;
        total_remaining = 0;

        for (i = 0; i < num_provinces; i++) {
            total_remaining += province_status[i].remaining_requests;
            if (province_status[i].remaining_requests > 0) {
                done = 0;
            }
        }

        printf("[Master] Total remaining requests across all provinces: %d\n", total_remaining);

        if (done) {
            printf("[Master] All provinces completed. Finalizing.\n");
            master_finalize();
            break;
        }
    }
}


void handle_province_report(int province_id, int remaining_requests, int idle_distributors) {
    int i;

    province_status[province_id].remaining_requests = remaining_requests;
    province_status[province_id].idle_distributors = idle_distributors;

    printf("[Master] Province %d report: remaining=%d, idle=%d\n",
           province_id, remaining_requests, idle_distributors);

    if (idle_distributors > 0) {
        for (i = 0; i < num_provinces; i++) {
            if (i != province_id && province_status[i].remaining_requests > 0) {
                printf("[Master] Province %d has work, province %d has idle distributors\n",
                       i, province_id);
                reassign_distributors(province_id, i);
                break;
            }
        }
    }
}


void reassign_distributors(int source_province_id, int target_province_id) {
    int distributor_tid;
    int sender_tid;

    printf("[Master] Requesting idle distributor from province %d\n", source_province_id);

    send_int(province_status[source_province_id].tid, MSG_REQUEST_IDLE_DISTRIBUTOR, 0);

    distributor_tid = recv_int(&sender_tid, MSG_IDLE_DISTRIBUTOR);

    if (distributor_tid <= 0) {
        printf("[Master] No idle distributor available from province %d\n", source_province_id);
        return;
    }

    notify_province_of_new_distributor(distributor_tid, province_status[target_province_id].tid);

    province_status[source_province_id].idle_distributors--;
    province_status[target_province_id].idle_distributors++;

    printf("[Master] Reassigned distributor %d from province %d to province %d\n",
           distributor_tid, source_province_id, target_province_id);
}


void master_finalize() {
    int i;

    printf("[Master] Sending termination signals to all provinces\n");

    for (i = 0; i < num_provinces; i++) {
        send_signal(province_status[i].tid, MSG_TERMINATE);
        printf("[Master] Sent termination to province %d (TID: %d)\n", i, province_status[i].tid);
    }

    printf("[Master] All provinces notified of termination\n");
}


void notify_province_of_new_distributor(int distributor_tid, int target_province_tid) {
    pvm_initsend(PvmDataDefault);
    pvm_pkint(&distributor_tid, 1, 1);
    pvm_send(target_province_tid, MSG_NEW_DISTRIBUTOR);

    printf("[Master] Notified province (TID: %d) of new distributor (TID: %d)\n",
           target_province_tid, distributor_tid);
}

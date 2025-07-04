#include "master.h"
#include "pvm_helpers.h"

ProvinceStatus province_status[MAX_PROVINCES];
int num_provinces = 0;

int master_init(int total_provinces) {
    int i;
    int tid;
    int buffer[1];  // Buffer to receive tid

    if (total_provinces > MAX_PROVINCES) {
        fprintf(stderr, "Exceeded max number of provinces (%d)\n", MAX_PROVINCES);
        return -1;
    }

    num_provinces = total_provinces;

    for (i = 0; i < num_provinces; i++) {
        // Receive tid from any source (-1), with MSG_REGISTER_PROVINCE tag, into buffer of size 1
        if (recv_int_message(-1, MSG_REGISTER_PROVINCE, buffer, 1) < 0) {
            fprintf(stderr, "[Master] Failed to receive registration from province %d\n", i);
            return -1;
        }
        tid = buffer[0];

        province_status[i].tid = tid;
        province_status[i].remaining_requests = 0;
        province_status[i].idle_distributors = 0;

        printf("[Master] Registered province %d with TID %d\n", i, tid);
    }
    return 0;
}

void master_run() {
    int buffer[3]; // [province_id, remaining_requests, idle_distributors]
    int sender_tid;
    int province_id;
    int remaining;
    int idle;
    int done;
    int i;

    while (1) {
        if (pvm_recv(-1, MSG_PROVINCE_REPORT) < 0) {
            fprintf(stderr, "[Master] Failed to receive province report\n");
            continue;
        }
        pvm_upkint(buffer, 3, 1);
        sender_tid = pvm_getrbuf();

        province_id = buffer[0];
        remaining = buffer[1];
        idle = buffer[2];

        handle_province_report(province_id, remaining, idle);

        done = 1;
        for (i = 0; i < num_provinces; i++) {
            if (province_status[i].remaining_requests > 0) {
                done = 0;
                break;
            }
        }

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

    if (idle_distributors > 0) {
        for (i = 0; i < num_provinces; i++) {
            if (i != province_id && province_status[i].remaining_requests > 0) {
                reassign_distributors(province_id, i);
                break;
            }
        }
    }
}

void reassign_distributors(int source_province_id, int target_province_id) {
    int distributor_tid;
    int buffer[1];
    int dummy_value = 0;

    // Request an idle distributor from source province
    send_int_message(province_status[source_province_id].tid, MSG_REQUEST_IDLE_DISTRIBUTOR, &dummy_value, 1);

    // Receive distributor tid
    if (recv_int_message(-1, MSG_IDLE_DISTRIBUTOR, buffer, 1) < 0) {
        fprintf(stderr, "[Master] Failed to receive idle distributor from province %d\n", source_province_id);
        return;
    }
    distributor_tid = buffer[0];

    notify_province_of_new_distributor(distributor_tid, province_status[target_province_id].tid);

    printf("[Master] Reassigned distributor %d from province %d to province %d\n",
           distributor_tid, source_province_id, target_province_id);
}

void master_finalize() {
    int i;
    int dummy_value = 0;
    for (i = 0; i < num_provinces; i++) {
        send_int_message(province_status[i].tid, MSG_TERMINATE, &dummy_value, 1);
    }
}

void notify_province_of_new_distributor(int distributor_tid, int target_province_tid) {
    pvm_initsend(PvmDataDefault);
    pvm_pkint(&distributor_tid, 1, 1);
    pvm_send(target_province_tid, MSG_NEW_DISTRIBUTOR);
}

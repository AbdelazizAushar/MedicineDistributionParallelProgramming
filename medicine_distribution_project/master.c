#include "master.h"
#include "pvm_helpers.h"

ProvinceStatus province_status[MAX_PROVINCES];
int num_provinces = 0;

int master_init(int total_provinces) {
    if (total_provinces > MAX_PROVINCES) {
        fprintf(stderr, "Exceeded max number of provinces (%d)\n", MAX_PROVINCES);
        return -1;
    }

    num_provinces = total_provinces;

    // ?????? ????? ?? ????? ?????????
    for (int i = 0; i < num_provinces; i++) {
        int tid = recv_int(NULL, MSG_REGISTER_PROVINCE);
        province_status[i].tid = tid;
        province_status[i].remaining_requests = 0;
        province_status[i].idle_distributors = 0;
        printf("[Master] Registered province %d with TID %d\n", i, tid);
    }
    return 0;
}

void master_run() {
    while (1) {
        int buffer[3]; // [province_id, remaining_requests, idle_distributors]
        int sender_tid;
        pvm_recv(-1, MSG_PROVINCE_REPORT);
        pvm_upkint(buffer, 3, 1);
        sender_tid = pvm_getrbuf();

        int province_id = buffer[0];
        int remaining = buffer[1];
        int idle = buffer[2];

        handle_province_report(province_id, remaining, idle);

        // ?????? ?? ????? ?? ?????????
        int done = 1;
        for (int i = 0; i < num_provinces; i++) {
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
    province_status[province_id].remaining_requests = remaining_requests;
    province_status[province_id].idle_distributors = idle_distributors;

    // ?????? ????? ?????? ?????? ???? ???????? ?????
    if (idle_distributors > 0) {
        for (int i = 0; i < num_provinces; i++) {
            if (i != province_id && province_status[i].remaining_requests > 0) {
                reassign_distributors(province_id, i);
                break;
            }
        }
    }
}

void reassign_distributors(int source_province_id, int target_province_id) {
    int distributor_tid;

    // ??????? ???? ?????? ?????? ?? ???? ???????? ??????
    send_int(province_status[source_province_id].tid, MSG_REQUEST_IDLE_DISTRIBUTOR, 0);
    distributor_tid = recv_int(NULL, MSG_IDLE_DISTRIBUTOR);

    // ????? ???? ?????? ??? ???????? ?????
    notify_province_of_new_distributor(distributor_tid, province_status[target_province_id].tid);

    printf("[Master] Reassigned distributor %d from province %d to province %d\n",
           distributor_tid, source_province_id, target_province_id);
}

void master_finalize() {
    for (int i = 0; i < num_provinces; i++) {
        send_int(province_status[i].tid, MSG_TERMINATE, 0);
    }
}

void notify_province_of_new_distributor(int distributor_tid, int target_province_tid) {
    pvm_initsend(PvmDataDefault);
    pvm_pkint(&distributor_tid, 1, 1);
    pvm_send(target_province_tid, MSG_NEW_DISTRIBUTOR);
}

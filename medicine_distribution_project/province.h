#ifndef PROVINCE_H
#define PROVINCE_H

#include <pvm3.h>
#include "messages.h"

int province_init(int province_id, int num_distributors, int total_requests);

void province_run();

void send_report_to_master();

void receive_reassigned_distributor(int new_distributor_tid);

void province_finalize();

#endif 
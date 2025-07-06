#ifndef MASTER_H
#define MASTER_H

#include <pvm3.h>
#include "messages.h"

int master_init(int total_provinces);

void master_run();

void handle_province_report(int province_id, int remaining_requests, int idle_distributors);

void reassign_distributors(int source_province_id, int target_province_id);

void master_finalize();

void notify_province_of_new_distributor(int distributor_tid, int target_province_tid);

#endif 
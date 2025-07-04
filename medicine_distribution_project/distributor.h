#ifndef DISTRIBUTOR_H
#define DISTRIBUTOR_H

#include <pvm3.h>
#include "messages.h"

int distributor_init(int distributor_id, int province_id);

void distributor_run();

int receive_task();

void execute_task(void* task_data);

void notify_task_done();

void wait_for_reassignment();

void distributor_finalize();

#endif 

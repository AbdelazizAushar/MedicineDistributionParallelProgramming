#ifndef DISTRIBUTOR_H
#define DISTRIBUTOR_H

#include <pvm3.h>
#include "messages.h"

// Initialize distributor with ID and province assignment
int distributor_init(int distributor_id, int province_id);

// Main distributor execution loop
void distributor_run();

// Receive and handle task assignments
int receive_task();

// Execute a distribution task
void execute_task(void* task_data);

// Notify completion of task
void notify_task_done();

// Wait for reassignment or new tasks
void wait_for_reassignment();

// Clean up distributor resources
void distributor_finalize();

// Main function for distributor process
int distributor_main(int argc, char* argv[]);

#endif // DISTRIBUTOR_H

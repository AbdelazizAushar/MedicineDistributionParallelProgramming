#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "input.h"
#include "master.h"
#include "province.h"

int main(int argc, char* argv[]) {
    // === Variable Declarations ===
    int mytid;
    SystemInput input_data;
    int province_tids[MAX_PROVINCES];
    int spawned_count;
    int i;
    int total_requests;
    char args[4][20];
    int tid;
    int result;

    // === Initialize PVM ===
    mytid = pvm_mytid();
    if (mytid < 0) {
        fprintf(stderr, "Failed to initialize PVM\n");
        return 1;
    }

    printf("=== Medicine Distribution System ===\n");

    // Read user input
    read_user_input(&input_data);
    print_input_summary(&input_data);

    // Spawn province processes
    spawned_count = 0;

    printf("\n=== Spawning Province Processes ===\n");

    for (i = 0; i < input_data.num_provinces; i++) {
        // Calculate total requests for this province
        total_requests = input_data.provinces[i].points.pharmacies +
                         input_data.provinces[i].points.clinics +
                         input_data.provinces[i].points.hospitals;

        sprintf(args[0], "province");
        sprintf(args[1], "%d", i);  // province_id
        sprintf(args[2], "%d", input_data.provinces[i].num_distributors);
        sprintf(args[3], "%d", total_requests);  // total requests

        result = pvm_spawn("medicine_distribution_project", args, 0, "", 4, &tid);

        if (result < 0) {
            fprintf(stderr, "Failed to spawn province %d\n", i);
            continue;
        }

        province_tids[spawned_count] = tid;
        spawned_count++;

        printf("Spawned province %d with TID %d (%d distributors, %d requests)\n", 
               i, tid, input_data.provinces[i].num_distributors, total_requests);
    }

    if (spawned_count == 0) {
        fprintf(stderr, "No provinces were spawned successfully\n");
        return 1;
    }

    printf("\n=== Starting Master Process ===\n");
    printf("Successfully spawned %d provinces\n", spawned_count);

    // Initialize and run master
    if (master_init(spawned_count) != 0) {
        fprintf(stderr, "Failed to initialize master\n");
        return 1;
    }

    master_run();

    printf("\n=== System Completed ===\n");
    return 0;
}

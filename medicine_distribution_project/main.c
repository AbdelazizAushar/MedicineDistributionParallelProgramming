#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pvm3.h>
#include <windows.h>  // For high-resolution timer
#include "input.h"
#include "master.h"
#include "province.h"
#include "messages.h"

int main(int argc, char* argv[]) {
    int mytid;
    SystemInput input_data;
    int i, j;
    int total_requests;
    char args[5][20];     // Buffer for argument strings
    char* arg_ptrs[6];    // Array of pointers (must end with NULL)
    int result;
    int spawned_count = 0;

    int sequential = 0;
    int total_requests_overall = 0;

    LARGE_INTEGER frequency, start_time, end_time;
    double parallel_time_ms = 0.0;

    // Initialize PVM
    mytid = pvm_mytid();
    if (mytid < 0) {
        fprintf(stderr, "Failed to initialize PVM\n");
        return 1;
    }

    printf("=== Medicine Distribution System ===\n");
    printf("Master TID: %d\n", mytid);

    // Read user input
    read_user_input(&input_data);
    print_input_summary(&input_data);

    printf("\n=== Spawning Province Processes ===\n");

    // Spawn province processes
    for (i = 0; i < input_data.num_provinces; i++) {
        // Calculate total requests for this province
        total_requests = input_data.provinces[i].points.pharmacies +
                         input_data.provinces[i].points.clinics +
                         input_data.provinces[i].points.hospitals;

        total_requests_overall += total_requests;

        // Prepare arguments
        sprintf(args[0], "%d", i);  // province_id
        sprintf(args[1], "%d", input_data.provinces[i].num_distributors);
        sprintf(args[2], "%d", total_requests);
        sprintf(args[3], "%d", input_data.average_distribution_time);

        // Set up pointer array (null-terminated)
        for (j = 0; j < 4; j++) {
            arg_ptrs[j] = args[j];
        }
        arg_ptrs[4] = NULL;

        // Spawn province process
        result = pvm_spawn("province_process", arg_ptrs, PvmTaskDefault, "", 1, &spawned_count);

        if (result != 1) {
            fprintf(stderr, "Failed to spawn province %d (result: %d)\n", i, result);
            continue;
        }

        printf("Successfully spawned province %d (TID will be received during registration)\n", i);
    }

    if (spawned_count == 0) {
        fprintf(stderr, "No provinces were spawned successfully\n");
        return 1;
    }

    // Calculate sequential time
    sequential = total_requests_overall * input_data.average_distribution_time;

    printf("\n=== Starting Master Process ===\n");
    printf("Waiting for %d provinces to register\n", input_data.num_provinces);

    // Initialize master
    if (master_init(input_data.num_provinces) != 0) {
        fprintf(stderr, "Failed to initialize master\n");
        return 1;
    }

    // Start high-res timer (Windows)
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&start_time);

    // Run master coordination
    master_run();

    // End timer
    QueryPerformanceCounter(&end_time);

    // Compute parallel time in milliseconds
    parallel_time_ms = (double)(end_time.QuadPart - start_time.QuadPart) * 1000.0 / frequency.QuadPart;

    printf("\n=== System Completed Successfully ===\n");

    // Print timings
    printf("\n=== Sequential / Parallel Time Comparison ===\n");
    printf("Sequential would take = %d seconds\n", sequential);
    printf("Parallel   = %.2f ms\n", parallel_time_ms);

    // Exit PVM
    pvm_exit();
    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <pvm3.h>

#include "province.h"
#include "pvm_helpers.h"
#include "messages.h"

int prov_main(int argc, char* argv[]) {
    int province_id;
    int num_distributors;
    int total_requests;
    int my_tid;
    int master_tid;

    /* Check argument count */
    if (argc < 4) {
        fprintf(stderr, "[Province] Invalid number of arguments. Expected 3.\n");
        return 1;
    }

    /* Parse arguments */
    province_id = atoi(argv[1]);         /* Province ID */
    num_distributors = atoi(argv[2]);    /* Number of distributors */
    total_requests = atoi(argv[3]);      /* Total number of requests */

    /* Get TIDs */
    my_tid = pvm_mytid();
    master_tid = pvm_parent();

    /* Send registration to master (fixed!) */
    if (send_int_message(master_tid, MSG_REGISTER_PROVINCE, &my_tid, 1) < 0) {
        fprintf(stderr, "[Province %d] Failed to register with master.\n", province_id);
        return 1;
    }

    /* Initialize province */
    if (province_init(province_id, num_distributors, total_requests) != 0) {
        return 1;
    }

    /* Run main loop */
    province_run();

    /* Final cleanup */
    province_finalize();

    return 0;
}
